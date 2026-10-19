"""Compile a resource script into the image's .rsrc section.

Two stages, mirroring the period tools: the resource compiler (RC.EXE) turns the script into a
list of resources in script order, and CVTRES/LINK lay them out as the section: the directory
tree (types, names, languages; IDs ascending), the data entries, then every resource's data in
script order, each on an 8-byte boundary.  Only the statements Indy3D's script uses are
supported: LANGUAGE, DIALOG (with STYLE, EXSTYLE, CAPTION, FONT, CLASS, MENU and the control
statements), ICON files and DLGINIT.  ``#include`` of the Windows headers is accepted and the
style constants are built in; other preprocessor directives are rejected.

  python -m Scripts.pe_rsrc --rc Jones3D/Indy3D.rc --icons <directory> --address 0x01861000 \\
      --size 0x28800 --output <file>
  python -m Scripts.pe_rsrc --extract-icons orig/Indy3D.exe --sha256 <hex> --icons <directory> \\
      --output <stamp>

The icons are game artwork and are not in the repository: they are a *user-supplied asset*,
extracted at build time from the user's own executable (``--extract-icons``).  ``asset_bytes``
counts how many bytes of the section are that asset (the RT_ICON image data), so the image
builder can report them apart and keep them out of every progress figure (Scripts/pe_generate.py).
"""
import argparse
import hashlib
from pathlib import Path
import re
import struct

RT_ICON, RT_DIALOG, RT_GROUP_ICON, RT_DLGINIT = 3, 5, 14, 240

