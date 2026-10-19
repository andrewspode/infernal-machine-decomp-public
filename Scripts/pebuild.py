"""PE/COFF backend: split, compare, reconstruct at fixed addresses, and verify.

This is deliberately not a general Windows linker. Original image layout,
imports, resources and unclaimed bytes are preserved as explicit COFF inputs.
Only complete, reviewed TUs selected in configure.py are replaced by MSVC output.
"""
import argparse
import bisect
import hashlib
import json
from pathlib import Path
import struct

import pefile

from . import coff
from .pe_link import align_up, digest
from .pe_image import link, match
from .pe_project import DEFAULT_VERSION, Project, SECTION_EXTRA_FLAGS, SECTION_KINDS, UNASSIGNED, write_changed, write_json


def checksum(path, expected):
    actual = hashlib.sha256(Path(path).read_bytes()).hexdigest()
    if actual != expected:
        raise ValueError(f'{path}: SHA-256 mismatch\n  expected {expected}\n  actual   {actual}')
    return actual


CODE_PADDING = (0x90, 0xCC)


def folded_extent(data, address):
    """Measure the retail function body at `address`, trailing padding included.

    A fold onto an unowned address has no `symbols.txt` extent to compare
    against, so the extent has to come out of the image: a retail body ends at
    the 16-byte boundary its trailing linker padding reaches, which is the
    first offset holding a padding byte (0x90 or 0xCC) where every byte up to
    the next boundary is padding too and real content resumes exactly there.
    That reproduces the extent convention the reviewed `symbols.txt` sizes
    already use for the owned folded bodies (0x10 for each return-only one).
    Returns None when the bytes are not delimited that way, which the strict
    linker then rejects. `data` is the image starting at `address`.
    """
    for offset in range(1, len(data)):
        if data[offset] not in CODE_PADDING:
            continue
        end = align_up(address + offset, 16) - address
        if end <= offset or end >= len(data):
            continue
        if all(b in CODE_PADDING for b in data[offset:end]) and data[end] not in CODE_PADDING:
            return end
    return None


