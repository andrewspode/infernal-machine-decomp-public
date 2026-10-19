"""Small, deterministic i386 COFF reader/writer used by the PE build backend.

The reader retains raw symbol indices (including gaps for auxiliary records),
because COFF relocations refer to those indices, not to a compact symbol list.
See https://learn.microsoft.com/en-us/windows/win32/debug/pe-format.
"""
from dataclasses import dataclass, field, replace
from pathlib import Path
import re
import struct

DIR32 = 0x0006
DIR32NB = 0x0007
REL32 = 0x0014
CODE = 0x60500020
RDATA = 0x40300040
DATA = 0xC0300040
BSS = 0xC0300080


def is_string(name):
    """VC6 compiler identities for narrow and wide string literals."""
    return name.startswith(('??_C@_0', '??_C@_1'))


@dataclass
class Symbol:
    name: str
    value: int = 0
    section: int = 0
    type: int = 0
    storage: int = 2


@dataclass
class Section:
    name: str
    data: bytes
    flags: int = CODE
    relocations: list = field(default_factory=list)
    size: int | None = None

    def __post_init__(self):
        if self.size is None:
            self.size = len(self.data)
        if self.size < 0 or (self.is_bss and (self.data or self.relocations)):
            raise ValueError('BSS must have a size, without initialized bytes or relocations')
        if not self.is_bss and self.size != len(self.data):
            raise ValueError('Initialized section size differs from its payload')

    @property
    def is_bss(self):
        return bool(self.flags & 0x80)

    @property
    def alignment(self):
        value = (self.flags >> 20) & 15
        if value == 15:
            raise ValueError('Invalid COFF section alignment')
        return 1 << (value - 1) if value else 16


def undecorate(name):
    # Floating-point and C++ compiler symbols can end in @<digits>, too.
    # That suffix is part of their identity, not a stdcall argument count.
    if name.startswith(('?', '__real@')):
        return name
    # VC5 appends a compiler-generated serial to file-local C identifiers.
    name = re.sub(r'\$S\d+$', '', name)
    return re.sub(r'^([A-Za-z_]\w*)@\d+$', r'\1', name.removeprefix('_'))


def allocate_common(obj):
    """Represent linker allocations without changing the native COFF object.

    Section-zero external symbols with nonzero values request that many BSS
    bytes. LINK 5.00 aligns them to the next power of two, capped at 32 bytes
    (see stdConffile.md). No original metadata or payload determines these
    sizes/alignments. Strict placement separately checks ownership, exact size,
    addresses and full coverage; common coalescing across owners is unsupported.
    """
    sections = list(obj.sections)
    symbols = {i: replace(s) for i, s in obj.symbols.items()}
    for symbol in symbols.values():
        if symbol.section != 0 or not symbol.value:
            continue
        if symbol.storage != 2 or symbol.type & 0x20:
            raise ValueError(f'Invalid common symbol {symbol.name}')
        size = symbol.value
        alignment = min(32, 1 << (size - 1).bit_length())
        flags = (BSS & ~0xF00000) | (alignment.bit_length() << 20)
        sections.append(Section('.bss', b'', flags, size=size))
        symbol.section, symbol.value = len(sections), 0
    return Object(sections, symbols)


