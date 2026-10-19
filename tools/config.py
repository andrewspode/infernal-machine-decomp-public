"""DTK text metadata for the PE backend.

The supported DTK grammar is section declarations/ranges and named symbols.
PE review evidence uses standalone ``# pe: {...}`` comments immediately before
the entry it describes; DTK itself ignores these comments. Unknown attributes
are rejected rather than silently accepting unsupported linker semantics.
"""
import json
import re

import yaml


class UniqueLoader(yaml.SafeLoader):
    pass


def mapping(loader, node, deep=False):
    result = {}
    for key_node, value_node in node.value:
        key = loader.construct_object(key_node, deep=deep)
        if key in result:
            raise ValueError(f'Duplicate YAML key: {key}')
        result[key] = loader.construct_object(value_node, deep=deep)
    return result


UniqueLoader.add_constructor(yaml.resolver.BaseResolver.DEFAULT_MAPPING_TAG, mapping)


def read_yaml(path):
    try:
        return yaml.load(path.read_text(), Loader=UniqueLoader)
    except yaml.YAMLError as exc:
        raise ValueError(f'{path}: {exc}') from exc


def entries(path, allowed_notes):
    notes = {}
    for number, line in enumerate(path.read_text().splitlines(), 1):
        line = line.strip()
        if line.startswith('# pe:'):
            try:
                value = json.loads(line[5:].strip())
            except ValueError as exc:
                raise ValueError(f'{path}:{number}: invalid PE comment JSON') from exc
            if not isinstance(value, dict) or set(value) - allowed_notes or notes:
                raise ValueError(f'{path}:{number}: unknown or duplicate PE annotation')
            notes = value
        elif line and not line.startswith(('#', '//')):
            yield number, line, notes
            notes = {}
    if notes:
        raise ValueError(f'{path}: PE annotation has no following entry')


def attributes(tokens, allowed):
    result = {}
    for token in tokens:
        key, sep, value = token.partition(':')
        if not sep or not value or key not in allowed or key in result:
            raise ValueError(f'Unsupported or duplicate attribute: {token}')
        result[key] = value
    return result


# `.data1` is the separate PE section Intel C/C++ 4.0 objects put their literal pools, strings and SEH scope
# tables in (docs/crt.md, "`.data1` as a section kind"); it is initialized writable data like `.data`.
# `_TEXT16` is the 16-bit code segment (PE section `_TEXT16`, flag IMAGE_SCN_MEM_16BIT) one of INSANE's MASM 6.13
# objects defines (docs/crt.md, "MASM 6.13"); a code kind like `.text`.
SECTION_KINDS = {'.text': 'code', '_TEXT16': 'code', '.rdata': 'rodata', '.data': 'data', '.data1': 'data',
                 '.bss': 'bss'}
# COFF characteristics a section kind carries beyond its type (MASM's USE16 segment flag).
SECTION_EXTRA_FLAGS = {'_TEXT16': 0x20000}


def read_splits(path):
    sections, units, parts = {}, set(), []
    unit = None
    for number, line, notes in entries(path, {'bounds', 'padding', 'lead'}):
        try:
            if line.endswith(':'):
                unit = line[:-1].strip()
                if notes or not unit or any(c.isspace() for c in unit):
                    raise ValueError('Invalid unit header')
                if unit in units:
                    raise ValueError(f'Duplicate unit: {unit}')
                units.add(unit)
                continue
            name, *tokens = line.split()
            if unit == 'Sections':
                attrs = attributes(tokens, {'type', 'align'})
                if notes or name in sections or SECTION_KINDS.get(name) != attrs.get('type'):
                    raise ValueError(f'Unsupported or duplicate PE section: {name}')
                align = int(attrs.get('align', '1'), 0)
                if align <= 0 or align > 8192 or align & (align - 1):
                    raise ValueError('Invalid section alignment')
                sections[name] = align
            else:
                if not unit or name not in sections:
                    raise ValueError('Split needs a unit and a declared section')
                attrs = attributes(tokens, {'start', 'end', 'align'})
                parts.append({'source': unit, 'section': name,
                              'start': int(attrs['start'], 0), 'end': int(attrs['end'], 0),
                              'align': int(attrs.get('align', str(sections[name])), 0), **notes})
        except (ValueError, KeyError) as exc:
            raise ValueError(f'{path}:{number}: {exc}') from exc
    if not sections:
        raise ValueError(f'{path}: missing Sections declaration')
    return parts