def split(project):
    original = project.root / project.config['original']
    checksum(original, project.config['sha256'])
    data = original.read_bytes()
    pe = pefile.PE(data=data)
    base = pe.OPTIONAL_HEADER.ImageBase
    if pe.FILE_HEADER.Machine != int(str(project.config['machine']), 0) or base != int(str(project.config['image_base']), 0):
        raise ValueError('Unexpected PE machine or image base')
    layout = {'schema': 3, 'identity': project.layout_identity, 'sha256': project.config['sha256'],
              'file_size': len(data), 'image_base': base, 'symbols': {}, 'folds': {},
              'fragments': [], 'ranges': [], 'imports': {}, 'sections': []}
    for descriptor in getattr(pe, 'DIRECTORY_ENTRY_IMPORT', []):
        for imp in descriptor.imports:
            if imp.name:
                layout['imports'][imp.name.decode('ascii')] = imp.address
    for section in pe.sections:
        layout['sections'].append({'name': section.Name.rstrip(b'\0').decode(),
                                   'address': base + section.VirtualAddress,
                                   'size': section.Misc_VirtualSize,
                                   'file_offset': section.PointerToRawData,
                                   'file_size': section.SizeOfRawData})
    claimed = []
    flags = {'.text': coff.CODE, '_TEXT16': coff.CODE | SECTION_EXTRA_FLAGS['_TEXT16'], '.rdata': coff.RDATA,
             '.data': coff.DATA, '.data1': coff.DATA, '.bss': coff.BSS}
    for unit, parts in sorted(project.unit_splits.items()):
        sections, symbols = [], []
        path = project.obj('orig', unit)
        for part in parts:
            va, size, kind = part['start'], part['end'] - part['start'], part['section']
            section = pe.get_section_by_rva(va - base)
            if not section or part['end'] > base + section.VirtualAddress + section.Misc_VirtualSize:
                raise ValueError(f'{unit}: split at 0x{va:08X} exceeds a PE section')
            if bool(section.Characteristics & 0x20) != (SECTION_KINDS[kind] == 'code'):
                raise ValueError(f'{unit}: split at 0x{va:08X} has the wrong code/data kind')
            relative = va - base - section.VirtualAddress
            backed = min(size, max(0, section.SizeOfRawData - relative))
            offset = section.PointerToRawData + relative
            raw = data[offset:offset + backed] if backed else b''
            if len(raw) != backed:
                raise ValueError('Split extends past the executable file')
            if part.get('lead') and not part.get('lead_after_range'):
                # Not after a recorded range: the preceding contribution must
                # visibly end at the fill, so the fill is all the alignment adds.
                previous = pe.get_data(va - base - 1, 1)
                if not relative or previous[0] == part['padding']:
                    raise ValueError(f'{unit}: leading alignment fill at 0x{va:08X} does not follow '
                                     'a recorded range or the end of other contents')
            if kind == '.bss':
                if not section.Characteristics & 0x80000000 or any(raw):
                    raise ValueError(f'{unit}: BSS must occupy writable, zero-initialized storage')
                payload = b''
            else:
                if backed != size:
                    raise ValueError(f'{unit}: initialized split extends into zero-fill storage')
                payload = raw
            alignment = part['align'].bit_length() << 20
            characteristics = (flags[kind] & ~0xF00000) | alignment
            sections.append(coff.Section(kind, payload, characteristics, size=size))
            for symbol in project.units[unit]:
                if va <= symbol['address'] < part['end']:
                    name = symbol.get('coff_name') or '_' + symbol['name']
                    symbols.append(coff.Symbol(name, symbol['address'] - va, len(sections),
                                               0x20 if symbol['kind'] == 'function' else 0,
                                               3 if symbol['scope'] == 'local' else 2))
            entry = {'object': path.as_posix(), 'section': len(sections), 'source': unit, 'address': va,
                     'size': size, 'kind': kind, 'align': part['align'], 'file_size': backed,
                     'sha256': digest(payload) if kind != '.bss' else None}
            layout['ranges'].append(entry)
            if backed:
                layout['fragments'].append({**entry, 'size': backed, 'section_size': size,
                                            'file_offset': offset, 'sha256': digest(raw)})
                claimed.append((offset, offset + backed))
        write_changed(project.root / path, coff.encode(sections, symbols))

    for symbol in project.symbols.values():
        if symbol['kind'] != 'label':
            layout['symbols'][symbol['name']] = {
                'sha256': None if symbol['section'] == '.bss' else
                digest(pe.get_data(symbol['address'] - base, symbol['size'])),
            }

    # Retail payload for folds declared onto addresses no TU owns. The measured
    # extent and digest come from the image alone; the strict linker compares
    # both against the declaration and the compiled COMDAT.
    for source, address, size in project.unowned_folds:
        section = pe.get_section_by_rva(address - base)
        if (not section or not section.Characteristics & 0x20 or address < base + section.VirtualAddress
                or address + size > base + section.VirtualAddress + section.SizeOfRawData):
            raise ValueError(f'{source}: folded address 0x{address:08X} is not file-backed code')
        limit = base + section.VirtualAddress + section.SizeOfRawData - address
        extent = folded_extent(pe.get_data(address - base, min(limit, 4096)), address)
        layout['folds'][f'0x{address:08X}'] = {
            'extent': extent,
            'sha256': digest(pe.get_data(address - base, extent)) if extent else None}

    # Keep all file bytes outside explicit splits as independent COFF inputs.
    # BSS without file backing has no file fragment and never expands the EXE.
    boundaries = {0, len(data), pe.OPTIONAL_HEADER.SizeOfHeaders}
    for section in pe.sections:
        if section.SizeOfRawData:
            boundaries.update((section.PointerToRawData, section.PointerToRawData + section.SizeOfRawData))
    # A generated section padding (config.yml image_sources, Scripts/pe_generate.py `padding`: a section's zeros from
    # its virtual size to its raw size) is a fragment of its own even where the bytes before it are unowned too.
    for section in pe.sections:
        name = section.Name.rstrip(b'\0').decode()
        if 0 < section.Misc_VirtualSize < section.SizeOfRawData and any(
                Path(path).name == f'padding-{name}.bin' for path in project.image_sources.values()):
            boundaries.add(section.PointerToRawData + section.Misc_VirtualSize)
    for start, end in claimed:
        boundaries.update((start, end))
    ordered = sorted(boundaries)
    claimed = iter(sorted(claimed))
    current = next(claimed, None)
    sections = []
    path = project.build / 'orig/__preserved__.obj'
    for start, end in zip(ordered, ordered[1:]):
        if not 0 <= start < end <= len(data):
            raise ValueError('Invalid PE file layout')
        while current and current[1] <= start:
            current = next(claimed, None)
        if current and current[0] <= start < current[1]:
            continue
        section = next((s for s in pe.sections if s.PointerToRawData <= start
                        and end <= s.PointerToRawData + s.SizeOfRawData), None)
        name = section.Name.rstrip(b'\0').decode() if section else '.header'
        characteristics = (section.Characteristics & ~0x01F00000) | 0x100000 if section else coff.RDATA
        sections.append(coff.Section(name, data[start:end], characteristics))
        layout['fragments'].append({'object': path.as_posix(), 'section': len(sections),
                                    'file_offset': start, 'size': end - start, 'sha256': digest(data[start:end])})
    write_changed(project.root / path, coff.encode(sections, []))
    layout['fragments'].sort(key=lambda f: f['file_offset'])
    write_json(project.root / project.build / 'layout.json', layout)
    print(f'Split {len(project.functions)} functions and {len(project.data_symbols)} data symbols '
          f'into {len(project.units)} units / {len(project.splits)} ranges.')


