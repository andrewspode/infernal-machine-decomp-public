"""Select complete TU providers and assemble the fixed-address PE image."""
import json

from . import coff
from .pe_library import is_vendor
from .pe_link import compiled_unit, digest
from .pe_project import write_changed, write_json


def load_layout(project, path=None):
    layout = json.loads((path or project.link_layout).read_text())
    if (layout.get('schema') != 3 or layout.get('identity') != project.layout_identity
            or layout['sha256'] != project.config['sha256']):
        raise ValueError(f'Reference layout is stale ({project.relative(path or project.link_layout)}); regenerate it '
                         'with ninja split and refresh any frozen layout (ninja freeze-layout)')
    # Frozen metadata is portable between --build-dir values. The unit and
    # section index identify each input; no old build-root path is authoritative.
    for part in [*layout['ranges'], *layout['fragments']]:
        part['object'] = (project.obj('orig', part['source']) if part.get('source') else
                          project.build / 'orig/__preserved__.obj').as_posix()
    return layout


def user_assets(project, key, size):
    """Bytes of a generated fragment that are a declared user-supplied asset (0 if none)."""
    path = project.image_assets.get(key)
    if path is None:
        return 0
    accounting = json.loads((project.root / path).read_text())
    if accounting.get('class') != 'user-supplied' or not 0 <= accounting.get('bytes', -1) <= size:
        raise ValueError(f'{path}: invalid user-supplied asset accounting')
    return accounting['bytes']


def match(project, unit):
    if unit not in project.matching_units:
        raise ValueError(f'{unit}: select Object(Matching, ...) in configure.py before checking it')
    output = project.root / project.obj('matched', unit)
    output.unlink(missing_ok=True)
    obj = coff.Object.read(project.root / project.obj('obj', unit))
    sections = compiled_unit(project, unit, obj, load_layout(project))
    # A validation artifact for Ninja/objdiff. Reconstruction rechecks native
    # compiler output, so an old artifact can never authorize a stale match.
    write_changed(output, coff.encode(sections, []))
    print(f'Matched complete TU: {unit} ({len(sections)} code/data/BSS splits).')


