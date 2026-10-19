"""Automatic, symmetric objdiff presentation of VC6 string contributions.

String COMDAT order is normalized by original address on BOTH sides. Expected
contents, sizes and references come from the original; native bytes are never
repaired. Unknown data and extra/missing strings remain in the comparison.
The linker always consumes the untouched compiler object.
"""
from dataclasses import dataclass

from . import coff
from .pe_link import align_up, digest, entry_alias_base


@dataclass
class Fragment:
    section: coff.Section
    index: int | None
    start: int = 0
    literal: dict | None = None
    key: str = ''

    def order(self):
        if not self.key:
            return (0, self.index, self.start)
        address = self.literal['address'] if self.literal else 0x100000000
        return (1, self.section.name, address, self.key)


def code_extents(project, unit):
    """Recorded retail code length and range fill byte for each owned function.

    A function's extent is its *code* length -- the bytes before the
    inter-function alignment fill -- which equals the native `/Gy` COMDAT's
    code. The fill itself is inserted by the linker between COMDATs (or by the
    compiler after the COMDAT's last instruction), so it belongs to no
    function; see docs/decompilation.md.
    """
    extents = {}
    for part in project.unit_splits[unit]:
        if part['section'] != '.text':
            continue
        for datum in project.units[unit]:
            if datum['kind'] == 'function' and part['start'] <= datum['address'] < part['end']:
                name = datum.get('coff_name') or '_' + datum['name']
                extents[coff.undecorate(name)] = (datum['size'], part['padding'])
    return extents


def compact(section, index, symbols, gaps):
    """Remove byte ranges from a section, moving its symbols and relocations."""
    if not gaps:
        return 0
    kept, cursor = bytearray(), 0
    for start, end in gaps:
        if not cursor <= start < end <= section.size:
            raise ValueError('Invalid alignment-fill range')
        kept += section.data[cursor:start]
        cursor = end
    kept += section.data[cursor:section.size]

    def remap(offset):
        removed = 0
        for start, end in gaps:
            if offset >= end:
                removed += end - start
            elif offset > start:
                raise ValueError('Offset inside removed alignment fill')
        return offset - removed

    section.relocations = [(remap(offset), i, kind) for offset, i, kind in section.relocations]
    for symbol in symbols:
        if symbol.section == index:
            symbol.value = remap(symbol.value)
    dropped = section.size - len(kept)
    section.data, section.size = bytes(kept), len(kept)
    return dropped


def strip_reference_fill(project, unit, obj, symbols):
    """Drop inter-function alignment fill from the objdiff reference view.

    `split` emits one section per `.text` range, so objdiff -- which has no
    COFF symbol sizes to read -- would size a function as "the next symbol's
    address minus this one" and charge the fill that follows a function to the
    function itself. Only a run that starts exactly at a function's recorded
    extent, consists solely of the range's declared fill byte and carries no
    relocation is removed, so fill bytes inside a function's own code (an
    immediate, a jump table or string data) can never be mistaken for it.
    """
    parts = project.unit_splits[unit]
    if len(parts) != len(obj.sections):
        raise ValueError(f'{unit}: reference object does not have one section per split')
    dropped = 0
    for index, (part, section) in enumerate(zip(parts, obj.sections), 1):
        if part['section'] != '.text' or section.is_bss:
            continue
        starts = sorted({s.value for s in symbols if s.section == index})
        gaps = []
        # Library members can place data (DirectInput's format tables) in
        # .text; their fill is the linker's, exactly as between functions.
        for datum in sorted((d for d in project.units[unit] if d['kind'] in ('function', 'data')
                             and part['start'] <= d['address'] < part['end']),
                            key=lambda d: d['address']):
            end = datum['address'] - part['start'] + datum['size']
            limit = min([v for v in starts if v >= end] + [section.size])
            if (end < limit and set(section.data[end:limit]) == {part['padding']}
                    and not any(offset + 4 > end and offset < limit
                                for offset, _, _ in section.relocations)):
                gaps.append((end, limit))
        dropped += compact(section, index, symbols, gaps)
    return dropped