def target(project, unit):
    """Create an objdiff reference with conservative, inferred relocations.

    Direct rel32 branches and references to known symbols/IAT entries are
    normalized. Data pointer fields also require a counterpart COFF relocation.
    Unknown operands stay untouched. Inferred relocations never certify completion.
    """
    from capstone import Cs, CS_ARCH_X86, CS_MODE_32, CS_GRP_CALL, CS_GRP_JUMP
    from capstone.x86 import X86_OP_IMM, X86_OP_MEM

    obj = coff.Object.read(project.root / project.obj('orig', unit))
    basepath = project.root / project.obj('obj', unit)
    build_config = json.loads((project.root / project.build / 'config.json').read_text())
    decorated = {}
    referenced = set()
    baseobj = None
    if unit in build_config['converted'] or unit in project.libraries:
        baseobj = coff.Object.read(basepath)
        from .pe_link import bind_local_data
        baseobj = bind_local_data(project, unit, baseobj)
        baseobj = coff.allocate_common(baseobj)
        decorated = {coff.undecorate(s.name): s.name for s in baseobj.symbols.values()}
        referenced = {baseobj.symbols[index].name for section in baseobj.sections
                      for _, index, _ in section.relocations}
        # Intel C/C++ 4.0 addresses its `.data1` literals through the section symbol plus an
        # offset, never through the pool labels (`$2$1_2il0floatpacket$N`): a section-symbol
        # reference reaches every named datum of that section.
        reached = {baseobj.symbols[index].section for section in baseobj.sections
                   for _, index, _ in section.relocations
                   if baseobj.symbols[index].name.startswith('.') and baseobj.symbols[index].section > 0
                   and not baseobj.sections[baseobj.symbols[index].section - 1].flags & 0x20}
        referenced |= {s.name for s in baseobj.symbols.values()
                       if s.section in reached and not s.name.startswith('.')}
        from .pe_objdiff import name_section_references
        name_section_references(baseobj)
    layout = json.loads((project.root / project.build / 'layout.json').read_text())
    addresses = {}
    # Prefer functions over aliases such as EXE_TEXT_START.
    for s in sorted(project.symbols.values(), key=lambda s: s['kind'] == 'function'):
        addresses[s['address']] = s.get('coff_name') or decorated.get(s['name'], '_' + s['name'])
    for name, va in layout['imports'].items():
        matches = [s for s in decorated.values() if s.startswith('__imp__')
                   and coff.undecorate(s.removeprefix('__imp_')) == name]
        addresses[va] = matches[0] if len(matches) == 1 else '__imp__' + name
    symbols = list(obj.symbols.values())
    literal_addresses = {s['address']: s for s in project.symbols.values()
                         if s['kind'] == 'data' and coff.is_string(s.get('coff_name', ''))}
    literal_references = {name: project.symbols[name]
                          for key, name in project.matching.get('shared', {}).get(unit, {}).items()
                          if coff.is_string(key)}
    explicit_names = {s['coff_name'] for s in project.units[unit] if s.get('coff_name')}
    for symbol in symbols:
        if symbol.name not in explicit_names:
            symbol.name = decorated.get(coff.undecorate(symbol.name), symbol.name)
    if len({s.name for s in symbols}) != len(symbols):
        raise ValueError(f'{unit}: duplicate target definitions after compiler decoration')
    indices = {s.name: i for i, s in enumerate(symbols)}
    decoder = Cs(CS_ARCH_X86, CS_MODE_32)
    decoder.detail = True
    data_ranges = sorted(project.data_symbols, key=lambda s: s['address'])
    data_starts = [s['address'] for s in data_ranges]

    def data_reference(value):
        if value in addresses and (value in literal_addresses or not decorated or addresses[value] in referenced):
            return addresses[value], 0
        index = bisect.bisect_right(data_starts, value) - 1
        if index >= 0:
            datum = data_ranges[index]
            name = addresses[datum['address']]
            if value < datum['address'] + datum['size'] and (not decorated or name in referenced):
                return name, value - datum['address']
        return None

    def add(section, payload, offset, name, typ, addend=0):
        if name not in indices:
            indices[name] = len(symbols)
            symbols.append(coff.Symbol(name))
        struct.pack_into('<I', payload, offset, addend & 0xFFFFFFFF)
        section.relocations.append((offset, indices[name], typ))

    native_symbols = {s.name: s for s in baseobj.symbols.values()} if baseobj else {}
    native_relocated = {(number, offset) for number, s in enumerate(baseobj.sections, 1)
                        for offset, _, _ in s.relocations} if baseobj else set()

    def native_direct(function, offset, callee):
        """True when the native object reaches `callee` by a resolved rel32 from the same section.

        An assembler object (MASM) calls a procedure of its own section with a resolved displacement
        and no relocation; the reference view must then keep retail's raw displacement at that site
        too, or objdiff compares a relocation against none although the bytes agree. Only the exact
        case qualifies: the native caller and callee are defined in one section and the native site
        carries no relocation. Presentation only: obj/ and the strict matcher are not involved."""
        caller = native_symbols.get(function.get('coff_name') or decorated.get(function['name'], '_' + function['name']))
        target_symbol = native_symbols.get(callee)
        if caller is None or caller.section <= 0:
            return False
        if (caller.section, caller.value + offset) in native_relocated:
            return False
        # A branch to an address no symbol names (an assembler label inside another procedure)
        # that the native object also encodes without a relocation is likewise a resolved
        # displacement on both sides: compare the raw bytes rather than a relocation against none.
        return callee.startswith('__va_') or (target_symbol is not None and target_symbol.section == caller.section)

    # The retail address each name a native relocation may use stands for (owned symbols, imports).
    retail_names = {}
    for s in project.symbols.values():
        retail_names.setdefault(s.get('coff_name') or decorated.get(s['name'], '_' + s['name']), set()).add(s['address'])
    for va, name in addresses.items():
        retail_names.setdefault(name, set()).add(va)

    def adopt_native_spelling(function, section, section_index, code, start, address, size):
        """Spell an inferred reference the way the native object does when both reach one address.

        An address has many spellings (`LeadUpVec-4` or the label at that address, `TrailUpVec+0x10`
        or `TrailUp0`, `_JonesHud_aCredits+0x80` under a base register, where only absolute operands
        are inferred), and objdiff compares a relocation by name and addend. For each native DIR32
        relocation inside the function, the destination it names is computed in retail terms (a
        label of the function's own section by its offset from the function, any other symbol by
        its retail address) and compared with retail's own operand at the same offset; only when
        they are equal is the reference view's relocation at that site spelled the native way.
        The destination is never taken from the candidate: a native reference to anything else
        leaves the inferred relocation, and the difference, in place; a site whose inferred
        relocation already carries the native addend is left as inferred too, so the reference only
        changes where the two spellings would otherwise compare unequal. Presentation only: obj/
        and the strict matcher are not involved."""
        caller = native_symbols.get(function.get('coff_name') or decorated.get(function['name'], '_' + function['name']))
        if caller is None or caller.section <= 0:
            return
        native_section = baseobj.sections[caller.section - 1]
        for offset, index, kind in native_section.relocations:
            relative = offset - caller.value
            symbol = baseobj.symbols[index]
            if kind != coff.DIR32 or relative < 0 or relative + 4 > size or symbol.name.startswith('.'):
                continue
            addend = struct.unpack_from('<I', native_section.data, offset)[0]
            if symbol.section == caller.section:
                destination = address + symbol.value - caller.value + addend
            elif len(retail_names.get(symbol.name, ())) == 1:
                destination = next(iter(retail_names[symbol.name])) + addend
            else:
                continue
            site = start + relative
            if struct.unpack_from('<I', section.data, site)[0] != destination & 0xFFFFFFFF:
                continue
            # An inferred relocation with the native addend already compares equal (by name, or by
            # address for a label of the function's own section); the reference is left as inferred.
            current = [r for r in section.relocations if r[0] == site]
            if len(current) == 1 and current[0][2] == kind and struct.unpack_from('<I', code, site)[0] == addend:
                continue
            if symbol.name not in indices and symbol.section == caller.section:
                indices[symbol.name] = len(symbols)
                symbols.append(coff.Symbol(symbol.name, start + symbol.value - caller.value,
                                           section_index, storage=6))
            section.relocations = [r for r in section.relocations if r[0] != site]
            add(section, code, site, symbol.name, kind, addend)

    # A vendor unit's member is the retail bytes themselves, with no relocations (pe_library): both views
    # keep the raw operands, so no relocation is inferred on the reference side either.
    from .pe_library import is_vendor
    libraries = getattr(project, 'libraries', {})
    vendor = unit in libraries and is_vendor(libraries[unit])
    for function in (s for s in project.units[unit] if s['kind'] == 'function' and not vendor):
        symbol = obj.function(coff.undecorate(function.get('coff_name') or function['name']))
        section = obj.sections[symbol.section - 1]
        address, size, start = function['address'], function['size'], symbol.value
        code = bytearray(section.data)
        instructions = list(decoder.disasm(section.data[start:start + size], address))
        instruction_addresses = {insn.address for insn in instructions}

        def local_reference(value):
            if not address <= value < address + size:
                return None
            # Original addresses determine local labels, independently of the
            # candidate's compiler-generated $L serials and relocation addends.
            name = f'$L_{value:08X}'
            if name not in indices:
                indices[name] = len(symbols)
                symbols.append(coff.Symbol(name, start + value - address, symbol.section, storage=6))
            return name, 0

        jump_tables = set()
        for insn in instructions:
            if insn.mnemonic == 'jmp' and insn.operands[0].type == X86_OP_MEM:
                mem = insn.operands[0].mem
                if not mem.base and mem.index and mem.scale == 4 and address <= mem.disp < address + size:
                    jump_tables.add(mem.disp)
        for table in sorted(jump_tables):
            # Read consecutive original case pointers, stopping at the first
            # non-code address. Never borrow candidate table entries or bounds.
            cursor = table
            # A table ends where the next one begins (VC5 places a switch's tables back to back);
            # reading on would relocate the next table's entries twice, and objdiff, which treats
            # the bytes between a relocation and the next as a byte (index) table, would then see
            # no room for the byte table that follows.
            while cursor + 4 <= address + size and (cursor == table or cursor not in jump_tables):
                offset = start + cursor - address
                destination = struct.unpack_from('<I', section.data, offset)[0]
                if destination not in instruction_addresses or destination >= table:
                    break
                name, addend = local_reference(destination)
                add(section, code, offset, name, coff.DIR32, addend)
                cursor += 4
        for insn in instructions:
            for operand in insn.operands:
                if operand.type == X86_OP_IMM and operand.imm in literal_addresses:
                    datum = literal_addresses[operand.imm]
                    literal_references[datum['name']] = datum
            branch = insn.group(CS_GRP_CALL) or insn.group(CS_GRP_JUMP)
            if branch and insn.imm_size == 4 and insn.operands[0].type == X86_OP_IMM:
                destination = insn.operands[0].imm & 0xFFFFFFFF
                if not address <= destination < address + size:
                    name = addresses.get(destination, f'__va_{destination:08X}')
                    if not native_direct(function, insn.address - address + insn.imm_offset, name):
                        add(section, code, start + insn.address - address + insn.imm_offset, name, coff.REL32)
            elif not branch and insn.imm_size == 4 and any(op.type == X86_OP_IMM for op in insn.operands):
                offset = start + insn.address - address + insn.imm_offset
                value = struct.unpack_from('<I', section.data, offset)[0]
                # An immediate 0 is a constant, never the address of the absolute symbol
                # `__except_list` (the FS:[0] SEH chain offset, value 0).
                reference = data_reference(value) if value else None
                if reference:
                    add(section, code, offset, reference[0], coff.DIR32, reference[1])
            if insn.disp_size == 4 and any(op.type == X86_OP_MEM for op in insn.operands):
                offset = start + insn.address - address + insn.disp_offset
                value = struct.unpack_from('<I', section.data, offset)[0]
                reference = local_reference(value)
                if not reference and any(op.type == X86_OP_MEM and op.mem.base == 0 for op in insn.operands):
                    reference = data_reference(value)
                if reference:
                    add(section, code, offset, reference[0], coff.DIR32, reference[1])
        if baseobj is not None:
            adopt_native_spelling(function, section, symbol.section, code, start, address, size)
        section.data = bytes(code)

    # Normalize data only at fields identified as pointers by a counterpart
    # COFF relocation, and only when the original destination has a name.
    # The original bytes determine the destination; no data is copied from C.
    if decorated and not vendor:
        base_symbols = {s.name: s for s in baseobj.symbols.values() if s.section > 0}
        target_symbols = {s.name: s for s in obj.symbols.values() if s.section > 0}
        for datum in project.units[unit]:
            if datum['kind'] != 'data':
                continue
            name = datum.get('coff_name') or decorated.get(datum['name'], '_' + datum['name'])
            current, expected = base_symbols.get(name), target_symbols.get(name)
            if not current or not expected:
                continue
            base_section = baseobj.sections[current.section - 1]
            section = obj.sections[expected.section - 1]
            if section.is_bss or base_section.is_bss:
                continue
            # A retail array can differ in size from its C counterpart. Do not
            # borrow pointer fields from the next compiled global in that case.
            end = min((s.value for s in base_symbols.values()
                       if s.section == current.section and s.value > current.value),
                      default=base_section.size)
            compared_size = min(datum['size'], end - current.value)
            payload = bytearray(section.data)
            for offset, _, typ in base_section.relocations:
                relative = offset - current.value
                if typ != coff.DIR32 or relative < 0 or relative + 4 > compared_size:
                    continue
                target_offset = expected.value + relative
                value = struct.unpack_from('<I', payload, target_offset)[0]
                addend = struct.unpack_from('<I', base_section.data, offset)[0]
                destination = (value - addend) & 0xFFFFFFFF
                if destination in addresses:
                    add(section, payload, target_offset, addresses[destination], coff.DIR32, addend)
                    if destination in literal_addresses:
                        datum = literal_addresses[destination]
                        literal_references[datum['name']] = datum
            section.data = bytes(payload)
    # Neither objdiff view owns the alignment fill between functions: a
    # function's extent is its code length on both sides. The untouched
    # compiler object in obj/ still feeds the strict linker and its checks.
    from .pe_objdiff import strip_native_fill, strip_reference_fill
    reference_fill = strip_reference_fill(project, unit, obj, symbols)
    if baseobj is None:
        expected = coff.encode(obj.sections, symbols)
    else:
        from .pe_objdiff import comparisons, demote_entry_aliases, demote_unnamed_aliases
        demote_entry_aliases(baseobj)
        demote_unnamed_aliases(baseobj, {s.name for s in symbols if s.section > 0})
        native_fill = strip_native_fill(project, unit, baseobj)
        expected, current, report = comparisons(project, unit, obj, symbols, baseobj, layout,
                                                literal_references.values())
        report['code_fill_bytes'] = {'reference': reference_fill, 'native': native_fill}
        write_changed(project.root / project.obj('compare', unit), current)
        write_json((project.root / project.obj('compare', unit)).with_suffix('.json'), report)
    write_changed(project.root / project.obj('target', unit), expected)


