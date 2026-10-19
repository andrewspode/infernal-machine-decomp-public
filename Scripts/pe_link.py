"""Strict placement and relocation of whole VC6 translation units.

All runtime sections must be owned or explicitly shared COMDATs. This module
never reads reference object bytes and never substitutes individual functions.
"""
import hashlib
from dataclasses import replace
import re

from . import coff
from tools.config import SECTION_EXTRA_FLAGS, SECTION_KINDS


EMPTY_ANSI_STRING = '??_C@_00A@?$AA@'
LNK_REMOVE = 0x800  # COFF section flag: the linker drops the section from the image


def digest(data):
    return hashlib.sha256(data).hexdigest()


def align_up(value, align):
    return (value + align - 1) & -align


# Intel C/C++ 4.0's static labels inside a function's own COMDAT: `name$$N` (the second entry of an
# aligned-stack prologue), `$$N$M_K$except` / `$$N$M_K$filter` (the handler and filter blocks of a
# `__try`), `TAG_PACKET_N` (a label inside an `__asm` block).
ICL_CODE_LABELS = (r'(.+)\$\$\d+', r'\$\$\d+\$\d+_\d+\$(?:except|filter)', r'TAG_PACKET_\d+')


def entry_alias_base(obj, symbol):
    """The function an Intel internal code label belongs to, else None.

    Intel C/C++ 4.0 gives every function with an aligned-stack prologue two
    entries: `_f` and a static `_f$$1` a few bytes in (the prologue's second
    `push ebx; mov ebx, esp`, docs/crt.md). It, the `__try` handler/filter
    blocks and the labels of an `__asm` block are static function-typed symbols
    inside the function's own COMDAT, not other functions: the external
    function symbol of the same section at a lower or equal offset owns them.
    """
    if symbol.section <= 0 or not symbol.type & 0x20 or symbol.storage != 3:
        return None
    match = re.fullmatch(ICL_CODE_LABELS[0], symbol.name)
    if match:
        return next((s for s in obj.symbols.values() if s.name == match.group(1) and s.section == symbol.section
                     and s.type & 0x20 and s.value < symbol.value), None)
    if any(re.fullmatch(pattern, symbol.name) for pattern in ICL_CODE_LABELS[1:]):
        return next((s for s in obj.symbols.values() if s.section == symbol.section and s.type & 0x20
                     and s.storage == 2 and s.value < symbol.value), None)
    return None


def is_library(project, unit):
    """Whether `unit` is a library unit (an archive member, libraries.yml)."""
    return unit in getattr(project, 'libraries', ())


def bind_local_data(project, unit, obj):
    """Bind VC5's unstable private names without consulting payload bytes.

    Explicit owned, file-local aliases ignore only the $S compiler serial.
    For anonymous $T symbols, a single ordinary data section and a single
    owning split establish an unambiguous offset mapping. COMDATs and
    discontiguous/ambiguous layouts need other anchors. Sizes, storage,
    coverage and bytes still face full checks.
    Only in-memory symbol names change; native objects remain untouched.
    """
    symbols = {i: replace(s) for i, s in obj.symbols.items()}
    aliases = [s['coff_name'] for s in project.units[unit]
               if s['kind'] == 'data' and s.get('scope') == 'local' and s.get('coff_name')]
    for symbol in symbols.values():
        private = re.fullmatch(r'(.+)\$S\d+', symbol.name)
        if symbol.section > 0 and symbol.storage == 3 and private:
            alias = private.group(1)
            if aliases.count(alias) > 1:
                raise ValueError(f'{unit}: ambiguous private COFF identity {alias}')
            if alias in aliases:
                symbol.name = alias
    # Intel C 4.0 names a function-scope static `<name>_<serial>` (rdModel3K_Flush's `i` is
    # `i_7196`), the serial counting every symbol before it, so any header edit moves it. Like
    # VC5's `$S`, the serial is ignored: an owned local alias binds to the object's only static
    # definition spelled `<alias>_<digits>`, unless the object also defines the alias itself.
    names = {s.name for s in symbols.values()}
    for alias in sorted(set(aliases) - names):
        serial = re.compile(re.escape(alias) + r'_\d+')
        matches = [s for s in symbols.values()
                   if s.section > 0 and s.storage == 3 and serial.fullmatch(s.name)]
        if len(matches) > 1:
            raise ValueError(f'{unit}: ambiguous serial COFF identity {alias}: '
                             + ', '.join(sorted(s.name for s in matches)))
        if matches:
            matches[0].name = alias
    # A library member's symbols.txt entries name its `$T` constants exactly
    # (their coff_name is the member's own symbol), so no offset binding.
    for kind in () if is_library(project, unit) else ('.rdata', '.data'):
        parts = [p for p in project.unit_splits[unit] if p['section'] == kind]
        sections = [(i, s) for i, s in enumerate(obj.sections, 1)
                    if s.name == kind and s.size and not s.flags & 0x1000]
        if len(parts) != 1 or len(sections) != 1:
            continue
        index, section = sections[0]
        # Offsets count from the first contribution: a declared `lead` is LINK's
        # alignment fill before it, not storage of this object.
        first = parts[0]['start'] + parts[0].get('lead', 0)
        by_offset = {s['address'] - first: s for s in project.units[unit]
                     if s['kind'] == 'data' and s['section'] == kind}
        for symbol in symbols.values():
            if (symbol.section == index and symbol.storage == 3
                    and re.fullmatch(r'\$T\d+', symbol.name) and symbol.value in by_offset):
                datum = by_offset[symbol.value]
                symbol.name = datum.get('coff_name') or '_' + datum['name']
    return coff.Object(obj.sections, symbols)