def demote_entry_aliases(obj):
    """Objdiff view only: make Intel internal code labels (entry, `__try` handler/filter, `__asm` labels) plain labels (like `$L` labels).

    objdiff sizes a function as the distance to the next function symbol, so the alias
    would cut every ICL function in two and score the first half against all of retail's.
    The strict linker reads the untouched object and never sees this change.
    """
    for symbol in obj.symbols.values():
        if entry_alias_base(obj, symbol):
            symbol.type, symbol.storage = 0, 6


def demote_unnamed_aliases(obj, reference_names):
    """Objdiff view only: make a second native name for a reference-named code address a plain label.

    An archive member can export one routine under two names (`chkstk.obj`'s `__chkstk` and
    `__alloca_probe`, `stricmp.obj`'s `__stricmp` and `__strcmpi`, 87tran's `__fFEXP` and its
    static `fFEXP`), where the retail symbol map names it once. objdiff sizes the first of two
    symbols at one address as zero bytes and pairs the reference's name with that empty symbol,
    so the routine scores nothing although its bytes are identical. A native code symbol that
    the reference view does not name, at the exact section offset of one that it does, is
    therefore demoted to a label (as `demote_entry_aliases` does for Intel's internal entries).
    Nothing else moves; the strict linker reads the untouched object in obj/.
    """
    named = {(s.section, s.value) for s in obj.symbols.values()
             if s.section > 0 and s.name in reference_names and not s.name.startswith('.')}
    demoted = 0
    for symbol in obj.symbols.values():
        if (symbol.section > 0 and (symbol.section, symbol.value) in named and symbol.storage in (2, 3)
                and symbol.name not in reference_names and not symbol.name.startswith('.')
                and obj.sections[symbol.section - 1].flags & 0x20):
            symbol.type, symbol.storage = 0, 6
            demoted += 1
    return demoted


def name_section_references(obj):
    """Objdiff view only: spell a native `section symbol + offset` data reference by its named datum.

    Intel C/C++ 4.0 addresses its `.bss`/`.data1` objects through the section symbol plus an
    in-place offset (`.bss+0x3898`), where the reference view names the datum
    (`_rdModel3K_bufB+0`). objdiff resolves the section symbol of an instruction's relocation
    to the named symbol itself, but it reads only one relocation per instruction: a second
    one (`mov [gB], offset bufB`, an address stored into an address) is compared as its raw
    in-place bytes, 0 against 0x3898, although both name the same object. Each such
    relocation is rewritten to the named symbol objdiff's own resolution picks (the
    highest-addressed named symbol of that section at or below the target, the lowest symbol
    index on a tie) with the in-place offset reduced to match, so both views spell every such
    reference alike. Only DIR32 references to data/BSS section symbols change and the
    destination is preserved, so a reference to the wrong datum still differs. Returns the
    number of relocations rewritten. The strict linker reads the untouched object in obj/.
    """
    renamed = 0
    for section in obj.sections:
        if section.is_bss or not section.relocations:
            continue
        data, relocations = bytearray(section.data), []
        for offset, index, kind in section.relocations:
            symbol = obj.symbols.get(index)
            if (kind == coff.DIR32 and symbol is not None and symbol.name.startswith('.')
                    and symbol.storage == 3 and symbol.section > 0 and not symbol.value
                    and not obj.sections[symbol.section - 1].flags & 0x20):
                target = int.from_bytes(data[offset:offset + 4], 'little')
                named = [(s.value, i) for i, s in obj.symbols.items()
                         if s.section == symbol.section and not s.name.startswith('.')
                         and s.value <= target]
                if named:
                    value, index = max(named, key=lambda item: (item[0], -item[1]))
                    data[offset:offset + 4] = (target - value).to_bytes(4, 'little')
                    renamed += 1
            relocations.append((offset, index, kind))
        section.data, section.relocations = bytes(data), relocations
    return renamed


