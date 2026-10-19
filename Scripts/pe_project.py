"""Shared paths and validation for the versioned PE build."""
import bisect
import csv
import hashlib
import json
from pathlib import Path
import re

from tools.config import read_yaml, read_splits, read_symbols, SECTION_EXTRA_FLAGS, SECTION_KINDS
from . import pe_library

ROOT = Path(__file__).resolve().parent.parent
# The version a Project() and the research tools default to: retail CD 1.0, the version the measurement data, the
# debug-build evidence and the notes' addresses describe. The default *build* is configure.py's (1.2).
DEFAULT_VERSION = 'indy3d-v1.0'
UNASSIGNED = '__unassigned__'
# A version built from the shared tree with some files replaced: the overlay directory mirrors the tree's
# paths, and a file there is used instead of the shared one for that version. The shared tree is the 1.2
# source; 1.0 builds with its copies of the files 1.2 changed (README.md, "Versions").
OVERLAY_DIRS = {'indy3d-v1.0': Path('versions/1.0')}

# Source files only the debug build compiled (docs/debug-build.md, "Debug-only code under J3D_DEBUG"): retail has no byte
# of them, so they are in no configure.py object and own no split. Each maps to (a sibling unit whose release build
# configuration compiles it for tools/probes/dbgverify.py, its original path as the debug image's strings give it).
DEBUG_ONLY_SOURCES = {
    'Libs/sith/World/sithWrite.c': ('Libs/sith/World/sithWorld.c', r'C:\Jones3D\Libs\sith\World\sithWrite.c'),
}

# The release flags an unoptimised, asserts-on compile drops (tools/probes/dbgverify.py and `ninja debug`): the
# optimisations, their implied /Ob1 inline expansion (the debug build expanded no function inline) and /DNDEBUG, whose
# absence turns on J3D_DEBUG in Libs/j3dcore/j3d.h. '-O2' is Intel C 4.0's spelling. /Od is put first in their place.
DEBUG_DROP_FLAGS = frozenset({'-O2', '/O2', '/Oi', '/Ot', '/Oy', '/DNDEBUG', '/Od', '/Ox', '/O1', '/Ob1'})
# The compiler of the units the debug build compiled unoptimised: the Jones3D project and its Libs. The other
# configured compilers build separately built libraries the debug image links in their optimised release form
# (INSANE and zlib with VC6, the Katmai rdModel3K and libm with Intel C 4.0, stdUtil_StrCmp with VC4.20, the MASM
# objects; docs/debug-build-census.md, the FPO region), so `ninja debug` compiles those with their release flags.
DEBUG_COMPILERS = frozenset({'VC5_SP3'})


def debug_cflags(cflags):
    """A unit's release flags as the debug build's unoptimised, asserts-on compile (DEBUG_DROP_FLAGS)."""
    return ['/Od'] + [f for f in cflags if f not in DEBUG_DROP_FLAGS]


def version_file(version, path, root=ROOT):
    """The file a version builds from for the tree path `path`: its overlay copy when there is one."""
    overlay = OVERLAY_DIRS.get(version)
    if overlay is not None and (Path(root) / overlay / path).is_file():
        return (overlay / path).as_posix()
    return Path(path).as_posix()


def write_changed(path, data):
    path = Path(path)
    if isinstance(data, str):
        data = data.encode()
    path.parent.mkdir(parents=True, exist_ok=True)
    if not path.exists() or path.read_bytes() != data:
        temporary = path.with_name(path.name + '.tmp')
        temporary.write_bytes(data)
        temporary.replace(path)


def write_json(path, value):
    write_changed(path, json.dumps(value, indent=2) + '\n')