STYLE_NAMES = {
    # window styles
    'WS_OVERLAPPED': 0, 'WS_POPUP': 0x80000000, 'WS_CHILD': 0x40000000, 'WS_MINIMIZE': 0x20000000,
    'WS_VISIBLE': 0x10000000, 'WS_DISABLED': 0x08000000, 'WS_CLIPSIBLINGS': 0x04000000,
    'WS_CLIPCHILDREN': 0x02000000, 'WS_MAXIMIZE': 0x01000000, 'WS_CAPTION': 0x00C00000,
    'WS_BORDER': 0x00800000, 'WS_DLGFRAME': 0x00400000, 'WS_VSCROLL': 0x00200000, 'WS_HSCROLL': 0x00100000,
    'WS_SYSMENU': 0x00080000, 'WS_THICKFRAME': 0x00040000, 'WS_GROUP': 0x00020000, 'WS_TABSTOP': 0x00010000,
    'WS_MINIMIZEBOX': 0x00020000, 'WS_MAXIMIZEBOX': 0x00010000,
    'WS_EX_DLGMODALFRAME': 0x1, 'WS_EX_NOPARENTNOTIFY': 0x4, 'WS_EX_TOPMOST': 0x8, 'WS_EX_ACCEPTFILES': 0x10,
    'WS_EX_TRANSPARENT': 0x20, 'WS_EX_TOOLWINDOW': 0x80, 'WS_EX_WINDOWEDGE': 0x100, 'WS_EX_CLIENTEDGE': 0x200,
    'WS_EX_CONTEXTHELP': 0x400, 'WS_EX_RIGHT': 0x1000, 'WS_EX_STATICEDGE': 0x20000, 'WS_EX_APPWINDOW': 0x40000,
    # dialog styles
    'DS_ABSALIGN': 0x1, 'DS_SYSMODAL': 0x2, 'DS_3DLOOK': 0x4, 'DS_FIXEDSYS': 0x8, 'DS_NOFAILCREATE': 0x10,
    'DS_LOCALEDIT': 0x20, 'DS_SETFONT': 0x40, 'DS_MODALFRAME': 0x80, 'DS_NOIDLEMSG': 0x100,
    'DS_SETFOREGROUND': 0x200, 'DS_CONTROL': 0x400, 'DS_CENTER': 0x800, 'DS_CENTERMOUSE': 0x1000,
    'DS_CONTEXTHELP': 0x2000,
    # static
    'SS_LEFT': 0, 'SS_CENTER': 1, 'SS_RIGHT': 2, 'SS_ICON': 3, 'SS_BLACKRECT': 4, 'SS_GRAYRECT': 5,
    'SS_WHITERECT': 6, 'SS_BLACKFRAME': 7, 'SS_GRAYFRAME': 8, 'SS_WHITEFRAME': 9, 'SS_USERITEM': 0xA,
    'SS_SIMPLE': 0xB, 'SS_LEFTNOWORDWRAP': 0xC, 'SS_OWNERDRAW': 0xD, 'SS_BITMAP': 0xE, 'SS_ENHMETAFILE': 0xF,
    'SS_ETCHEDHORZ': 0x10, 'SS_ETCHEDVERT': 0x11, 'SS_ETCHEDFRAME': 0x12, 'SS_NOPREFIX': 0x80,
    'SS_NOTIFY': 0x100, 'SS_CENTERIMAGE': 0x200, 'SS_RIGHTJUST': 0x400, 'SS_REALSIZEIMAGE': 0x800,
    'SS_SUNKEN': 0x1000,
    # button
    'BS_PUSHBUTTON': 0, 'BS_DEFPUSHBUTTON': 1, 'BS_CHECKBOX': 2, 'BS_AUTOCHECKBOX': 3, 'BS_RADIOBUTTON': 4,
    'BS_3STATE': 5, 'BS_AUTO3STATE': 6, 'BS_GROUPBOX': 7, 'BS_USERBUTTON': 8, 'BS_AUTORADIOBUTTON': 9,
    'BS_OWNERDRAW': 0xB, 'BS_LEFTTEXT': 0x20, 'BS_TEXT': 0, 'BS_ICON': 0x40, 'BS_BITMAP': 0x80,
    'BS_LEFT': 0x100, 'BS_RIGHT': 0x200, 'BS_CENTER': 0x300, 'BS_TOP': 0x400, 'BS_BOTTOM': 0x800,
    'BS_VCENTER': 0xC00, 'BS_PUSHLIKE': 0x1000, 'BS_MULTILINE': 0x2000, 'BS_NOTIFY': 0x4000, 'BS_FLAT': 0x8000,
    # edit
    'ES_LEFT': 0, 'ES_CENTER': 1, 'ES_RIGHT': 2, 'ES_MULTILINE': 4, 'ES_UPPERCASE': 8, 'ES_LOWERCASE': 0x10,
    'ES_PASSWORD': 0x20, 'ES_AUTOVSCROLL': 0x40, 'ES_AUTOHSCROLL': 0x80, 'ES_NOHIDESEL': 0x100,
    'ES_OEMCONVERT': 0x400, 'ES_READONLY': 0x800, 'ES_WANTRETURN': 0x1000, 'ES_NUMBER': 0x2000,
    # list box
    'LBS_NOTIFY': 0x1, 'LBS_SORT': 0x2, 'LBS_NOREDRAW': 0x4, 'LBS_MULTIPLESEL': 0x8,
    'LBS_OWNERDRAWFIXED': 0x10, 'LBS_OWNERDRAWVARIABLE': 0x20, 'LBS_HASSTRINGS': 0x40,
    'LBS_USETABSTOPS': 0x80, 'LBS_NOINTEGRALHEIGHT': 0x100, 'LBS_MULTICOLUMN': 0x200,
    'LBS_WANTKEYBOARDINPUT': 0x400, 'LBS_EXTENDEDSEL': 0x800, 'LBS_DISABLENOSCROLL': 0x1000,
    'LBS_NODATA': 0x2000, 'LBS_NOSEL': 0x4000,
    # combo box
    'CBS_SIMPLE': 0x1, 'CBS_DROPDOWN': 0x2, 'CBS_DROPDOWNLIST': 0x3, 'CBS_OWNERDRAWFIXED': 0x10,
    'CBS_OWNERDRAWVARIABLE': 0x20, 'CBS_AUTOHSCROLL': 0x40, 'CBS_OEMCONVERT': 0x80, 'CBS_SORT': 0x100,
    'CBS_HASSTRINGS': 0x200, 'CBS_NOINTEGRALHEIGHT': 0x400, 'CBS_DISABLENOSCROLL': 0x800,
    'CBS_UPPERCASE': 0x2000, 'CBS_LOWERCASE': 0x4000,
    # scroll bar
    'SBS_HORZ': 0, 'SBS_VERT': 0x1, 'SBS_TOPALIGN': 0x2, 'SBS_LEFTALIGN': 0x2, 'SBS_BOTTOMALIGN': 0x4,
    'SBS_RIGHTALIGN': 0x4, 'SBS_SIZEBOX': 0x8, 'SBS_SIZEGRIP': 0x10,
    # common controls (commctrl.h)
    'TBS_AUTOTICKS': 0x1, 'TBS_VERT': 0x2, 'TBS_HORZ': 0, 'TBS_TOP': 0x4, 'TBS_BOTTOM': 0, 'TBS_LEFT': 0x4,
    'TBS_RIGHT': 0, 'TBS_BOTH': 0x8, 'TBS_NOTICKS': 0x10, 'TBS_ENABLESELRANGE': 0x20, 'TBS_FIXEDLENGTH': 0x40,
    'TBS_NOTHUMB': 0x80, 'TBS_TOOLTIPS': 0x100,
    'LVS_ICON': 0, 'LVS_REPORT': 0x1, 'LVS_SMALLICON': 0x2, 'LVS_LIST': 0x3, 'LVS_SINGLESEL': 0x4,
    'LVS_SHOWSELALWAYS': 0x8, 'LVS_SORTASCENDING': 0x10, 'LVS_SORTDESCENDING': 0x20,
    'LVS_SHAREIMAGELISTS': 0x40, 'LVS_NOLABELWRAP': 0x80, 'LVS_AUTOARRANGE': 0x100, 'LVS_EDITLABELS': 0x200,
    'LVS_OWNERDRAWFIXED': 0x400, 'LVS_ALIGNTOP': 0, 'LVS_ALIGNLEFT': 0x800, 'LVS_OWNERDATA': 0x1000,
    'LVS_NOSCROLL': 0x2000, 'LVS_NOCOLUMNHEADER': 0x4000, 'LVS_NOSORTHEADER': 0x8000,
    'UDS_WRAP': 0x1, 'UDS_SETBUDDYINT': 0x2, 'UDS_ALIGNRIGHT': 0x4, 'UDS_ALIGNLEFT': 0x8,
    'UDS_AUTOBUDDY': 0x10, 'UDS_ARROWKEYS': 0x20, 'UDS_HORZ': 0x40, 'UDS_NOTHOUSANDS': 0x80,
    'PBS_SMOOTH': 0x1, 'PBS_VERTICAL': 0x4,
    # LANGUAGE
    'LANG_NEUTRAL': 0, 'LANG_ENGLISH': 0x09, 'SUBLANG_NEUTRAL': 0, 'SUBLANG_DEFAULT': 1, 'SUBLANG_ENGLISH_US': 1,
}