def strip_native_fill(project, unit, obj):
    """Drop a compiled code COMDAT's trailing fill past the retail extent.

    VC pads a `/Gy` code COMDAT to its alignment with the same fill byte the
    linker uses between COMDATs. Removing exactly the surplus over the retail
    extent keeps both objdiff views on the same function boundary; a compiled
    body that is a different length, or whose surplus is not pure fill, is a
    real difference and stays visible.
    """
    extents = code_extents(project, unit)
    dropped = 0
    for index, section in enumerate(obj.sections, 1):
        if section.name.split('$', 1)[0] != '.text' or section.is_bss:
            continue
        defined = [s for s in obj.symbols.values() if s.section == index and s.type & 0x20]
        if len(defined) != 1 or defined[0].value:
            continue
        extent, padding = extents.get(coff.undecorate(defined[0].name), (None, None))
        if extent is None or not 0 < extent < section.size:
            continue
        if (set(section.data[extent:section.size]) != {padding}
                or any(offset + 4 > extent for offset, _, _ in section.relocations)):
            continue
        dropped += section.size - extent
        section.data, section.size = section.data[:extent], extent
    return dropped


def slice_section(section, start, end):
    relocs = []
    for offset, index, kind in section.relocations:
        if start <= offset < end:
            if offset + 4 > end:
                raise ValueError('Relocation crosses an objdiff data boundary')
            relocs.append((offset - start, index, kind))
    return coff.Section(section.name, section.data[start:end], section.flags, relocs,
                        size=end - start)


def literal_matches(section, datum, layout):
    """BSS has independently verified zero storage, not a payload hash."""
    if datum['section'] == '.bss':
        return section.size == datum['size'] and not any(section.data)
    return digest(section.data) == layout['symbols'][datum['name']]['sha256']


def literal_section(datum, payload):
    flags = {'.data': coff.DATA, '.data1': coff.DATA, '.rdata': coff.RDATA, '.bss': coff.BSS}
    return coff.Section(datum['section'], payload, flags[datum['section']], size=datum['size'])


def assemble(fragments, symbols):
    fragments.sort(key=Fragment.order)
    for symbol in symbols:
        if symbol.section <= 0:
            continue
        candidates = [(i, f) for i, f in enumerate(fragments, 1)
                      if f.index == symbol.section and f.start <= symbol.value
                      and (symbol.value < f.start + f.section.size
                           or (symbol.value == f.start and not f.section.size))]
        # Real section symbols can describe an empty section or its end.
        if not candidates:
            candidates = [(i, f) for i, f in enumerate(fragments, 1)
                          if f.index == symbol.section and symbol.value == f.start + f.section.size]
        if len(candidates) != 1:
            raise ValueError(f'Unaccounted objdiff symbol: {symbol.name}')
        index, fragment = candidates[0]
        symbol.section, symbol.value = index, symbol.value - fragment.start
    definitions = {s.name: s for s in symbols}
    for index, fragment in enumerate(fragments, 1):
        if fragment.index is None:
            symbol = definitions.get(fragment.key)
            if symbol is None:
                symbol = coff.Symbol(fragment.key)
                symbols.append(symbol)
                definitions[fragment.key] = symbol
            symbol.section, symbol.value = index, 0
    return coff.encode([f.section for f in fragments], symbols)


def reference_payload(project, datum, layout, cache):
    owner = datum['source']
    if owner not in cache:
        cache[owner] = coff.Object.read(project.root / project.obj('orig', owner))
    obj = cache[owner]
    matches = [s for s in obj.symbols.values() if s.section > 0 and s.name == datum['coff_name']]
    if len(matches) != 1:
        raise ValueError(f'Missing or ambiguous original string: {datum["name"]}')
    symbol = matches[0]
    section = obj.sections[symbol.section - 1]
    end = symbol.value + datum['size']
    if (section.name != datum['section'] or end > section.size
            or not literal_matches(slice_section(section, symbol.value, end), datum, layout)):
        raise ValueError(f'Original string bytes differ: {datum["name"]}')
    return section.data[symbol.value:end]