def shared_symbols(project, unit, obj):
    """Resolve routine string pooling by exact COFF identity; overrides handle ambiguity."""
    shared = dict(project.matching.get('shared', {}).get(unit, {}))
    owned_keys = {s.get('coff_name') for s in project.units[unit]}
    by_key = {}
    for datum in project.symbols.values():
        if (datum['kind'] == 'data' and datum.get('source') and datum['source'] != unit
                and coff.is_string(datum.get('coff_name', ''))):
            by_key.setdefault(datum['coff_name'], []).append(datum)
    for symbol in obj.symbols.values():
        if symbol.section <= 0 or symbol.name in owned_keys or symbol.name in shared:
            continue
        candidates = by_key.get(symbol.name, [])
        if len(candidates) == 1 and candidates[0]['bounds'] == 'reviewed':
            shared[symbol.name] = candidates[0]['name']
    return shared


def folded_bodies(project, unit):
    """Declared identical-COMDAT folds for this unit.

    Retail LINK folded two byte-identical function COMDATs (normally a
    return-only body or a one-call wrapper) onto a single address, so an
    object can define a function that owns no retail bytes of its own: the
    surviving copy belongs to another TU, or to a module no TU owns yet.
    Deleting the definition is not an option when the same object references
    it — `RegisterFunctions` pushes the folded address, and that reference has
    to resolve.

    `relocations.yml` `folded:` maps the compiled COFF name either to the
    retail symbol whose body it shares, or — when the surviving body sits in a
    module no TU owns, so there is no symbol to name — to an explicit
    `{address, size, evidence}` entry. The declaration is only a claim; the
    strict linker verifies it (see compiled_unit): a named owner must be a
    reviewed retail function, of another TU or of this one (LINK folds two
    identical COMDATs of one object too; the surviving body must then be this
    object's own, separately placed definition of the owner, and the folded
    section's relocated bytes must equal that section's); an address target must be unowned
    (checked in pe_project) and its `size` must equal the extent measured in
    the retail image; the compiled COMDAT must define the folded name at
    offset 0 over exactly that extent; and the relocated bytes must equal the
    retail bytes at the target address. The folded section supplies no image
    storage and is not part of any split, so it can never shift another
    function.
    """
    return dict((project.matching.get('folded') or {}).get(unit, {}))