def link(project, source_only=False):
    directory = project.root / project.build
    if source_only:
        directory /= 'source-only'
    (directory / 'verify.ok').unlink(missing_ok=True)
    (directory / 'link.json').unlink(missing_ok=True)
    layout = load_layout(project, project.source_only_layout if source_only else None)
    selected = set(project.matching_units)
    providers = project.image_sources
    eligible = {(f['file_offset'], f['size']) for f in layout['fragments'] if not f.get('source')}
    if set(providers) - eligible:
        raise ValueError('Generated image inputs must exactly replace preserved file fragments, never part of a TU')
    fallback = [f for f in layout['fragments'] if f.get('source') not in selected
                and (f['file_offset'], f['size']) not in providers]
    fallback_units = {r['source'] for r in layout['ranges'] if r['source'] not in selected}
    fallback_bytes = sum(f['size'] for f in fallback)
    if source_only and (fallback or fallback_units):
        raise ValueError(f'Source-only build still needs original fallback: {len(fallback_units)} TUs, '
                         f'{len(fallback)} file fragments / {fallback_bytes:,} file bytes')

    objects, replacements = {}, {}
    for unit in project.matching_units:
        obj = coff.Object.read(project.root / project.obj('obj', unit))
        sections = compiled_unit(project, unit, obj, layout)
        for index, section in enumerate(sections, 1):
            replacements[project.obj('orig', unit).as_posix(), index] = section

    def original_section(part):
        path = part['object']
        if path not in objects:
            objects[path] = coff.Object.read(project.root / path)
        return objects[path].sections[part['section'] - 1]

    for part in layout['ranges']:
        if part['source'] in selected:
            continue
        section = original_section(part)
        if section.size != part['size'] or section.is_bss != (part['kind'] == '.bss') or section.relocations:
            raise ValueError(f'{part["object"]}: split section size or storage kind changed')
    output = bytearray()
    source_bytes, generated_bytes, library_bytes, asset_bytes, vendor_bytes = 0, 0, 0, 0, 0
    library_objects = {project.obj('orig', unit).as_posix() for unit in project.libraries}
    vendor_objects = {project.obj('orig', unit).as_posix() for unit, entry in project.libraries.items()
                      if is_vendor(entry)}
    for fragment in layout['fragments']:
        if fragment['file_offset'] != len(output):
            raise ValueError('Image layout contains a gap or overlap')
        key = fragment['file_offset'], fragment['size']
        if key in providers:
            data = (project.root / providers[key]).read_bytes()
            assets = user_assets(project, key, len(data))
            asset_bytes += assets
            generated_bytes += len(data) - assets
        else:
            section = replacements.get((fragment['object'], fragment['section']))
            if section is not None and fragment['object'] in library_objects:
                library_bytes += fragment['size']
                if fragment['object'] in vendor_objects:
                    vendor_bytes += fragment['size']
            elif section is not None:
                source_bytes += fragment['size']
            else:
                section = original_section(fragment)
            if section.relocations or section.size != fragment.get('section_size', fragment['size']):
                raise ValueError(f'{fragment["object"]}: fragment has changed size or has relocations')
            data = bytes(fragment['size']) if section.is_bss else section.data
        if len(data) != fragment['size'] or digest(data) != fragment['sha256']:
            raise ValueError(f'Image bytes differ at file fragment 0x{fragment["file_offset"]:X} (size {fragment["size"]})')
        output.extend(data)
    if len(output) != layout['file_size'] or digest(output) != project.config['sha256']:
        raise ValueError('Rebuilt image size or SHA-256 mismatch')
    import hashlib
    if hashlib.sha1(output).hexdigest() != project.config['sha1']:
        raise ValueError('Rebuilt image SHA-1 differs from config.yml / build.sha1')

    # Library members are verified the same way but are not reconstructed
    # source: they are reported apart, so TU progress counts only game code.
    source_units, library_units = [], []
    for unit in project.matching_units:
        parts = project.unit_splits[unit]
        (library_units if unit in project.libraries else source_units).append({
            'source': unit, **({'vendor': True} if unit in project.libraries and is_vendor(project.libraries[unit]) else {}),
            'functions': sum(s['kind'] == 'function' for s in project.units[unit]),
            'data_symbols': sum(s['kind'] == 'data' for s in project.units[unit]),
            'sections': {kind: sum(p['end'] - p['start'] for p in parts if p['section'] == kind)
                         for kind in ('.text', '_TEXT16', '.rdata', '.data', '.data1', '.bss')},
        })
    report = {
        'backend': 'pe-fixed-address-tu', 'source_units': source_units,
        'source_bytes': source_bytes, 'generated_image_bytes': generated_bytes,
        'library_units': library_units, 'library_bytes': library_bytes,
        # Of library_bytes: vendor members, whose bytes the build reads from the user's own retail
        # executable because no shipped archive holds the build retail linked (pe_library, image: true).
        'vendor_bytes': vendor_bytes,
        'original_bytes': fallback_bytes, 'file_size': len(output),
        # User-supplied assets (game artwork read from the user's own executable at build time) are
        # neither source nor original fallback: reported here and excluded from every progress figure.
        'user_asset_bytes': asset_bytes, 'counted_file_size': len(output) - asset_bytes,
        'source_bss_bytes': sum(u['sections']['.bss'] for u in source_units),
        'original_bss_bytes': sum(r['size'] for r in layout['ranges']
                                  if r['kind'] == '.bss' and r['source'] not in selected),
        'fallback_units': sorted(fallback_units),
        'fallback_fragments': [{k: f[k] for k in ('object', 'section', 'file_offset', 'size')} for f in fallback],
        'source_only': not fallback and not fallback_units,
    }
    write_changed(directory / 'Indy3D.exe', output)
    write_json(directory / 'link.json', report)
    write_changed(directory / 'verify.ok', project.config['sha256'] + '\n')
    print(f'Rebuilt {len(output):,} bytes; {len(source_units)} complete TUs supply {source_bytes:,} file bytes '
          f'and {report["source_bss_bytes"]:,} BSS bytes; {len(library_units)} library members supply '
          f'{library_bytes:,} file bytes (vendor, from the retail image: {vendor_bytes:,}); user-supplied assets {asset_bytes:,} file bytes (not counted). '
          f'Original fallback: {fallback_bytes:,} file bytes. SHA-256 OK.')