# The predefined control classes RC stores as ordinals, and the control statements' defaults.
CLASS_ORDINALS = {'button': 0x80, 'edit': 0x81, 'static': 0x82, 'listbox': 0x83, 'scrollbar': 0x84,
                  'combobox': 0x85}
WS_CHILD_VISIBLE = 0x50000000
CONTROLS = {  # statement: (class ordinal, default style, has text)
    'LTEXT': (0x82, 0x00020000, True), 'CTEXT': (0x82, 0x00020001, True), 'RTEXT': (0x82, 0x00020002, True),
    'ICON': (0x82, 0x3, True),
    'PUSHBUTTON': (0x80, 0x00010000, True), 'DEFPUSHBUTTON': (0x80, 0x00010001, True),
    'GROUPBOX': (0x80, 0x7, True), 'CHECKBOX': (0x80, 0x00010002, True),
    'AUTOCHECKBOX': (0x80, 0x00010003, True), 'RADIOBUTTON': (0x80, 0x4, True),
    'AUTORADIOBUTTON': (0x80, 0x9, True), 'STATE3': (0x80, 0x00010005, True),
    'EDITTEXT': (0x81, 0x00810000, False), 'LISTBOX': (0x83, 0x00800001, False),
    'COMBOBOX': (0x85, 0x00010001, False), 'SCROLLBAR': (0x84, 0x0, False),
}