def comparisons(project, unit, target, symbols, native, layout, references):
    by_key = {}
    for datum in project.symbols.values():
        if datum['kind'] == 'data' and coff.is_string(datum.get('coff_name', '')):
            by_key.setdefault(datum['coff_name'], []).append(datum)
    expected = {s['name']: s for s in project.units[unit] if coff.is_string(s.get('coff_name', ''))}
    expected.update({s['name']: s for s in references})
    fragments, padding, extracted = [], 0, set()
    for index, section in enumerate(target.sections, 1):
        if section.name not in ('.data', '.data1', '.rdata', '.bss'):
            fragments.append(Fragment(section, index))
            continue
        members = sorted((s for s in symbols if s.section == index), key=lambda s: s.value)
        cursor = 0
        for symbol in members:
            candidates = [s for s in expected.values() if s['source'] == unit and s['coff_name'] == symbol.name]
            if len(candidates) != 1:
                continue
            datum = candidates[0]
            start, end = symbol.value, symbol.value + datum['size']
            next_start = min((s.value for s in members if s.value > start), default=section.size)
            # Ambiguous or overlapping extents stay in the ordinary data view.
            if (end > next_start or start < cursor or datum['address'] % 4
                    or any(start <= r[0] < end for r in section.relocations)):
                continue
            payload = section.data[start:end]
            if not literal_matches(slice_section(section, start, end), datum, layout):
                raise ValueError(f'Original string bytes differ: {datum["name"]}')
            if start > cursor:
                fragments.append(Fragment(slice_section(section, cursor, start), index, cursor))
            fragments.append(Fragment(literal_section(datum, payload), index, start, datum, symbol.name))
            extracted.add(datum['name'])
            cursor = end
            pad_end = min(align_up(end, 4), section.size)
            # Only natural zero alignment, with no labels or relocations, is
            # reproduced by objdiff's section combiner rather than stored twice.
            if (pad_end <= next_start and not any(section.data[end:pad_end])
                    and not any(end <= r[0] < pad_end for r in section.relocations)):
                padding += pad_end - end
                cursor = pad_end
        if cursor < section.size or not section.size:
            fragments.append(Fragment(slice_section(section, cursor, section.size), index, cursor))

    cache = {}
    for datum in expected.values():
        if datum['source'] == unit or not datum['source']:
            continue
        payload = reference_payload(project, datum, layout, cache)
        fragments.append(Fragment(literal_section(datum, payload), None,
                                  literal=datum, key=datum['coff_name']))
        extracted.add(datum['name'])

    # Native COFF has auxiliary symbol slots. Rebase relocation indices when
    # serializing the comparison copy; the original file is never rewritten.
    native_symbols = list(native.symbols.values())
    symbol_indices = {old: new for new, old in enumerate(native.symbols)}
    current, recognized, unknown, ambiguous = [], set(), [], []
    changed, storage_mismatches = [], []
    for index, section in enumerate(native.sections, 1):
        section.relocations = [(offset, symbol_indices[symbol], kind) for offset, symbol, kind in section.relocations]
        definitions = [s for s in native_symbols if s.section == index and not s.name.startswith('.')]
        key, datum = '', None
        if (section.name in ('.data', '.data1', '.rdata', '.bss') and section.flags & 0x1000
                and not section.relocations and len(definitions) == 1
                and definitions[0].value == 0 and coff.is_string(definitions[0].name)):
            key = definitions[0].name
            candidates = by_key.get(key, [])
            if len(candidates) == 1:
                datum = candidates[0]
                recognized.add(datum['name'])
                if not literal_matches(section, datum, layout):
                    changed.append(datum['name'])
                if section.name != datum['section']:
                    storage_mismatches.append(datum['name'])
            elif candidates:
                ambiguous.append(key)
            else:
                unknown.append(key)
        current.append(Fragment(section, index, literal=datum, key=key))

    report = {
        'source': unit, 'native_object': project.obj('obj', unit).as_posix(),
        'string_order': 'original address on both sides; unknown native strings follow known strings',
        'expected_strings': len(expected), 'shared_strings': sum(s['source'] != unit for s in expected.values()),
        'normalized_strings': len(extracted), 'alignment_bytes': padding,
        'missing_strings': sorted(set(expected) - recognized),
        'extra_strings': sorted(recognized - set(expected)),
        'unknown_strings': sorted(unknown), 'ambiguous_strings': sorted(ambiguous),
        'changed_strings': sorted(changed), 'storage_mismatches': sorted(storage_mismatches),
    }
    return assemble(fragments, symbols), assemble(current, native_symbols), report
