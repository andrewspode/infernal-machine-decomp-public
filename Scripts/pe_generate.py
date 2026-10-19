"""Generate the image fragments that no translation unit owns: PE headers, the import table and
the resource section.

Each generator reads a checked-in description under ``config/<version>/image/`` (plus pinned
toolchain inputs such as import libraries) and writes one complete preserved file fragment.
``config.yml`` names the outputs in ``pe.image_sources``; the image builder accepts them only when
their size and SHA-256 equal the fragment they replace, exactly as for a compiled TU.  Nothing
here reads the original executable.

  python -m Scripts.pe_generate header  --version indy3d-v1.0 --output <file>
  python -m Scripts.pe_generate imports --version indy3d-v1.0 --fragment address_table --output <file>
  python -m Scripts.pe_generate thunks    --version indy3d-v1.0 --address 0x004E09F0 --output <file>
  python -m Scripts.pe_generate padding   --version indy3d-v1.0 --section _TEXT16 --output <file>
  python -m Scripts.pe_generate resources --version indy3d-v1.0 --icons <dir> --assets <json> --output <file>

The resource section is compiled from ``Jones3D/Indy3D.rc``; its icons are the one input that is
not source: a *user-supplied asset* extracted from the user's own executable at build time
(``Scripts/pe_rsrc.py --extract-icons``), whose bytes ``--assets`` records so they are reported
apart and kept out of every progress figure.
"""
import argparse
import hashlib
from pathlib import Path
import re
import struct

from tools.config import read_yaml
from .pe_library import archive_members
from .pe_project import version_file
from . import pe_rsrc

ROOT = Path(__file__).resolve().parent.parent

# LINK's built-in MS-DOS header and real-mode stub (64 + 64 bytes), placed before every image it
# writes: a 3-page header with e_lfarlc 0x40, and a stub printing the message and exiting.
DOS_HEADER = struct.pack('<2s13H8sHH20sI', b'MZ', 0x90, 3, 0, 4, 0, 0xFFFF, 0, 0xB8, 0, 0, 0, 0x40, 0,
                         bytes(8), 0, 0, bytes(20), 0)  # e_lfanew is filled in
DOS_STUB = (bytes.fromhex('0e1fba0e00b409cd21b8014ccd21') + b'This program cannot be run in DOS mode.\r\r\n$')
DOS_STUB += bytes(64 - len(DOS_STUB))


def align(value, alignment):
    return (value + alignment - 1) // alignment * alignment


def rol(value, count):
    count %= 32
    return ((value << count) | (value >> (32 - count))) & 0xFFFFFFFF


def rich_record(entries, stub_end):
    """The "Rich" record of build tools.  LINK 5.10's key is the record's file offset plus each
    entry's tool id rotated left by its count (no DOS-header term, unlike later linkers); zero fill
    after the record puts the PE signature on a 16-byte boundary."""
    key = stub_end
    for product, build, count in entries:
        key = (key + rol(product << 16 | build, count)) & 0xFFFFFFFF
    words = [0x536E6144 ^ key, key, key, key]  # "DanS" and three zero words
    for product, build, count in entries:
        words += [(product << 16 | build) ^ key, count ^ key]
    record = struct.pack(f'<{len(words)}I', *words) + b'Rich' + struct.pack('<I', key)
    return record + bytes(align(stub_end + len(record), 16) - stub_end - len(record))


def image_dir(version):
    return ROOT / 'config' / version / 'image'


def import_member(payload):
    """Sections {name: (data, relocation count)} and symbol names of one import-library member.
    The descriptor member of the VS97 libraries carries an optional header, so this reads the
    COFF tables directly instead of through coff.Object."""
    machine, count, _, symptr, symcount, optional, _ = struct.unpack_from('<HHIIIHH', payload)
    if machine != 0x14C:
        raise ValueError('import library member is not i386 COFF')
    sections = {}
    for i in range(count):
        name, _, _, size, raw, _, _, relcount, _, _ = struct.unpack_from('<8sIIIIIIHHI', payload, 20 + optional + 40 * i)
        sections[name.rstrip(b'\0').decode('ascii')] = (payload[raw:raw + size], relcount)
    strings = symptr + 18 * symcount
    names, i = [], 0
    while i < symcount:
        name, _, section, _, _, aux = struct.unpack_from('<8sIhHBB', payload, symptr + 18 * i)
        if name[:4] == bytes(4):
            start = strings + struct.unpack_from('<I', name, 4)[0]
            name = payload[start:payload.index(b'\0', start)]
        names.append((name.rstrip(b'\0').decode('latin-1'), section))
        i += aux + 1
    return sections, names