TOKEN = re.compile(r'''\s+|//[^\n]*|/\*.*?\*/|(?P<string>"(?:[^"]|"")*")|(?P<number>0[xX][0-9A-Fa-f]+[lL]?|\d+[lL]?)
                       |(?P<name>[A-Za-z_][A-Za-z0-9_.]*)|(?P<punct>[,|(){}+-])''', re.S | re.X)


def tokens(text):
    lines = []
    for line in text.splitlines():
        stripped = line.strip()
        if stripped.startswith('#'):
            if not re.fullmatch(r'#\s*include\s*<(windows|commctrl|winresrc)\.h>', stripped):
                raise ValueError(f'unsupported preprocessor directive: {stripped}')
            line = ''
        lines.append(line)
    text, position, out = '\n'.join(lines), 0, []
    while position < len(text):
        match = TOKEN.match(text, position)
        if not match:
            raise ValueError(f'resource script: cannot parse {text[position:position + 20]!r}')
        position = match.end()
        for kind in ('string', 'number', 'name', 'punct'):
            if match.group(kind) is not None:
                out.append((kind, match.group(kind)))
    return out


ESCAPES = {'n': '\n', 'r': '\r', 't': '\t', 'a': '\a', 'b': '\b', 'f': '\f', 'v': '\v', '\\': '\\', '"': '"'}


def unquote(token):
    body, out, i = token[1:-1].replace('""', '"'), [], 0
    while i < len(body):
        if body[i] == '\\' and i + 1 < len(body):
            octal = re.match(r'[0-7]{1,3}', body[i + 1:])
            if octal:
                out.append(chr(int(octal.group(), 8)))
                i += 1 + len(octal.group())
                continue
            out.append(ESCAPES.get(body[i + 1], '\\' + body[i + 1]))
            i += 2
            continue
        out.append(body[i])
        i += 1
    return ''.join(out)


class Parser:
    def __init__(self, text):
        self.tokens, self.position = tokens(text), 0

    def peek(self, offset=0):
        index = self.position + offset
        return self.tokens[index] if index < len(self.tokens) else (None, None)

    def take(self, kind=None, value=None):
        token = self.peek()
        if token[0] is None or (kind and token[0] != kind) or (value and token[1].upper() != value):
            raise ValueError(f'resource script: expected {value or kind}, found {token[1]!r}')
        self.position += 1
        return token[1]

    def accept(self, value):
        if self.peek()[1] is not None and self.peek()[1].upper() == value and self.peek()[0] in ('name', 'punct'):
            self.position += 1
            return True
        return False

    def number(self):
        """A constant expression: terms joined by | or +, each a number, a style name or NOT name."""
        value, negate_mask = 0, 0
        while True:
            negate = self.accept('NOT')
            sign = -1 if self.accept('-') else 1
            kind, text = self.peek()
            if kind == 'number':
                term = sign * int(text.rstrip('lL'), 0)
            elif kind == 'name' and text in STYLE_NAMES:
                term = STYLE_NAMES[text]
            else:
                raise ValueError(f'resource script: expected a number, found {text!r}')
            self.position += 1
            if negate:
                negate_mask |= term
            else:
                value |= term
            if not (self.accept('|') or self.accept('+')):
                return value & 0xFFFFFFFF, negate_mask

    def value(self):
        value, negate = self.number()
        if negate:
            raise ValueError('resource script: NOT outside a style')
        return value

    def comma(self):
        return self.accept(',')

    def text_or_ordinal(self):
        if self.peek()[0] == 'string':
            return unquote(self.take())
        return self.value()


def sz_or_ord(value):
    if isinstance(value, int):
        return struct.pack('<HH', 0xFFFF, value)
    return value.encode('utf-16-le') + b'\0\0'


def numbers(parser, count):
    values = [parser.value()]
    for _ in range(count - 1):
        parser.comma()
        values.append(parser.value())
    return values