def libextract(project, unit):
    """Write a library unit's member, unmodified, where a compiled TU's object goes.

    A vendor unit's member is its split object from the pinned retail image (pe_library)."""
    from .pe_library import extract
    if unit not in project.libraries:
        raise ValueError(f'{unit}: not a library unit in libraries.yml')
    write_changed(project.root / project.obj('obj', unit),
                  extract(project.root, unit, project.libraries[unit], project.root / project.obj('orig', unit),
                          [part['lead'] for part in project.unit_splits[unit]]))


def verify(project):
    stamp = project.root / project.build / 'verify.ok'
    stamp.unlink(missing_ok=True)
    digest = checksum(project.root / project.build / 'Indy3D.exe', project.config['sha256'])
    sha1 = hashlib.sha1((project.root / project.build / 'Indy3D.exe').read_bytes()).hexdigest()
    if sha1 != project.config['sha1']:
        raise ValueError('Rebuilt image SHA-1 differs from config.yml / build.sha1')
    write_changed(stamp, digest + '\n')
    print(f'Indy3D.exe: SHA-256 OK ({digest})')


def map_image(project):
    """A fixed-address ownership map, including every original fallback range."""
    from .pe_image import load_layout
    checksum(project.root / project.build / 'Indy3D.exe', project.config['sha256'])
    layout = load_layout(project)
    report = json.loads((project.root / project.build / 'link.json').read_text())
    selected = {u['source'] for u in report['source_units']}
    libraries = {u['source'] for u in report.get('library_units', [])}
    vendors = {u['source'] for u in report.get('library_units', []) if u.get('vendor')}
    lines = ['Indy3D PE ownership map', 'Address   Size      Section  Provider  Translation unit']
    for part in sorted(layout['ranges'], key=lambda p: p['address']):
        provider = ('source' if part['source'] in selected else 'vendor' if part['source'] in vendors else
                    'library' if part['source'] in libraries else 'original')
        lines.append(f'{part["address"]:08X}  {part["size"]:08X}  {part["kind"]:8} {provider:8}  {part["source"]}')
    lines += ['', 'File offset  Size      Provider  Preserved image fragment']
    for part in layout['fragments']:
        if not part.get('source'):
            provider = project.image_sources.get((part['file_offset'], part['size']))
            assets = project.image_assets.get((part['file_offset'], part['size']))
            lines.append(f'{part["file_offset"]:08X}     {part["size"]:08X}  '
                         f'{"source" if provider else "original":8}  {provider or part["object"]}'
                         + (f' (embeds user-supplied assets, {assets})' if assets else ''))
    lines += ['', 'Address   Size      Symbol']
    lines += [f'{s["address"]:08X}  {s["size"]:08X}  {s["name"]}'
              for s in sorted(project.symbols.values(), key=lambda s: s['address'])]
    write_changed(project.root / project.build / 'Indy3D.map', '\n'.join(lines) + '\n')