def import_libraries(spec):
    """{library: {'dll': name bytes, 'imports': {function: (thunk value or None, hint/name bytes)}}}"""
    libraries = {}
    for key, entry in spec['libraries'].items():
        data = (ROOT / entry['path']).read_bytes()
        if hashlib.sha256(data).hexdigest() != entry['sha256']:
            raise ValueError(f'{entry["path"]} is not the pinned import library (SHA-256 mismatch)')
        dll, imports = None, {}
        for _, payload in archive_members(data):
            sections, names = import_member(payload)
            if '.idata$2' in sections:
                if dll is not None:
                    raise ValueError(f'{entry["path"]}: two import descriptor members')
                dll = sections['.idata$6'][0]
                continue
            imp = [n for n, section in names if n.startswith('__imp_') and section > 0]
            if len(imp) != 1 or '.idata$5' not in sections:
                continue
            function = re.sub(r'@\d+$', '', imp[0][len('__imp_'):])
            function = function[1:] if function.startswith('_') else function
            thunk, relocations = sections['.idata$5']
            value = None if relocations else struct.unpack('<I', thunk)[0]
            hint_name = sections['.idata$6'][0] if '.idata$6' in sections else None
            if function in imports:
                raise ValueError(f'{entry["path"]}: duplicate import {function}')
            imports[function] = (value, hint_name)
        if dll is None:
            raise ValueError(f'{entry["path"]}: no import descriptor member')
        libraries[key] = {'dll': dll, 'imports': imports}
    return libraries


def import_tables(version):
    """{fragment name: (virtual address, bytes)} for the import address table and directory."""
    spec = read_yaml(image_dir(version) / 'imports.yml')
    base = read_yaml(image_dir(version) / 'header.yml')['optional_header']['image_base']
    libraries = import_libraries(spec)
    fragments = spec['fragments']
    if not spec['thunks'] or not set(spec['thunks']) == set(spec['descriptors']) == set(libraries):
        raise ValueError('imports.yml: thunks, descriptors and libraries must name the same libraries')

    def lookup(library, function):
        imports = libraries[library]['imports']
        if function.startswith('#'):
            matches = [v for v, _ in imports.values() if v == 0x80000000 | int(function[1:])]
            if len(matches) != 1:
                raise ValueError(f'{library}: no unique import by ordinal {function}')
            return matches[0], None
        if function not in imports or imports[function][1] is None:
            raise ValueError(f'{library}: {function} is not imported by name')
        return imports[function]

    # The lookup tables follow the descriptors and their null terminator; the hint/name table
    # follows the lookup tables.  Both thunk arrays share one block order.
    table = fragments['address_table']['address']
    directory = spec['directory']
    lookups = directory + 20 * (len(spec['descriptors']) + 1)
    blocks, offset = {}, 0
    for library, functions in spec['thunks'].items():
        blocks[library] = offset
        offset += 4 * (len(functions) + 1)
    names_start = lookups + offset
    names, position, name_rva = bytearray(), names_start, {}
    for item in spec['hint_names']:
        library, _, function = item.partition(' ')
        payload = lookup(library, function)[1] if function else libraries[library]['dll']
        name_rva[library, function] = position - base
        names += payload
        position += len(payload)
    thunks = bytearray()
    for library, functions in spec['thunks'].items():
        for function in functions:
            value, _ = lookup(library, function)
            if value is None:
                if (library, function) not in name_rva:
                    raise ValueError(f'{library} {function}: missing from hint_names')
                value = name_rva[library, function]
            thunks += struct.pack('<I', value)
        thunks += bytes(4)
    descriptors = bytearray()
    for library in spec['descriptors']:
        descriptors += struct.pack('<5I', lookups + blocks[library] - base, 0, 0,
                                   name_rva[library, ''], table + blocks[library] - base)
    descriptors += bytes(20)

    def place(name, parts):
        start, size = fragments[name]['address'], fragments[name]['size']
        data = bytearray(size)
        for address, payload in parts:
            if not start <= address <= address + len(payload) <= start + size:
                raise ValueError(f'imports.yml: {name} fragment does not contain its tables')
            data[address - start:address - start + len(payload)] = payload
        return start, bytes(data)

    return {
        'address_table': place('address_table', [(table, thunks)]),
        'directory': place('directory', [(directory, descriptors), (lookups, thunks), (names_start, names)]),
        'directories': {'import': (directory - base, len(descriptors)), 'iat': (table - base, len(thunks))},
    }