def dialog(parser):
    x, y, width, height = numbers(parser, 4)
    style, explicit_style, exstyle, caption, font, menu, klass = 0, None, 0, '', None, 0, 0
    while not (parser.accept('BEGIN') or parser.accept('{')):
        keyword = parser.take('name').upper()
        if keyword == 'STYLE':
            value, negate = parser.number()
            explicit_style = value & ~negate
        elif keyword == 'EXSTYLE':
            exstyle = parser.value()
        elif keyword == 'CAPTION':
            caption = unquote(parser.take('string'))
        elif keyword == 'FONT':
            size = parser.value()
            parser.comma()
            font = (size, unquote(parser.take('string')))
        elif keyword == 'MENU':
            menu = parser.text_or_ordinal()
        elif keyword == 'CLASS':
            klass = parser.text_or_ordinal()
        else:
            raise ValueError(f'resource script: unsupported dialog statement {keyword}')
    style = 0x80880000 if explicit_style is None else explicit_style  # WS_POPUP | WS_BORDER | WS_SYSMENU
    if caption:
        style |= 0x00C00000
    if font:
        style |= 0x40
    items = []
    while not (parser.accept('END') or parser.accept('}')):
        keyword = parser.take('name').upper()
        if keyword == 'CONTROL':
            text = parser.text_or_ordinal()
            parser.comma()
            ident = parser.value()
            parser.comma()
            klass_name = parser.text_or_ordinal() if parser.peek()[0] == 'string' else parser.take('name')
            control_class = CLASS_ORDINALS.get(klass_name.lower(), klass_name) if isinstance(klass_name, str) else klass_name
            parser.comma()
            value, negate = parser.number()
            control_style = (WS_CHILD_VISIBLE | value) & ~negate
            parser.comma()
            geometry = numbers(parser, 4)
            control_exstyle = parser.value() if parser.comma() else 0
        elif keyword in CONTROLS:
            control_class, default, has_text = CONTROLS[keyword]
            text = parser.text_or_ordinal() if has_text else ''
            if has_text:
                parser.comma()
            ident = parser.value()
            parser.comma()
            geometry = numbers(parser, 4)
            value, negate, control_exstyle = 0, 0, 0
            if parser.comma():
                value, negate = parser.number()
                if keyword == 'COMBOBOX':
                    default &= ~0x00010000  # RC keeps a combo box's WS_TABSTOP only without a style
                if parser.comma():
                    control_exstyle = parser.value()
            control_style = (WS_CHILD_VISIBLE | default | value) & ~negate
        else:
            raise ValueError(f'resource script: unsupported control statement {keyword}')
        items.append((control_style, control_exstyle, geometry, ident, control_class, text))
    data = bytearray(struct.pack('<IIH4h', style, exstyle, len(items), x, y, width, height))
    data += sz_or_ord(menu) if menu else b'\0\0'
    data += sz_or_ord(klass) if klass else b'\0\0'
    data += sz_or_ord(caption)
    if font:
        data += struct.pack('<H', font[0]) + sz_or_ord(font[1])
    for control_style, control_exstyle, geometry, ident, control_class, text in items:
        data += bytes(-len(data) % 4)
        data += struct.pack('<II4hH', control_style, control_exstyle, *geometry, ident & 0xFFFF)
        data += sz_or_ord(control_class) + sz_or_ord(text) + b'\0\0'
    return bytes(data)


def dlginit(parser):
    data = bytearray()
    while not (parser.accept('END') or parser.accept('}')):
        kind, text = parser.peek()
        if kind == 'string':
            data += unquote(parser.take()).encode('cp1252')
        else:
            data += struct.pack('<I' if text.upper().endswith('L') else '<H', parser.value())
        parser.comma()
    return bytes(data)


def icon_file(path, next_icon):
    """RC's ICON statement: one RT_ICON per image (ordinals counted across the script) and the group."""
    data = path.read_bytes()
    reserved, kind, count = struct.unpack_from('<HHH', data)
    if reserved or kind != 1:
        raise ValueError(f'{path}: not an icon file')
    images, group = [], bytearray(struct.pack('<HHH', 0, 1, count))
    for i in range(count):
        width, height, colors, zero, planes, bits, size, offset = struct.unpack_from('<BBBBHHII', data, 6 + 16 * i)
        images.append((next_icon + i, data[offset:offset + size]))
        group += struct.pack('<BBBBHHIH', width, height, colors, zero, planes, bits, size, next_icon + i)
    return images, bytes(group)