def unreferenced_comdats(project, unit, obj, by_name, shared, folded):
    """Model LINK /OPT:REF removal of unreferenced COMDATs with no retail extent.

    Retail can lack a function (and its unique /Gf strings) that the original
    source defined: VC5 packages every function and literal as a COMDAT and
    the linker drops those nothing references. A section is only discarded
    when it is a COMDAT, none of its definitions has any retail identity
    (no symbols.txt entry by name or COFF identity, no shared pooling key, no
    declared folded owner and no relocations.yml address), and no retained section of this object
    reaches it through relocations. Only such a function, and data it alone
    references, is dropped. Everything with a retail identity is a
    root and stays fully checked, so a referenced or retail-present function
    is never dropped. Another TU cannot reference a discarded definition
    either: without a retail address its reference cannot resolve.

    Owner policy: every dropped function must also be listed for the unit in
    relocations.yml `discarded`, next to the retail evidence that it existed
    (an orphaned literal, a __LINE__ gap, a folded call target). An unlisted
    droppable function is an error (a fork leftover), and so is a listed name
    the object does not drop.
    """
    overrides = project.matching.get('symbols', {})
    known = {s.get('coff_name') for s in project.symbols.values()} - {None}
    runtime, candidates = [], set()
    for index, section in enumerate(obj.sections, 1):
        if (section.name == '.drectve' or section.name.startswith('.debug$')
                or section.flags & LNK_REMOVE or (not section.size and not section.relocations)):
            continue
        runtime.append(index)
        names = [s.name for s in obj.symbols.values() if s.section == index
                 and not s.name.startswith('.') and not s.name.startswith('$L')]
        if not section.flags & 0x1000 or not names:
            continue
        exact = is_library(project, unit)  # library symbols carry exact COFF names
        if any(name in by_name or name in shared or name in folded or name in known
               or name in overrides or unit + ':' + name in overrides
               or (not exact and (coff.undecorate(name) in by_name or coff.undecorate(name) in project.symbols))
               for name in names):
            continue
        candidates.add(index)
    defined = {s.name: s.section for s in obj.symbols.values() if s.section > 0}
    reached = [i for i in runtime if i not in candidates]
    kept = set()
    while reached:
        for _, symbol_index, _ in obj.sections[reached.pop() - 1].relocations:
            symbol = obj.symbols[symbol_index]
            target = symbol.section if symbol.section > 0 else defined.get(symbol.name)
            if target in candidates and target not in kept:
                kept.add(target)
                reached.append(target)
    # Only discarded functions (and what they alone reference) may vanish.
    # An isolated data COMDAT that nothing references at all stays an error:
    # C never emits one, so it indicates a stray or wrong native definition.
    unreached = candidates - kept
    library = is_library(project, unit)
    # A library member is not source whose functions need evidence of having
    # existed: LINK /OPT:REF simply drops whatever COMDAT nothing references.
    dropped = [i for i in unreached if library or obj.sections[i - 1].name.split('$', 1)[0] == '.text']
    discarded = set(dropped)
    while dropped:
        for _, symbol_index, _ in obj.sections[dropped.pop() - 1].relocations:
            symbol = obj.symbols[symbol_index]
            target = symbol.section if symbol.section > 0 else defined.get(symbol.name)
            if target in unreached and target not in discarded:
                discarded.add(target)
                dropped.append(target)
    allowed = set((project.matching.get('discarded') or {}).get(unit, []))
    functions = {s.name for s in obj.symbols.values()
                 if s.section in discarded and s.type & 0x20 and not entry_alias_base(obj, s)}
    if library:
        functions = allowed
    for name in sorted(functions - allowed):
        raise ValueError(f'{unit}: unreferenced function {name} has no retail identity and is not '
                         'listed as discarded in relocations.yml')
    for name in sorted(allowed - functions):
        raise ValueError(f'{unit}: discarded function {name} listed in relocations.yml is not an '
                         'unreferenced function of the compiled object')
    return discarded


def library_commons(project, unit, obj, by_name):
    """A library member's COMMON request that another unit's object satisfies.

    LIBC members declare shared runtime state as COMMON (`__FPinit`,
    `__matherr_flag`); LINK merges the request with the initialized definition
    another member supplies, or allocates one block for several requesters. A
    request this unit does not own is therefore an ordinary external reference
    when the project names the storage elsewhere, at least as large. The
    storage itself is still verified by the unit that owns it.
    """
    symbols = {i: replace(s) for i, s in obj.symbols.items()}
    for symbol in symbols.values():
        if symbol.section or not symbol.value:
            continue
        if by_name.get(symbol.name) or by_name.get(coff.undecorate(symbol.name)):
            continue
        other = project.symbols.get(coff.undecorate(symbol.name))
        if other and other['kind'] == 'data' and other['source'] not in ('', unit) and other['size'] >= symbol.value:
            symbol.value = 0
    return coff.Object(obj.sections, symbols)