def header(version):
    spec = read_yaml(image_dir(version) / 'header.yml')
    opt, file_header = spec['optional_header'], spec['file_header']
    directories = import_tables(version)['directories']
    if not spec.get('linker_stub'):
        raise ValueError('header.yml: only the linker stub is supported')
    stub = DOS_STUB
    rich = rich_record(spec['rich'], len(DOS_HEADER) + len(stub))
    pe_offset = len(DOS_HEADER) + len(stub) + len(rich)
    dos = bytearray(DOS_HEADER)
    struct.pack_into('<I', dos, 0x3C, pe_offset)

    salign, falign, headers = opt['section_alignment'], opt['file_alignment'], opt['size_of_headers']
    section_table, rva, raw = bytearray(), salign, headers
    code = initialized = 0
    bases, rsrc = {}, None
    for section in spec['sections']:
        vsize, rsize, flags = section['virtual_size'], section['raw_size'], section['characteristics']
        if rsize % falign or rsize > align(vsize, falign):
            raise ValueError(f'header.yml: {section["name"]} raw size')
        if flags & 0x20:
            code += align(vsize, falign)
            bases.setdefault('code', rva)
        else:
            bases.setdefault('data', rva)
        if flags & 0x40:
            initialized += align(vsize, falign)
        if section['name'] == '.rsrc':
            rsrc = (rva, vsize)
        section_table += struct.pack('<8sIIIIIIHHI', section['name'].encode(), vsize, rva, rsize,
                                     raw if rsize else 0, 0, 0, 0, 0, flags)
        rva += align(vsize, salign)
        raw += rsize

    dirs = [(0, 0)] * opt['directories']
    dirs[1] = directories['import']
    dirs[12] = directories['iat']
    if rsrc:
        dirs[2] = rsrc
    optional = struct.pack('<HBBIIIIIIIIIHHHHHHIIIIHHIIIIII', 0x10B, *opt['linker_version'], code, initialized, 0,
                           opt['entry_point'] - opt['image_base'], bases['code'], bases['data'],
                           opt['image_base'], salign, falign, *opt['os_version'], *opt['image_version'],
                           *opt['subsystem_version'], 0, rva, headers, 0, opt['subsystem'],
                           opt['dll_characteristics'], *opt['stack'], *opt['heap'], 0, len(dirs))
    optional += b''.join(struct.pack('<II', *d) for d in dirs)
    coff_header = struct.pack('<HHIIIHH', file_header['machine'], len(spec['sections']), file_header['timestamp'],
                              0, 0, len(optional), file_header['characteristics'])
    data = bytes(dos) + stub + rich + b'PE\0\0' + coff_header + optional + section_table
    if len(data) > headers:
        raise ValueError('header.yml: headers exceed size_of_headers')
    return data + bytes(headers - len(data))


def jump_thunk(version, address):
    """One `jump_thunks` fragment of imports.yml: LINK's `jmp dword ptr [slot]` (FF 25) for each directly
    called import, in link order, through its import address table slot, then the fill to the
    fragment's end."""
    spec = read_yaml(image_dir(version) / 'imports.yml')
    slots, offset = {}, spec['fragments']['address_table']['address']
    for library, functions in spec['thunks'].items():
        for i, function in enumerate(functions):
            slots[library, function] = offset + 4 * i
        offset += 4 * (len(functions) + 1)
    matches = [f for f in spec['jump_thunks'] if f['address'] == address]
    if len(matches) != 1:
        raise ValueError(f'imports.yml: no jump_thunks fragment at 0x{address:08X}')
    fragment = matches[0]
    data = bytearray()
    for item in fragment['imports']:
        library, _, function = item.partition(' ')
        if (library, function) not in slots:
            raise ValueError(f'imports.yml: jump thunk {item} has no import address table slot')
        data += b'\xff\x25' + struct.pack('<I', slots[library, function])
    if len(data) > fragment['size']:
        raise ValueError(f'imports.yml: jump thunks exceed the fragment at 0x{address:08X}')
    return bytes(data) + bytes([fragment['fill']]) * (fragment['size'] - len(data))


RESOURCE_SCRIPT = ROOT / 'Jones3D/Indy3D.rc'


def resource_script(version):
    """The resource script of a version: its overlay of Jones3D/Indy3D.rc when it has one."""
    return ROOT / version_file(version, RESOURCE_SCRIPT.relative_to(ROOT), ROOT)


def section_placement(version, name):
    """(rva, virtual size, raw size) of a section as header.yml lays it out."""
    spec = read_yaml(image_dir(version) / 'header.yml')
    rva = spec['optional_header']['section_alignment']
    for section in spec['sections']:
        if section['name'] == name:
            return rva, section['virtual_size'], section['raw_size']
        rva += align(section['virtual_size'], spec['optional_header']['section_alignment'])
    raise ValueError(f'header.yml: no {name} section')