def compile_script(path, icons):
    """RC.EXE: the script's resources in order, as (type, name, language, data)."""
    parser = Parser(Path(path).read_text(encoding='cp1252'))
    resources, language, next_icon = [], 0, 1
    while parser.peek()[0] is not None:
        if parser.accept('LANGUAGE'):
            primary = parser.value()
            parser.comma()
            language = primary | parser.value() << 10
            continue
        name = parser.text_or_ordinal() if parser.peek()[0] == 'number' else parser.take('name')
        if not isinstance(name, int):
            raise ValueError('resource script: only numeric resource IDs are supported')
        keyword = parser.take('name').upper()
        if keyword == 'DIALOG':
            resources.append((RT_DIALOG, name, language, dialog(parser)))
        elif keyword == 'DLGINIT':
            parser.accept('BEGIN') or parser.take('punct', '{')
            resources.append((RT_DLGINIT, name, language, dlginit(parser)))
        elif keyword == 'ICON':
            images, group = icon_file(Path(icons) / unquote(parser.take('string')), next_icon)
            next_icon += len(images)
            resources += [(RT_ICON, ident, language, image) for ident, image in images]
            resources.append((RT_GROUP_ICON, name, language, group))
        else:
            raise ValueError(f'resource script: unsupported resource type {keyword}')
    return resources


def asset_bytes(resources):
    """Bytes of the section that are icon image data (RT_ICON), i.e. the user-supplied artwork."""
    return sum(len(data) for kind, _, _, data in resources if kind == RT_ICON)


def resource_section(exe):
    """The .rsrc section of a PE image: (raw bytes, virtual address relative to the image base)."""
    pe = struct.unpack_from('<I', exe, 0x3C)[0]
    if exe[pe:pe + 4] != b'PE\0\0':
        raise ValueError('not a PE image')
    count, optional = struct.unpack_from('<H12xH', exe, pe + 6)
    table = pe + 24 + optional
    for i in range(count):
        name, _, rva, raw_size, raw = struct.unpack_from('<8sIIII', exe, table + 40 * i)
        if name.rstrip(b'\0') == b'.rsrc':
            return exe[raw:raw + raw_size], rva
    raise ValueError('image has no .rsrc section')


def extract_icons(exe, directory):
    """Write each RT_GROUP_ICON of the image as the .ico file RC read (icon<id>.ico): the group's
    directory entries with file offsets in place of image ordinals, then the RT_ICON images."""
    section, base = resource_section(exe)

    def entries(offset):
        count = sum(struct.unpack_from('<HH', section, offset + 12))
        return [struct.unpack_from('<II', section, offset + 16 + 8 * i) for i in range(count)]

    def leaf(offset):
        rva, size = struct.unpack_from('<II', section, entries(offset & 0x7FFFFFFF)[0][1])
        return section[rva - base:rva - base + size]

    types = dict(entries(0))
    images = {ident: leaf(target) for ident, target in entries(types[RT_ICON] & 0x7FFFFFFF)}
    written = []
    for ident, target in entries(types[RT_GROUP_ICON] & 0x7FFFFFFF):
        group = leaf(target)
        count = struct.unpack_from('<H', group, 4)[0]
        header, body = bytearray(group[:6]), bytearray()
        for i in range(count):
            *fields, image = struct.unpack_from('<BBBBHHIH', group, 6 + 14 * i)
            header += struct.pack('<BBBBHHII', *fields, 6 + 16 * count + len(body))
            body += images[image]
        path = Path(directory) / f'icon{ident}.ico'
        path.parent.mkdir(parents=True, exist_ok=True)
        if not path.exists() or path.read_bytes() != header + body:
            path.write_bytes(header + body)
        written.append(path)
    return written