def freeze_layout(project):
    """Copy the current reference layout (geometry and hashes, no original payload) to the config's
    pe.source_layout, the layout `ninja source-only` reads instead of anything extracted from the original."""
    from .pe_image import load_layout
    if not project.config.get('source_layout'):
        raise ValueError('config.yml has no pe.source_layout to freeze into')
    load_layout(project)  # refuses a stale layout
    data = (project.root / project.build / 'layout.json').read_text()
    write_changed(project.source_only_layout, data)
    print(f'Froze {project.relative(project.source_only_layout)} ({len(data):,} bytes).')


def progress(project):
    checksum(project.root / project.build / 'Indy3D.exe', project.config['sha256'])
    result = json.loads((project.root / project.build / 'link.json').read_text())
    config = json.loads((project.root / project.build / 'config.json').read_text())
    result.update({'version': project.version, 'compiled_units': len(config['converted']),
                   'total_units': len(config['sources']), 'known_functions': len(project.functions),
                   'reviewed_functions': sum(s['bounds'] == 'reviewed' for s in project.functions),
                   'matched_units': len(result['source_units']),
                   'matched_functions': sum(u['functions'] for u in result['source_units']),
                   'known_function_bytes': sum(s['size'] for s in project.functions)})
    # Ownership coverage is separate from verified contribution. In particular,
    # zero-filled BSS and byte-equal literals do not certify source completion.
    result['split_coverage'] = {}
    for kind in SECTION_KINDS:
        parts = [p for p in project.splits if p['section'] == kind and p['source'] != UNASSIGNED]
        result['split_coverage'][kind] = {
            'ranges': len(parts), 'units': len({p['source'] for p in parts}),
            'bytes': sum(p['end'] - p['start'] for p in parts),
        }
    result['data_symbols'] = len(project.data_symbols)
    result['reviewed_data_symbols'] = sum(s['bounds'] == 'reviewed' for s in project.data_symbols)
    write_json(project.root / project.build / 'progress.json', result)
    print(f'Compiled: {result["compiled_units"]}/{result["total_units"]} units; '
          f'verified complete TUs: {result["matched_units"]}/{result["total_units"]} '
          f'({result["source_bytes"]:,} file bytes; {result["source_bss_bytes"]:,} BSS bytes); '
          f'library members: {len(result["library_units"])} ({result["library_bytes"]:,} file bytes, of which '
          f'{sum(bool(u.get("vendor")) for u in result["library_units"])} vendor members from the retail image: '
          f'{result.get("vendor_bytes", 0):,}); '
          f'user-supplied assets (not counted): {result.get("user_asset_bytes", 0):,} file bytes.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--version', default=DEFAULT_VERSION)
    parser.add_argument('--build-dir', type=Path, default=Path('build'))
    parser.add_argument('command', choices=('split', 'target', 'match', 'libextract', 'link', 'verify',
                                            'progress', 'map', 'freeze-layout'))
    parser.add_argument('--unit')
    parser.add_argument('--source-only', action='store_true', help='link without any original-byte fallback')
    args = parser.parse_args()
    try:
        project = Project(args.version, build_dir=args.build_dir)
        if args.source_only and args.command != 'link':
            parser.error('--source-only applies only to link')
        if args.command in ('target', 'match', 'libextract'):
            if args.unit not in project.units:
                parser.error('--unit must name a source in the split configuration')
            globals()[args.command](project, args.unit)
        elif args.command == 'link':
            link(project, source_only=args.source_only)
        elif args.command == 'map':
            map_image(project)
        elif args.command == 'freeze-layout':
            freeze_layout(project)
        else:
            globals()[args.command](project)
    except (ValueError, OSError, KeyError) as exc:
        parser.exit(1, f'error: {exc}\n')


if __name__ == '__main__':
    main()