class Object:
    def __init__(self, sections, symbols):
        self.sections = sections
        self.symbols = symbols

    @classmethod
    def read(cls, path):
        return cls.parse(Path(path).read_bytes(), path)

    @classmethod
    def parse(cls, data, path='<object>'):
        """Read an object from bytes (an archive member, say); `path` only labels errors."""
        if len(data) < 20:
            raise ValueError(f'{path}: truncated COFF header')
        machine, count, _, symptr, symcount, optional, _ = struct.unpack_from('<HHIIIHH', data)
        if machine != 0x14C or optional:
            raise ValueError(f'{path}: expected an i386 COFF object')
        if 20 + 40 * count > len(data) or symptr + 18 * symcount + 4 > len(data):
            raise ValueError(f'{path}: truncated COFF tables')
        strptr = symptr + 18 * symcount
        strlen = struct.unpack_from('<I', data, strptr)[0]
        if strlen < 4 or strptr + strlen > len(data):
            raise ValueError(f'{path}: invalid COFF string table')
        strings = data[strptr:strptr + strlen]

        def string(offset):
            if not 4 <= offset < len(strings):
                raise ValueError(f'{path}: invalid COFF string offset {offset}')
            end = strings.find(b'\0', offset)
            if end < 0:
                raise ValueError(f'{path}: unterminated COFF name')
            return strings[offset:end].decode('ascii')

        sections = []
        for i in range(count):
            name, _, _, size, raw, relptr, _, relcount, _, flags = struct.unpack_from(
                '<8sIIIIIIHHI', data, 20 + i * 40)
            name = name.rstrip(b'\0').decode('ascii')
            if name.startswith('/'):
                name = string(int(name[1:]))
            if flags & 0x1000000:
                raise ValueError(f'{path}: relocation overflow is not supported')
            if (raw and raw + size > len(data)) or (relcount and (not relptr or relptr + relcount * 10 > len(data))):
                raise ValueError(f'{path}: truncated section {i + 1}')
            relocs = [struct.unpack_from('<IIH', data, relptr + j * 10) for j in range(relcount)]
            if flags & 0x80:
                if raw or relcount:
                    raise ValueError(f'{path}: BSS has initialized bytes or relocations')
                payload = b''
            else:
                if size and not raw:
                    raise ValueError(f'{path}: initialized section has no payload')
                payload = data[raw:raw + size]
            sections.append(Section(name, payload, flags, relocs, size))
        symbols = {}
        i = 0
        while i < symcount:
            name, value, section, typ, storage, aux = struct.unpack_from('<8sIhHBB', data, symptr + i * 18)
            name = string(struct.unpack_from('<I', name, 4)[0]) if name[:4] == bytes(4) else name.rstrip(b'\0').decode('ascii')
            if section > count or i + aux >= symcount:
                raise ValueError(f'{path}: invalid symbol {name}')
            symbols[i] = Symbol(name, value, section, typ, storage)
            i += aux + 1
        for section in sections:
            for offset, index, _ in section.relocations:
                if offset >= len(section.data) or index not in symbols:
                    raise ValueError(f'{path}: invalid relocation in {section.name}')
        return cls(sections, symbols)

    def function(self, name):
        found = [s for s in self.symbols.values() if s.section > 0 and s.type & 0x20
                 and undecorate(s.name) == name]
        if len(found) != 1:
            raise ValueError(f'Expected one definition of {name}, found {len(found)}')
        return found[0]


def encode(sections, symbols):
    """Write objects without auxiliary symbols; relocation indices index symbols."""
    if len(sections) > 32767:
        raise ValueError('Too many sections for ordinary COFF')
    strings = bytearray(bytes(4))

    def name_field(name, section=False):
        encoded = name.encode('ascii')
        if len(encoded) <= 8:
            return encoded.ljust(8, b'\0')
        offset = len(strings)
        strings.extend(encoded + b'\0')
        return (f'/{offset}'.encode().ljust(8, b'\0') if section else struct.pack('<II', 0, offset))

    headers = bytearray()
    payload = bytearray()
    start = 20 + 40 * len(sections)
    for section in sections:
        section.__post_init__()
        if len(section.relocations) > 65535:
            raise ValueError('Too many relocations in one section')
        raw = 0 if section.is_bss else start + len(payload)
        payload.extend(section.data)
        relptr = start + len(payload) if section.relocations else 0
        for offset, index, typ in section.relocations:
            if index >= len(symbols):
                raise ValueError('Invalid relocation symbol index')
            payload.extend(struct.pack('<IIH', offset, index, typ))
        headers.extend(struct.pack('<8sIIIIIIHHI', name_field(section.name, True), 0, 0,
                                   section.size, raw, relptr, 0,
                                   len(section.relocations), 0, section.flags))
    symptr = start + len(payload)
    for sym in symbols:
        payload.extend(struct.pack('<8sIhHBB', name_field(sym.name), sym.value,
                                   sym.section, sym.type, sym.storage, 0))
    struct.pack_into('<I', strings, 0, len(strings))
    return (struct.pack('<HHIIIHH', 0x14C, len(sections), 0, symptr, len(symbols), 0, 0)
            + headers + payload + strings)


def relocate(data, relocations, symbols, address, resolve, image_base=0x400000):
    """Apply supported VC6 i386 relocations, failing on unresolved references."""
    result = bytearray(data)
    occupied = set()
    for offset, index, typ in relocations:
        if typ == 0:  # IMAGE_REL_I386_ABSOLUTE is a no-op.
            continue
        if typ not in (DIR32, DIR32NB, REL32):
            raise ValueError(f'Unsupported i386 relocation 0x{typ:04X}')
        if offset + 4 > len(result) or occupied.intersection(range(offset, offset + 4)):
            raise ValueError('Relocation lies outside the selected bytes or overlaps another relocation')
        occupied.update(range(offset, offset + 4))
        value = struct.unpack_from('<I', result, offset)[0] + resolve(symbols[index])
        if typ == REL32:
            value -= address + offset + 4
        elif typ == DIR32NB:
            value -= image_base
        struct.pack_into('<I', result, offset, value & 0xFFFFFFFF)
    return bytes(result)