def layout(resources, rva):
    """CVTRES/LINK: the .rsrc section at `rva` (relative to the image base)."""
    tree = {}
    for kind, name, language, data in resources:
        if language in tree.setdefault(kind, {}).setdefault(name, {}):
            raise ValueError(f'duplicate resource {kind}/{name}/{language}')
        tree[kind][name][language] = data

    def directory_size(entries):
        return 16 + 8 * entries

    # Breadth-first directories, then the data entries (in tree order), then the data (script order).
    offset = directory_size(len(tree))
    type_offsets, name_offsets = {}, {}
    for kind in sorted(tree):
        type_offsets[kind] = offset
        offset += directory_size(len(tree[kind]))
    for kind in sorted(tree):
        for name in sorted(tree[kind]):
            name_offsets[kind, name] = offset
            offset += directory_size(len(tree[kind][name]))
    entry_offsets = {}
    for kind in sorted(tree):
        for name in sorted(tree[kind]):
            for language in sorted(tree[kind][name]):
                entry_offsets[kind, name, language] = offset
                offset += 16
    data_offsets = {}
    for kind, name, language, data in resources:
        offset += -offset % 8
        data_offsets[kind, name, language] = offset
        offset += len(data)
    size = offset + (-offset % 8)

    section = bytearray(size)

    def directory(at, children):
        struct.pack_into('<IIHHHH', section, at, 0, 0, 0, 0, 0, len(children))
        for i, (ident, target, is_directory) in enumerate(children):
            struct.pack_into('<II', section, at + 16 + 8 * i, ident, target | (0x80000000 if is_directory else 0))

    directory(0, [(kind, type_offsets[kind], True) for kind in sorted(tree)])
    for kind in sorted(tree):
        directory(type_offsets[kind], [(name, name_offsets[kind, name], True) for name in sorted(tree[kind])])
        for name in sorted(tree[kind]):
            directory(name_offsets[kind, name], [(language, entry_offsets[kind, name, language], False)
                                                 for language in sorted(tree[kind][name])])
    for kind, name, language, data in resources:
        at = data_offsets[kind, name, language]
        struct.pack_into('<IIII', section, entry_offsets[kind, name, language], rva + at, len(data), 0, 0)
        section[at:at + len(data)] = data
    return bytes(section)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--rc', type=Path)
    parser.add_argument('--icons', type=Path, required=True, help='directory holding the script\'s icon files')
    parser.add_argument('--address', type=lambda v: int(v, 0), help='section virtual address')
    parser.add_argument('--image-base', type=lambda v: int(v, 0), default=0x400000)
    parser.add_argument('--size', type=lambda v: int(v, 0), help='file size of the section (zero fill)')
    parser.add_argument('--extract-icons', type=Path, metavar='EXE',
                        help='extract the icon files from the user\'s executable instead of compiling')
    parser.add_argument('--sha256', help='required SHA-256 of the executable given to --extract-icons')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    try:
        if args.extract_icons:
            exe = args.extract_icons.read_bytes()
            digest = hashlib.sha256(exe).hexdigest()
            if not args.sha256 or digest != args.sha256.lower():
                raise ValueError(f'{args.extract_icons}: SHA-256 {digest} is not the supported executable')
            files = extract_icons(exe, args.icons)
            stamp = ''.join(f'{hashlib.sha256(f.read_bytes()).hexdigest()}  {f.name}\n' for f in files)
            args.output.parent.mkdir(parents=True, exist_ok=True)
            if not args.output.exists() or args.output.read_text() != stamp:
                args.output.write_text(stamp)
            return
        if not args.rc or args.address is None:
            parser.error('--rc and --address are required to compile a script')
        resources = compile_script(args.rc, args.icons)
        data = layout(resources, args.address - args.image_base)
        if args.size is not None:
            if len(data) > args.size:
                raise ValueError(f'resources need 0x{len(data):X} bytes, more than 0x{args.size:X}')
            data += bytes(args.size - len(data))
    except (ValueError, OSError) as exc:
        parser.exit(1, f'error: {exc}\n')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(data)


if __name__ == '__main__':
    main()