def compiled_unit(project, unit, obj, layout):
    obj = bind_local_data(project, unit, obj)
    parts = project.unit_splits[unit]
    owned = project.units[unit]
    references = [r for r in layout['ranges'] if r['source'] == unit]
    if len(references) != len(parts):
        raise ValueError(f'{unit}: reference layout does not contain every TU split')
    library = is_library(project, unit)
    # A library symbol's identity is exact: its coff_name, else the default
    # C decoration of its name. Game TUs keep the undecorating lookups below.
    by_name = {s.get('coff_name') or ('_' + s['name'] if library else s['name']): s for s in owned}
    if library:
        obj = library_commons(project, unit, obj, by_name)
    for symbol in obj.symbols.values():
        if symbol.section == 0 and symbol.value:
            original = by_name.get(symbol.name) or by_name.get(coff.undecorate(symbol.name))
            if (not original or original['kind'] != 'data' or original['section'] != '.bss'
                    or original['scope'] != 'global' or original['size'] != symbol.value):
                raise ValueError(f'{unit}: common symbol {symbol.name} needs an exactly sized, owned global BSS datum')
    obj = coff.allocate_common(obj)
    definitions = {}
    for symbol in obj.symbols.values():
        if symbol.section > 0:
            if symbol.name in definitions and not symbol.name.startswith('.'):
                raise ValueError(f'{unit}: duplicate compiled definition {symbol.name}')
            definitions[symbol.name] = symbol
    shared = shared_symbols(project, unit, obj)
    folded = folded_bodies(project, unit)
    used_shared, used_folded = set(), set()
    discarded = unreferenced_comdats(project, unit, obj, by_name, shared, folded)
    placements, members, shared_sections = {}, [[] for _ in parts], []
    folded_sections, same_unit_folds = [], []
    for index, section in enumerate(obj.sections, 1):
        # VC6's debug records and linker directives do not occupy image bytes.
        if section.name == '.drectve' or section.name.startswith('.debug$') or index in discarded:
            continue
        if section.flags & LNK_REMOVE:
            continue  # ICL's `.comment` (like `.drectve`): removed by LINK, never in the image
        if not section.size and not section.relocations:
            continue
        kind = section.name.split('$', 1)[0]
        if library and kind == '.CRT':
            kind = '.data'  # LIBC's initializer tables; LINK merges .CRT$X* into .data
        if kind not in SECTION_KINDS:
            raise ValueError(f'{unit}: unsupported runtime section {section.name}')
        if (section.is_bss != (kind == '.bss') or bool(section.flags & 0x20) != (SECTION_KINDS[kind] == 'code')
                or bool(section.flags & 0x80000000) != (kind in ('.data', '.data1', '.bss'))):
            raise ValueError(f'{unit}: inconsistent storage flags for {section.name}')
        symbols = [s for s in obj.symbols.values() if s.section == index]
        # Library symbols carry their exact COFF name: `__tolower` must not
        # anchor to `_tolower`'s entry through undecoration.
        anchors = [(s, by_name.get(s.name) or (None if library else by_name.get(coff.undecorate(s.name))))
                   for s in symbols]
        anchors = [(s, original) for s, original in anchors if original]
        fold_keys = [s.name for s in symbols if s.name in folded]
        if fold_keys:
            # A folded body is not storage this TU owns, so it must be a
            # COMDAT of its own with no owned or pooled definition beside it.
            if len(fold_keys) != 1 or anchors or any(s.name in shared for s in symbols):
                raise ValueError(f'{unit}: folded body must be the only definition of its COMDAT '
                                 f'in section {index} ({section.name})')
            mapped_names = set(fold_keys)
        else:
            mapped_names = {s.name for s, _ in anchors} | set(shared)
        for symbol in symbols:
            if (symbol.storage in (2, 3) and not symbol.name.startswith('.')
                    and not (kind == '.text' and symbol.name.startswith('$L'))
                    and not (kind == '.text' and entry_alias_base(obj, symbol))
                    and symbol.name not in mapped_names
                    # A library member's own symbol table is its authority:
                    # an alias or static in a section placed by an owned
                    # anchor needs no metadata of its own.
                    and not (library and anchors)):
                raise ValueError(f'{unit}: unaccounted compiled definition {symbol.name}')
        if fold_keys:
            # The retail linker folded this body onto another TU's identical
            # function. Trust nothing: the declaration only names the owner,
            # and the extent, definition offset and relocated bytes are all
            # checked against retail below. No split member is recorded, so
            # the fold occupies no image space and shifts nothing.
            key = fold_keys[0]
            target = folded[key]
            if isinstance(target, str):
                original = project.symbols.get(target)
                if (not original or original['kind'] != 'function' or original['bounds'] != 'reviewed'
                        or not original['source'] or original['section'] != '.text'):
                    raise ValueError(f'{unit}: folded body {key} needs a reviewed retail function owned '
                                     'by another TU or by this one')
                if original['source'] == unit:
                    # LINK folds identical COMDATs of one object as readily as
                    # those of two. The surviving body is then this TU's own
                    # function: its compiled definition must be a separate,
                    # placed COMDAT, checked below once every section is placed.
                    same_unit_folds.append((index, key, original))
                address, extent, label = original['address'], original['size'], original['name']
                expected = layout['symbols'][label]['sha256']
            else:
                # No TU owns the surviving body, so the declaration states the
                # address and the length it measured. Neither is trusted: the
                # length must equal the extent the splitter measured in the
                # retail image at that address, and the bytes below must equal
                # the digest of exactly those retail bytes. pe_project has
                # already refused an address any split or symbol covers.
                address, extent = int(str(target['address']), 0), int(str(target['size']), 0)
                label = f'0x{address:08X}'
                retail = (layout.get('folds') or {}).get(label)
                if not retail or not retail['sha256'] or retail['extent'] != extent:
                    raise ValueError(f'{unit}: folded body {key} declares a {extent}-byte body at '
                                     f'{label}, but the retail body there measures '
                                     f'{retail["extent"] if retail else "nothing"}')
                expected = retail['sha256']
            if (kind != '.text' or not section.flags & 0x1000 or not definitions[key].type & 0x20
                    or definitions[key].value != 0 or section.size != extent):
                raise ValueError(f'{unit}: folded body {key} must be a code COMDAT defining the '
                                 f'whole {extent}-byte extent of {label}')
            used_folded.add(key)
            folded_sections.append((index, label, expected))
        elif not anchors:
            # Linker-pooled constants may be emitted by more than one TU.
            # Their canonical owner remains the only supplier to the image.
            keys = [s.name for s in symbols if s.name in shared]
            if len(keys) != 1 or not section.flags & 0x1000:
                raise ValueError(f'{unit}: unaccounted runtime section {index} ({section.name}); no owned symbol anchor')
            key = keys[0]
            original = project.symbols.get(shared[key])
            # VC6 /Gf pools string literals in writable .data, whereas /GF
            # emits .rdata. Only compiler-named, terminated ANSI strings with
            # no relocations may use the writable form of this exception.
            pooled_string = (kind == '.data' and key.startswith('??_C@_0')
                             and section.data.endswith(b'\0') and not section.relocations
                             and original and original.get('coff_name') == key)
            # VC5 /Gf emits the one-byte empty ANSI literal as a sparse .bss
            # COMDAT instead. Only that exact identity may use the zero-storage
            # form; the canonical owner's BSS extent supplies the image byte.
            pooled_empty = (kind == '.bss' and key == EMPTY_ANSI_STRING and section.is_bss
                            and section.size == 1 and not section.relocations
                            and original and original.get('coff_name') == key)
            # LINK keeps the first definition of a pick-any COMDAT; a library
            # member linked after a game TU that pooled the same literal (or
            # after a sibling member) reaches that copy, whatever its section.
            pick_any = library and not section.relocations
            if kind != '.rdata' and not (pooled_string or pooled_empty or pick_any):
                raise ValueError(f'{unit}: shared writable storage must be an identified /Gf string literal')
            if (not original or original['kind'] != 'data' or original['bounds'] != 'reviewed'
                    or not original['source'] or original['source'] == unit
                    or (original['section'] != kind and not pick_any) or original['size'] != section.size
                    or definitions[key].value != 0):
                raise ValueError(f'{unit}: shared COMDAT {key} needs an exact, reviewed datum owned by another TU')
            address = original['address']
            used_shared.add(key)
            shared_sections.append((index, original['name']))
        else:
            addresses = {original['address'] - s.value for s, original in anchors}
            if len(addresses) != 1:
                raise ValueError(f'{unit}: symbol offsets disagree in section {index} ({section.name})')
            address = addresses.pop()
            candidates = [i for i, p in enumerate(parts) if p['start'] <= address
                          and address + section.size <= p['end'] and p['section'] == kind]
            if len(candidates) != 1:
                raise ValueError(f'{unit}: compiled extent/storage for section {index} ({section.name}) exceeds its owning split')
            members[candidates[0]].append(index)
            for symbol, original in anchors:
                if (bool(symbol.type & 0x20) != (original['kind'] == 'function')
                        or symbol.value + original['size'] > section.size):
                    raise ValueError(f'{unit}: incompatible definition/extent for {original["name"]}')
        if address % section.alignment:
            raise ValueError(f'{unit}: alignment mismatch at 0x{address:08X} for {section.name}')
        placements[index] = address

    if used_shared != set(shared):
        raise ValueError(f'{unit}: unused shared COMDAT entries: {sorted(set(shared) - used_shared)}')
    if used_folded != set(folded):
        raise ValueError(f'{unit}: unused folded body entries: {sorted(set(folded) - used_folded)}')
    for original in owned:
        names = [original.get('coff_name')] if original.get('coff_name') else [
            s.name for s in obj.symbols.values() if coff.undecorate(s.name) == original['name'] and s.section > 0]
        if len(names) != 1 or names[0] not in definitions:
            raise ValueError(f'{unit}: missing compiled definition for {original["name"]}')
        symbol = definitions[names[0]]
        if symbol.section not in placements or placements[symbol.section] + symbol.value != original['address']:
            raise ValueError(f'{unit}: incorrect placement of {original["name"]}')
    folded_indices = {index for index, _, _ in folded_sections}
    same_unit_folds_checked = []
    for index, key, original in same_unit_folds:
        # The surviving copy must be this TU's own definition of the target,
        # in a different COMDAT that occupies the target's split extent; the
        # fold itself stays out of every split, so it holds no address of its
        # own and every reference to it resolves to the target's address.
        names = [original.get('coff_name')] if original.get('coff_name') else [
            s.name for s in obj.symbols.values() if coff.undecorate(s.name) == original['name'] and s.section > 0]
        if original not in owned or len(names) != 1 or names[0] not in definitions:
            raise ValueError(f'{unit}: folded body {key} must fold onto a separately placed definition '
                             f'of {original["name"]}')
        survivor = definitions[names[0]]
        if (survivor.section == index or survivor.section in folded_indices or survivor.value != 0
                or not any(survivor.section in m for m in members)):
            raise ValueError(f'{unit}: folded body {key} must fold onto a separately placed definition '
                             f'of {original["name"]}')
        same_unit_folds_checked.append((index, survivor.section, key, original['name']))

    def resolve(ref):
        if ref.section > 0:
            if ref.section not in placements:
                raise ValueError(f'{unit}: reference to non-runtime section: {ref.name}')
            return placements[ref.section] + ref.value
        if ref.section == -1:
            return ref.value
        overrides = project.matching.get('symbols', {})
        key = unit + ':' + ref.name
        if key in overrides or ref.name in overrides:
            return int(str(overrides.get(key, overrides.get(ref.name))), 0)
        name = coff.undecorate(ref.name)
        if name in project.symbols:
            return project.symbols[name]['address']
        # Recovered retail names can differ from native compiler identifiers
        # (e.g. sithCogYacc_yyparse is emitted as _yyparse). Honor explicit
        # identities for inter-TU references as well as owned definitions.
        aliases = {s['address'] for s in project.symbols.values()
                   if s.get('coff_name') == ref.name and s.get('scope') != 'local'}
        if len(aliases) > 1:
            raise ValueError(f'{unit}: ambiguous external COFF identity {ref.name}; add a source-qualified override')
        if aliases:
            return aliases.pop()
        if ref.name.startswith('__imp__'):
            name = coff.undecorate(ref.name.removeprefix('__imp_'))
            if name in layout['imports']:
                return layout['imports'][name]
        raise ValueError(f'{unit}: unresolved symbol {ref.name}; add an audited external address to relocations.yml')

    relocated = {}
    for index, address in placements.items():
        section = obj.sections[index - 1]
        relocated[index] = coff.relocate(section.data, section.relocations, obj.symbols, address,
                                         resolve, int(project.config['image_base'], 0))
    for index, name in shared_sections:
        if layout['symbols'][name]['sha256'] is None and library:
            # A pick-any literal kept as another object's zero BSS storage.
            if any(obj.sections[index - 1].data):
                raise ValueError(f'{unit}: shared COMDAT {name} is BSS but this copy is not zero')
            continue
        if obj.sections[index - 1].is_bss:
            # Sparse storage has no payload; the owner's split proves it zero.
            if layout['symbols'][name]['sha256'] is not None:
                raise ValueError(f'{unit}: shared BSS COMDAT {name} is not zero-initialized storage')
            continue
        if digest(relocated[index]) != layout['symbols'][name]['sha256']:
            raise ValueError(f'{unit}: shared COMDAT bytes differ for {name}')
    for index, name, expected in folded_sections:
        # The evidence for a fold is byte identity with the surviving body.
        if digest(relocated[index]) != expected:
            raise ValueError(f'{unit}: folded body differs from the retail bytes of {name}')
    for index, survivor, key, name in same_unit_folds_checked:
        # A same-TU fold must also be byte-identical to the compiled body that
        # survives in this object, both relocated at the target address.
        if relocated[index] != relocated[survivor]:
            raise ValueError(f'{unit}: folded body {key} differs from the compiled body of {name}')

    generated = []
    flags = {'.text': coff.CODE, '_TEXT16': coff.CODE | SECTION_EXTRA_FLAGS['_TEXT16'], '.data': coff.DATA,
             '.data1': coff.DATA, '.rdata': coff.RDATA, '.bss': coff.BSS}
    for part, reference, indices in zip(parts, references, members):
        if (reference['address'], reference['size'], reference['kind'], reference['align']) != (
                part['start'], part['end'] - part['start'], part['section'], part['align']):
            raise ValueError(f'{unit}: reference layout differs from the owning splits')
        indices.sort(key=placements.get)
        if not indices:
            raise ValueError(f'{unit}: no compiled section covers split 0x{part["start"]:08X}')
        if max(obj.sections[i - 1].alignment for i in indices) != part['align']:
            raise ValueError(f'{unit}: split alignment differs at 0x{part["start"]:08X}')
        # Leading fill is only what the first contribution's own alignment
        # requires after the preceding range: exactly the declared `lead`.
        if placements[indices[0]] - part['start'] != part.get('lead', 0):
            raise ValueError(f'{unit}: leading alignment fill at 0x{part["start"]:08X} is '
                             f'{placements[indices[0]] - part["start"]} bytes, not the declared '
                             f'{part.get("lead", 0)}')
        cursor = part['start']
        for index in indices:
            section = obj.sections[index - 1]
            if placements[index] != align_up(cursor, section.alignment):
                raise ValueError(f'{unit}: gap or overlap in compiled coverage at 0x{cursor:08X}')
            cursor = placements[index] + section.size
        if cursor != part['end'] and align_up(cursor, part['align']) != part['end']:
            raise ValueError(f'{unit}: incomplete compiled coverage at 0x{cursor:08X}')
        size, kind = part['end'] - part['start'], part['section']
        data = bytearray() if kind == '.bss' else bytearray([part['padding']]) * size
        if kind != '.bss':
            for index in indices:
                offset = placements[index] - part['start']
                data[offset:offset + len(relocated[index])] = relocated[index]
            if digest(data) != reference['sha256']:
                raise ValueError(f'{unit}: compiled bytes differ in {kind} split at VA 0x{part["start"]:08X}')
        characteristics = (flags[kind] & ~0xF00000) | (part['align'].bit_length() << 20)
        generated.append(coff.Section(kind, bytes(data), characteristics, size=size))
    return generated
