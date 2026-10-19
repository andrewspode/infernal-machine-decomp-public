"""Statically linked library members, taken from the shipped library archives.

Retail links some code and data from third-party static libraries: the
DirectInput data-format tables come from the DirectX SDK's ``dinput.lib`` and
the C runtime from the compiler's ``libc.lib``.  That code is not decompiled.
A *library unit* names one archive member; the build extracts it, unmodified,
from a pinned archive whose SHA-256 is recorded in ``libraries.yml``, and the
strict whole-TU matcher (``pe_link.compiled_unit``) then places and relocates
it exactly like a compiled game TU.  Byte identity after relocation is the only
evidence that the member is what retail linked; nothing is copied from the
original image.  See docs/crt.md.

The one exception is a *vendor* library (``image: true``): a third-party
library whose shipped archive is not the build retail linked, so no archive
can supply its members (owner ruling 2026-10-16, the Intel libm members).  Its
units take their bytes from the user's own retail executable at build time:
the split object of the unit's ranges, from the SHA-256-checked image named by
``config.yml``.  Those bytes are never committed; the unit is still placed and
verified by the strict matcher and reported apart from game code.
"""
import hashlib
from pathlib import Path
import re

ARCHIVE_MAGIC = b'!<arch>\n'
UNIT_PREFIX = 'lib/'


def archive_members(data):
    """Yield (name, payload) for every ordinary member of an ``ar`` archive.

    Microsoft archives start with two linker members (``/``) and an optional
    long-name member (``//``); ordinary members name themselves either inline
    (``name/``) or through ``/<offset>`` into the long-name member.
    """
    if not data.startswith(ARCHIVE_MAGIC):
        raise ValueError('not an ar archive')
    position, longnames = len(ARCHIVE_MAGIC), b''
    while position < len(data):
        header = data[position:position + 60]
        if len(header) != 60 or header[58:60] != b'`\n':
            raise ValueError(f'invalid archive member header at offset {position}')
        name = header[:16].decode('latin-1').rstrip()
        size = int(header[48:58].decode('ascii').strip())
        payload = data[position + 60:position + 60 + size]
        if len(payload) != size:
            raise ValueError('truncated archive member')
        position += 60 + size + (size & 1)
        if name == '//':
            longnames = payload
        elif name != '/':
            if re.fullmatch(r'/\d+', name):
                start = int(name[1:])
                end = longnames.find(b'\0', start)
                if end < 0:
                    raise ValueError('invalid archive long name')
                name = longnames[start:end].decode('latin-1')
            else:
                name = name.removesuffix('/')
            yield name, payload


def load(metadata, root, path, image=None):
    """Validate ``libraries.yml``: the pinned archives and the library units.

    ``image`` is ``(path, sha256)`` of the project's retail executable, which a
    vendor library (``image: true``) reads instead of an archive."""
    unknown = set(metadata) - {'libraries', 'units'}
    if unknown:
        raise ValueError(f'{path}: unsupported library settings {sorted(unknown)}')
    libraries = metadata.get('libraries') or {}
    for key, library in libraries.items():
        if isinstance(library, dict) and 'image' in library:
            if set(library) != {'image', 'origin'} or library['image'] is not True or not library['origin']:
                raise ValueError(f'{path}: vendor library {key} takes exactly `image: true` and its origin')
            if image is None:
                raise ValueError(f'{path}: vendor library {key} needs the project\'s retail executable')
            library.update(path=str(image[0]), sha256=str(image[1]))
            continue
        if (not isinstance(library, dict) or set(library) != {'path', 'sha256', 'origin'}
                or not re.fullmatch(r'[0-9a-f]{64}', str(library['sha256']))
                or Path(library['path']).is_absolute() or '..' in Path(library['path']).parts):
            raise ValueError(f'{path}: library {key} needs a relative path, a sha256 and its origin')
    units = {}
    for unit, entry in (metadata.get('units') or {}).items():
        if (not isinstance(entry, dict) or set(entry) - {'reference'} != {'library', 'member'}
                or entry['library'] not in libraries or not isinstance(entry['member'], str)):
            raise ValueError(f'{path}: library unit {unit} needs a declared library and a member name')
        if 'reference' in entry:
            # A vendor unit may name the C reconstruction kept beside it as documentation (not built).
            reference = entry['reference']
            if (not is_vendor(libraries[entry['library']]) or not isinstance(reference, str)
                    or Path(reference).is_absolute() or '..' in Path(reference).parts
                    or (root is not None and not (Path(root) / reference).is_file())):
                raise ValueError(f'{path}: library unit {unit}: a reference source is an existing file of a vendor unit')
        expected = f'{UNIT_PREFIX}{entry["library"]}/{entry["member"].replace(chr(92), "/").rsplit("/", 1)[-1]}'
        if unit != expected:
            raise ValueError(f'{path}: library unit {unit} must be named {expected}')
        units[unit] = {**entry, **libraries[entry['library']]}
    return units


def is_vendor(entry):
    """Whether a library unit's bytes come from the retail executable, not an archive."""
    return entry.get('image') is True


def vendor_member(unit, split_object, leads):
    """A vendor unit's member: its split object without the alignment fill LINK put before it.

    ``split_object`` holds the unit's retail ranges in split order, one section
    each; ``leads`` is each range's declared leading fill (splits.txt ``lead``),
    which belongs to the link, not to the member, so it is cut and the section's
    symbols move with it.  Nothing else changes."""
    from . import coff
    obj = coff.Object.read(split_object)
    if len(leads) != len(obj.sections):
        raise ValueError(f'{unit}: split object does not match the unit\'s ranges')
    sections = []
    for section, lead in zip(obj.sections, leads):
        if section.relocations or (lead and section.is_bss):
            raise ValueError(f'{unit}: unexpected split section {section.name}')
        if lead:
            section = coff.Section(section.name, section.data[lead:], section.flags)
        sections.append(section)
    symbols = []
    for symbol in obj.symbols.values():
        lead = leads[symbol.section - 1] if symbol.section > 0 else 0
        if symbol.value < lead:
            raise ValueError(f'{unit}: symbol {symbol.name} lies in the leading fill')
        symbols.append(coff.Symbol(symbol.name, symbol.value - lead, symbol.section, symbol.type, symbol.storage))
    return coff.encode(sections, symbols)


def extract(root, unit, entry, split_object=None, leads=None):
    """The unmodified bytes of one member of a hash-verified library archive.

    A vendor unit's member is built from its split object (``split_object``,
    written by ``pebuild split`` from the retail image; ``leads`` per range),
    after the image itself is checked against the pinned SHA-256."""
    if is_vendor(entry):
        image = Path(root) / entry['path']
        if hashlib.sha256(image.read_bytes()).hexdigest() != entry['sha256']:
            raise ValueError(f'{unit}: {entry["path"]} is not the pinned retail executable (SHA-256 mismatch)')
        if split_object is None or not Path(split_object).is_file():
            raise ValueError(f'{unit}: vendor unit has no split object; run the split first')
        return vendor_member(unit, split_object, leads or [])
    archive = Path(root) / entry['path']
    data = archive.read_bytes()
    if hashlib.sha256(data).hexdigest() != entry['sha256']:
        raise ValueError(f'{unit}: {entry["path"]} is not the pinned archive (SHA-256 mismatch)')
    found = [payload for name, payload in archive_members(data) if name == entry['member']]
    if len(found) != 1:
        raise ValueError(f'{unit}: {entry["path"]} has {len(found)} members named {entry["member"]}')
    return found[0]