class Project:
    def __init__(self, version=DEFAULT_VERSION, root=ROOT, build_dir=Path("build"), matching_units=None):
        self.root = Path(root)
        if Path(version).name != version:
            raise ValueError('Version must be a directory name under config/')
        self.version = version
        self.directory = self.root / 'config' / version
        self.config_file = self.directory / 'config.yml'
        metadata = read_yaml(self.config_file)
        unknown = set(metadata) - {'game_name', 'name', 'object', 'hash', 'symbols', 'splits', 'pe'}
        if unknown:
            raise ValueError(f'Unsupported PE project settings: {sorted(unknown)}')
        unknown = set(metadata['pe']) - {'sha256', 'image_base', 'machine', 'data_pending',
                                         'relocations', 'layout', 'source_layout', 'image_sources', 'libraries'}
        if unknown:
            raise ValueError(f'Unsupported PE backend settings: {sorted(unknown)}')
        self.config = {**metadata['pe'], 'original': metadata['object'],
                       'name': metadata.get('game_name', metadata['name']), 'sha1': metadata['hash']}
        self.build = Path(build_dir) / version
        self.symbol_file = self.root / metadata['symbols']
        self.split_file = self.root / metadata['splits']
        self.check_sha_path = self.directory / 'build.sha1'
        check = self.check_sha_path.read_text().split()
        if len(check) != 2 or check[0] != self.config['sha1'] or Path(check[1]).name != 'Indy3D.exe':
            raise ValueError('build.sha1 must agree with config.yml hash and name Indy3D.exe')
        self.matching_file = self.directory / self.config.get('relocations', 'relocations.yml')
        self.matching = read_yaml(self.matching_file)
        if set(self.matching) - {'symbols', 'shared', 'discarded', 'folded'}:
            raise ValueError('TU matching status belongs in configure.py Object(Matching, ...), not relocation metadata')
        for source, names in (self.matching.get('discarded') or {}).items():
            if not isinstance(names, list) or not all(isinstance(name, str) for name in names):
                raise ValueError(f'{source}: discarded functions must be a list of COFF names')
        for source, folded in (self.matching.get('folded') or {}).items():
            if not isinstance(folded, dict) or not all(isinstance(key, str) for key in folded):
                raise ValueError(f'{source}: folded bodies must map a COFF name to a retail symbol '
                                 'name or to an address/size/evidence entry')
            for key, value in folded.items():
                if isinstance(value, str):
                    continue
                # A fold onto an address no TU owns has no owner symbol to name,
                # so the declaration carries the address, the measured retail
                # extent and the referencing evidence instead. The extent and
                # the bytes are still verified against the retail image.
                if not isinstance(value, dict) or set(value) != {'address', 'size', 'evidence'}:
                    raise ValueError(f'{source}: folded body {key} must name a retail symbol or give '
                                     'exactly address, size and evidence for an unowned fold target')
                try:
                    address, size = int(str(value['address']), 0), int(str(value['size']), 0)
                except ValueError:
                    raise ValueError(f'{source}: folded body {key} has an unparsable address/size')
                if not 0 < address <= 0xFFFFFFFF or size <= 0 or address % 16:
                    raise ValueError(f'{source}: folded body {key} needs a positive size and a '
                                     '16-byte-aligned retail address')
                if not isinstance(value['evidence'], str) or not re.search(
                        r'0x[0-9A-Fa-f]{6}', value['evidence']):
                    raise ValueError(f'{source}: folded body {key} needs evidence naming the retail '
                                     'address that reaches the folded name')
        # Statically linked library members (pe_library): always selected,
        # because the archive, not reconstructed source, supplies them.
        self.library_file = (self.directory / self.config['libraries']) if self.config.get('libraries') else None
        # A vendor library's units read the retail executable itself (pe_library, image: true).
        self.libraries = (pe_library.load(read_yaml(self.library_file), self.root, self.library_file,
                                          image=(self.config.get('original'), self.config.get('sha256')))
                          if self.library_file else {})
        if matching_units is None:
            runtime = self.root / self.build / 'config.json'
            if not runtime.exists():
                raise ValueError('Run configure.py before using the PE backend')
            matching_units = json.loads(runtime.read_text())['matching_units']
        if not isinstance(matching_units, list):
            raise ValueError('Matching units must be a list of source paths')
        matching_units = [*matching_units, *(u for u in sorted(self.libraries) if u not in matching_units)]
        self.matching_units = matching_units
        if not isinstance(matching_units, list) or any(not isinstance(s, str) for s in matching_units):
            raise ValueError('Matching units must be a list of source paths')
        if len(set(self.matching_units)) != len(self.matching_units):
            raise ValueError('Duplicate matching TU')
        self.symbols = {}
        for row in read_symbols(self.symbol_file):
            name = row['name']
            if name in self.symbols:
                raise ValueError(f'Duplicate symbol: {name}')
            if row['kind'] != 'label' and row['size'] <= 0:
                raise ValueError(f'{name}: invalid symbol extent')
            if not 0 <= row['address'] <= 0xFFFFFFFF or row['size'] < 0:
                raise ValueError(f'{name}: invalid symbol address/size')
            self.symbols[name] = row
        self.splits = read_splits(self.split_file)
        self.splits.sort(key=lambda s: s['start'])
        self.unit_splits = {}
        self.units = {}
        last_end = 0
        for part in self.splits:
            source, start, end, align = part['source'], part['start'], part['end'], part['align']
            part['bounds'] = part.get('bounds') or 'inferred'
            padding = part.get('padding')
            if padding in (None, ''):
                padding = '0x90' if part['section'] == '.text' else '0'
            part['padding'] = int(str(padding), 0)
            if part['bounds'] not in ('inferred', 'reviewed') or not 0 <= part['padding'] <= 255:
                raise ValueError(f'Invalid split review/padding: {part}')
            if part['section'] == '.bss' and part['padding']:
                raise ValueError('BSS padding must be zero')
            if (source != UNASSIGNED and source not in self.libraries
                    and (not source or Path(source).is_absolute() or '..' in Path(source).parts
                         or not (self.root / source).is_file())):
                raise ValueError(f'Invalid split source path: {source}')
            if part['section'] not in SECTION_KINDS or not 0 < start < end <= 0x100000000:
                raise ValueError(f'Invalid section range: {part}')
            if start < last_end:
                raise ValueError(f'Overlapping split at 0x{start:08X} ({source})')
            # `lead`: the split begins with the alignment fill LINK inserted before
            # its first contribution (the bytes from the preceding contribution's
            # end up to this one's aligned start). It must be less than the
            # alignment and start where the preceding contribution ends: at a
            # recorded range's end, or else (checked against the image by
            # pebuild.split) right after a byte that is not fill. For a compiled
            # TU it must equal the gap the first section's own alignment
            # produces (pe_link).
            lead = part.get('lead')
            part['lead'] = int(str(lead), 0) if lead not in (None, '') else 0
            part['lead_after_range'] = bool(part['lead']) and start == last_end
            if align <= 0 or align > 8192 or align & (align - 1) or (start + part['lead']) % align:
                raise ValueError(f'Invalid split alignment: {part}')
            if part['lead'] and (part['lead'] >= align or part['section'] == '.bss' or start < last_end
                                 or start + part['lead'] >= end):
                raise ValueError(f'Invalid leading alignment fill: {part}')
            last_end = end
            self.unit_splits.setdefault(source, []).append(part)
            self.units.setdefault(source, [])
        starts = [s['start'] for s in self.splits]
        for symbol in self.symbols.values():
            index = bisect.bisect_right(starts, symbol['address']) - 1
            part = self.splits[index] if index >= 0 else None
            symbol['source'] = ''
            if symbol['kind'] == 'label':
                continue
            if not part or not part['start'] <= symbol['address'] < part['end']:
                raise ValueError(f'{symbol["name"]}: no owning split')
            if symbol['address'] + symbol['size'] > part['end']:
                raise ValueError(f'{symbol["name"]}: symbol exceeds its split')
            if symbol['kind'] == 'function' and part['section'] != '.text':
                raise ValueError(f'{symbol["name"]}: function must belong to .text')
            symbol['source'] = part['source'] if part['source'] != UNASSIGNED else ''
            if symbol['section'] != part['section']:
                raise ValueError(f'{symbol["name"]}: symbol section disagrees with owning split')
            symbol['section'] = part['section']
            self.units[part['source']].append(symbol)
        extents = sorted((s for s in self.symbols.values() if s['kind'] != 'label'), key=lambda s: s['address'])
        for a, b in zip(extents, extents[1:]):
            if a['address'] + a['size'] > b['address']:
                raise ValueError(f'Overlapping symbol ranges: {a["name"]}, {b["name"]}')
        # A fold declared by address may only target retail bytes that nothing
        # owns: anything inside an owned split, or covered by a symbol, must be
        # declared onto that symbol so the reviewed-owner checks still apply.
        for source, address, size in self.unowned_folds:
            covered = [s['name'] for s in self.symbols.values() if s['kind'] != 'label'
                       and s['address'] < address + size and address < s['address'] + s['size']]
            if covered:
                raise ValueError(f'{source}: folded address 0x{address:08X} is covered by '
                                 f'{covered[0]}; declare the fold onto that symbol')
            owner = next((p['source'] for p in self.splits if p['source'] != UNASSIGNED
                          and p['start'] < address + size and address < p['end']), None)
            if owner:
                raise ValueError(f'{source}: folded address 0x{address:08X} lies inside {owner}; '
                                 'an owned fold target must be declared by symbol name')
        self.pending_file = self.directory / self.config['data_pending'] if self.config.get('data_pending') else None
        self.pending_data = []
        if self.pending_file:
            with self.pending_file.open(newline='') as f:
                self.pending_data = list(csv.DictReader(f))
        for source in self.matching_units:
            if source == UNASSIGNED or not self.units.get(source):
                raise ValueError(f'{source}: matching TU needs owned symbols and splits')
            if any(p['bounds'] != 'reviewed' for p in self.unit_splits[source]):
                raise ValueError(f'{source}: every split must have bounds=reviewed before selecting a matching TU')
            if any(s['bounds'] != 'reviewed' for s in self.units[source]):
                raise ValueError(f'{source}: every function/data symbol needs a reviewed extent')
            if any(s['suggested_source'] == source for s in self.pending_data):
                raise ValueError(f'{source}: unresolved data_pending.csv entries prevent TU completion')
            addresses = {s['address'] for s in self.units[source]}
            for symbol in self.symbols.values():
                if symbol['kind'] == 'label' and symbol['address'] not in addresses:
                    if any(p['start'] <= symbol['address'] < p['end'] for p in self.unit_splits[source]):
                        raise ValueError(f'{source}: unresolved label inside a matching TU: {symbol["name"]}')

        self.image_sources = {}
        # Accounting files of user-supplied assets a generated fragment embeds (README.md,
        # "User-supplied assets"): bytes read from the user's own copy, reported apart.
        self.image_assets = {}
        for entry in self.config.get('image_sources', []):
            start, size = int(str(entry['file_offset']), 0), int(str(entry['size']), 0)
            path = Path(entry['path'])
            assets = Path(entry['assets']) if entry.get('assets') else None
            if (start < 0 or size <= 0 or set(entry) - {'file_offset', 'size', 'path', 'assets'}
                    or any(p.is_absolute() or '..' in p.parts for p in filter(None, (path, assets)))):
                raise ValueError(f'Invalid generated image input: {entry}')
            if (start, size) in self.image_sources:
                raise ValueError('Duplicate generated image range')
            self.image_sources[start, size] = path
            if assets:
                self.image_assets[start, size] = assets

    @property
    def unowned_folds(self):
        """Declared folds onto retail addresses no TU owns: (source, address, size)."""
        folds = []
        for source, folded in sorted((self.matching.get('folded') or {}).items()):
            for key, value in sorted(folded.items()):
                if not isinstance(value, str):
                    folds.append((source, int(str(value['address']), 0), int(str(value['size']), 0)))
        return folds

    @property
    def functions(self):
        return [s for s in self.symbols.values() if s['kind'] == 'function']

    @property
    def data_symbols(self):
        return [s for s in self.symbols.values() if s['kind'] == 'data']

    def obj(self, directory, unit):
        return self.build / directory / Path(unit).with_suffix('.obj')

    @property
    def original_objects(self):
        return [self.obj('orig', unit) for unit in sorted(self.units)] + [self.build / 'orig/__preserved__.obj']

    @property
    def link_layout(self):
        return self.directory / self.config['layout'] if self.config.get('layout') else self.root / self.build / 'layout.json'

    @property
    def source_only_layout(self):
        """The layout a source-only link reads: the frozen copy in the config directory (pe.source_layout,
        written by `pebuild freeze-layout`), so that link reads nothing extracted from the original."""
        return self.directory / self.config['source_layout'] if self.config.get('source_layout') else self.link_layout

    @property
    def layout_identity(self):
        # Geometry and symbol identity only: changing review status or TU
        # selection does not require re-extracting a frozen reference layout.
        value = {'sha256': self.config['sha256'],
                 'splits': [{k: p[k] for k in ('source', 'section', 'start', 'end', 'align', 'padding', 'lead')}
                            for p in self.splits],
                 'symbols': [{k: s.get(k, '') for k in ('name', 'address', 'kind', 'size', 'source', 'coff_name')}
                             for s in self.symbols.values()],
                 # A fold declared by address needs retail payload in the
                 # layout, so it is part of the layout's identity too.
                 'folds': sorted({(a, n) for _, a, n in self.unowned_folds})}
        return hashlib.sha256(json.dumps(value, sort_keys=True).encode()).hexdigest()

    def relative(self, path):
        try:
            return Path(path).relative_to(self.root).as_posix()
        except ValueError:
            return Path(path).as_posix()

    @property
    def config_inputs(self):
        paths = [self.config_file, self.check_sha_path, self.symbol_file, self.matching_file]
        if self.split_file:
            paths.append(self.split_file)
        if self.pending_file:
            paths.append(self.pending_file)
        if self.library_file:
            paths.append(self.library_file)
        if self.config.get('layout'):
            paths.append(self.link_layout)
        return [self.relative(p) for p in paths]