def section_padding(version, name):
    """LINK's zero fill of a section's last file block, from its virtual end to its raw size (header.yml)."""
    _, virtual_size, raw_size = section_placement(version, name)
    if raw_size <= virtual_size:
        raise ValueError(f'header.yml: {name} has no file padding')
    return bytes(raw_size - virtual_size)


def resources(version, icons):
    """The .rsrc file fragment compiled from the resource script, and its user-supplied asset bytes."""
    rva, virtual_size, raw_size = section_placement(version, '.rsrc')
    compiled = pe_rsrc.compile_script(resource_script(version), icons)
    data = pe_rsrc.layout(compiled, rva)
    if len(data) != virtual_size:
        raise ValueError(f'resources: 0x{len(data):X} bytes, header.yml says 0x{virtual_size:X}')
    return data + bytes(raw_size - len(data)), pe_rsrc.asset_bytes(compiled)


# Generated image fragments: output name -> generator arguments.  config.yml's image_sources
# names each output with the file fragment it replaces.  The jump-thunk fragments are named by their
# address, which imports.yml records per version.
FRAGMENTS = {
    'pe-header.bin': ['header'],
    'import-address-table.bin': ['imports', '--fragment', 'address_table'],
    'import-directory.bin': ['imports', '--fragment', 'directory'],
    'padding-_TEXT16.bin': ['padding', '--section', '_TEXT16'],
    'padding-.data1.bin': ['padding', '--section', '.data1'],
}


def fragments(version):
    """FRAGMENTS plus one jump-thunk fragment per imports.yml `jump_thunks` entry of the version."""
    spec = read_yaml(image_dir(version) / 'imports.yml')
    result = dict(list(FRAGMENTS.items())[:3])
    for fragment in spec['jump_thunks']:
        address = fragment['address']
        result[f'jump-thunks-{address:08X}.bin'] = ['thunks', '--address', f'0x{address:08X}']
    result.update(list(FRAGMENTS.items())[3:])
    return result


def inputs(version):
    """Every file the generators read, for the build graph."""
    spec = read_yaml(image_dir(version) / 'imports.yml')
    files = [Path(__file__), ROOT / 'Scripts/pe_library.py', ROOT / 'tools/config.py',
             image_dir(version) / 'header.yml', image_dir(version) / 'imports.yml']
    files += [ROOT / entry['path'] for entry in spec['libraries'].values()]
    return [f.relative_to(ROOT).as_posix() for f in files]


def resource_inputs(version):
    """Every checked-in file the resource generator reads (the icons are a separate, generated input)."""
    files = [Path(__file__), ROOT / 'Scripts/pe_rsrc.py', ROOT / 'tools/config.py',
             image_dir(version) / 'header.yml', resource_script(version)]
    return [f.relative_to(ROOT).as_posix() for f in files]


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('generator', choices=('header', 'imports', 'thunks', 'padding', 'resources'))
    parser.add_argument('--section', help='padding: the PE section whose file padding to write')
    parser.add_argument('--address', type=lambda v: int(v, 0), help='thunks: the fragment\'s virtual address')
    parser.add_argument('--version', default='indy3d-v1.0')
    parser.add_argument('--fragment', choices=('address_table', 'directory'))
    parser.add_argument('--icons', type=Path, help='resources: directory of the user-supplied icon files')
    parser.add_argument('--assets', type=Path, help='resources: write the user-supplied asset accounting here')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    assets = None
    try:
        if args.generator == 'header':
            data = header(args.version)
        elif args.generator == 'thunks':
            data = jump_thunk(args.version, args.address)
        elif args.generator == 'padding':
            if not args.section:
                parser.error('padding needs --section')
            data = section_padding(args.version, args.section)
        elif args.generator == 'resources':
            if not args.icons or not args.assets:
                parser.error('resources needs --icons and --assets')
            data, assets = resources(args.version, args.icons)
        else:
            if not args.fragment:
                parser.error('imports needs --fragment')
            data = import_tables(args.version)[args.fragment][1]
    except (ValueError, OSError, KeyError) as exc:
        parser.exit(1, f'error: {exc}\n')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    if not args.output.exists() or args.output.read_bytes() != data:
        args.output.write_bytes(data)
    if assets is not None:
        import json
        accounting = json.dumps({'class': 'user-supplied', 'bytes': assets,
                                 'description': 'icon image data (RT_ICON) from the icon files extracted '
                                                'from the user\'s own executable'}, indent=2) + '\n'
        if not args.assets.exists() or args.assets.read_text() != accounting:
            args.assets.write_text(accounting)


if __name__ == '__main__':
    main()