SYMBOL = re.compile(r'([^\s=]+)\s*=\s*([A-Za-z0-9._]+):(0[xX][0-9a-fA-F]+|[0-9]+);\s*//\s*(.*)')


def read_symbols(path):
    symbols = []
    for number, line, notes in entries(path, {'bounds', 'coff_name', 'notes'}):
        try:
            match = SYMBOL.fullmatch(line)
            if not match:
                raise ValueError('Expected name = section:address; // type:... size:...')
            name, section, address, tail = match.groups()
            attrs = attributes(tail.split(), {'type', 'size', 'scope'})
            kind = {'function': 'function', 'object': 'data', 'label': 'label'}[attrs['type']]
            if section not in (*SECTION_KINDS, 'ABS') or (section == 'ABS' and kind != 'label'):
                raise ValueError('Unsupported symbol section')
            scope = attrs.get('scope', 'global')
            if scope not in ('global', 'local'):
                raise ValueError('Unsupported symbol scope')
            if notes.get('bounds', 'inferred') not in ('inferred', 'reviewed'):
                raise ValueError('Invalid bounds review status')
            if any(not isinstance(v, str) for v in notes.values()):
                raise ValueError('PE symbol annotations must be strings')
            symbols.append({'name': name, 'section': section, 'address': int(address, 0),
                            'kind': kind, 'size': int(attrs.get('size', '0'), 0),
                            'bounds': 'inferred' if kind != 'label' else '',
                            'scope': scope, 'coff_name': '', 'notes': '', **notes})
        except (ValueError, KeyError) as exc:
            raise ValueError(f'{path}:{number}: {exc}') from exc
    return symbols


def annotation(value):
    return '# pe: ' + json.dumps(value, ensure_ascii=False) + '\n' if value else ''


def format_splits(parts):
    text = 'Sections:\n' + ''.join(f'    {s:8} type:{k} align:1\n' for s, k in SECTION_KINDS.items())
    units = {}
    for p in parts:
        units.setdefault(p['source'], []).append(p)
    for unit, ranges in units.items():
        text += f'\n{unit}:\n'
        for p in ranges:
            notes = {k: p[k] for k in ('bounds', 'padding', 'lead') if p.get(k) not in (None, '', 'inferred')}
            if 'padding' in notes and int(str(notes['padding']), 0) == (0x90 if p['section'] == '.text' else 0):
                del notes['padding']
            text += annotation(notes)
            text += (f'    {p["section"]:8} start:0x{int(str(p["start"]), 0):08X} '
                     f'end:0x{int(str(p["end"]), 0):08X} align:{int(str(p["align"]), 0)}\n')
    return text


def format_symbols(symbols, parts):
    text = ''
    for s in symbols:
        address = int(str(s['address']), 0)
        section = next((p['section'] for p in parts
                        if int(str(p['start']), 0) <= address < int(str(p['end']), 0)),
                       'ABS' if s['kind'] == 'label' else '.text' if s['kind'] == 'function' else '.data')
        kind = {'function': 'function', 'data': 'object', 'label': 'label'}[s['kind']]
        notes = {k: s[k] for k in ('bounds', 'coff_name', 'notes') if s.get(k) not in (None, '', 'inferred')}
        text += annotation(notes)
        text += f'{s["name"]} = {section}:0x{address:08X}; // type:{kind}'
        if s.get('size'):
            text += f' size:0x{int(str(s["size"]), 0):X}'
        text += f' scope:{s.get("scope", "global")}\n'
    return text
