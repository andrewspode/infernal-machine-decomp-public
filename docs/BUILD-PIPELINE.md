# How the build works

This is the detail behind [BUILDING.md](BUILDING.md): what the build does between the retail executable and a rebuilt
one, where each piece of the image comes from, and what is still copied from the retail executable. Each version has its own
build data in `config/<version>/` and its own outputs in `build/<version>/`; the two builds share the source tree and
the backend ([VERSIONS.md](VERSIONS.md#how-the-two-versions-are-built)). Paths and figures below are 1.0's unless they
say otherwise; 1.2's are the same in kind.

## The steps

1. **Split.** The original executable is checked against its SHA-256 and cut, by address, into one COFF object per
   original source file ([below](#splits)).
2. **Compile.** Each `.c` file is compiled with the compiler that originally built it, under the same flags
   ([COMPILERS.md](COMPILERS.md)). The 1.0 build compiles the same tree, except that the 14 files 1.2 changed come from
   their complete 1.0 copies in `versions/1.0/`.
3. **Match.** A strict checker compares everything a file owns *(code, constants, data, BSS layout, padding)* with its
   split, after resolving relocations. A file passes whole or not at all.
4. **Substitute.** Every passing file contributes its compiled sections; anything else falls back to its split
   original.
5. **Rebuild.** The image is reassembled at the original addresses, with the headers, import tables and resources
   generated from checked-in descriptions ([below](#generated-fragments)).
6. **Verify.** The rebuilt `Indy3D.exe` must have the original's SHA-256 and SHA-1.

```
orig/Indy3D-1.2.exe ──split──▶ per-file COFF objects (code, data, BSS at their addresses)
(1.0: orig/Indy3D.exe)                │
 source (.c) ──compile──▶ objects ──strict check against the split──▶ accepted objects
                                      │
 generated fragments, library members ┘
                                      ▼
            image builder: every piece at its original address ──▶ Indy3D.exe ──▶ SHA-256, SHA-1
```

The builder does not run Microsoft's 1999 linker and does not claim to reproduce its general algorithm. It places each
piece at the address and file offset the original has, so a complete image is reproduced only when every piece is. The
split -> compile -> rebuild -> verify shape follows [decomp-toolkit](https://github.com/encounter/decomp-toolkit)
projects, with a PE/COFF backend (`Scripts/`) for the x86 Windows executable.

## Splits

`config/<version>/splits.txt` assigns every range of `.text`, `.rdata`, `.data`, `.data1` and `.bss` to a file, in the
order the linker laid them out, and `symbols.txt` names the functions and variables inside, with sizes. Splits come from
the evidence in the image: a function's first and last address, the 16-byte `0xCC` padding between objects compiled
with function-level COMDATs, string literals that name a file, the order of initialised data, the order of BSS
variables (which a compiler assigns by a hash of the identifier) and the module registration tables of the movie
player. Each range records how well its bounds are known (`reviewed` or `inferred`). A matching file must have reviewed
bounds.

A split may own **alignment fill** between objects when the next object's alignment explains it: `0xCC` in `.text`,
zero bytes in data. Fill that no neighbour's alignment explains stays unowned.

`ninja split` validates the splits and cuts the original into one COFF object per file.

## Compiling and the strict check

For a file marked `Matching`, the builder places every section the file compiles to at its split's address, resolves
the relocations and runs the strict check ([METHODOLOGY.md](METHODOLOGY.md#what-matching-means) defines it). The
rebuilt image then takes **all** of that file's bytes from the compiler's output, never from the original.

Three things need accounting that a plain comparison does not give:

- **Linker-discarded functions.** Retail's linker removed unreferenced COMDAT functions. The source still defines
  them (the debug build shows their bodies), and the build lists them as discarded so that the compiler's copy is not
  placed.
- **Linker-folded bodies.** The linker folded byte-identical function COMDATs onto one address, sometimes across files
  and across the game/runtime boundary (for example, an empty `ret` body of LIBC's `fpinit.obj` is the game's
  `rdCanvas_FreeEntry`). The relocation table records the fold so that every referrer resolves to the one copy.
- **Shared constants.** String and floating-point constants that several files shared are mapped to the one retail copy.

A file that does not match entirely is `NonMatching`, and its bytes come from the original split object.

## Library units

Some bytes are not compiled at all. A **library unit** is one member of a static library that retail linked: 159
members of Visual C++ 5.0's `libc.lib` and the three DirectInput data-format members of the DirectX 6.1 SDK's
`dinput.lib`. Each is extracted unmodified from its archive, pinned by SHA-256 in `libraries.yml`, placed at its split
and checked like compiled code. Library units are reported in their own category, never as game code. How the libraries
were identified is in [COMPILERS.md](COMPILERS.md#the-linked-libraries).

## Generated fragments

Several structures of the file are not compiled code but are determined by what the linker was asked to do. They are
generated from small descriptions and checked, with everything else, by the whole-image hash. Each fragment is a
complete, named range of the file.

| Fragment | File offset, size | Generator input |
| --- | --- | --- |
| PE and section headers | `0x0`, `0x400` | `image/header.yml` |
| Import address table | `0x104000`, `0x330` | `image/imports.yml` and the import libraries |
| Import directory, lookup and hint/name tables | `0x109D63`, `0x129D` | the same |
| Import jump thunks in `.text` | `0x21190` (16), `0xDFDF0` (32), `0x103DC0` (64) | `jump_thunks` in `imports.yml` |
| File padding of `_TEXT16` | `0x103E04`, `0x1FC` | `image/header.yml` (zeros from the section's 4 bytes to its 0x200-byte raw size) |
| File padding of `.data1` | `0x14F390`, `0x70` | `image/header.yml` (zeros from the section's 0x190 bytes to its 0x200-byte raw size) |
| Resources (`.rsrc`) | `0x14F400`, `0x28800` | `Jones3D/Indy3D.rc` and the icons ([below](#resources)) |

The generator is [Scripts/pe_generate.py](../Scripts/pe_generate.py).

### Headers

`header.yml` lists what the linker was told or chose: the stub, the Rich entries, the timestamp, entry point, versions,
stack and heap sizes, section names, sizes and flags. Section addresses and file offsets, `SizeOfCode`,
`SizeOfInitializedData`, `SizeOfImage` and the directories are derived.

- **The Rich header.** Its six entries are in the order recorded in the image, with the build counts listed in
  [COMPILERS.md](COMPILERS.md#what-the-executable-records). Its key is the record's offset (`0x80`) plus each
  `product ID << 16 | build` rotated left by its object count, with **no** DOS-header term. The formula of later linkers
  gives `0x9A9648E7` for these entries; the retail value is `0x12471546`, which the older linker's rule reproduces.
- **The `PE\0\0` signature** sits at offset `0xD0`, on a 16-byte boundary, which is where zero fill of the stub puts
  it. This is inferred from this one image.
- The timestamp is `0x3819F0F4`, 1999-10-29 19:09:40 UTC; the linker is 5.10.

### Imports and thunks

Every import's hint, ordinal and name, and every DLL name, is taken from the import libraries retail linked: the
Visual Studio 97 SP3 platform libraries and the DirectX 6.1 SDK's `ddraw.lib`, `dsound.lib` and `dinput.lib`, each
pinned by SHA-256 in `imports.yml`. The SDK's `ddraw.lib` is required: the Visual Studio 97 copy has hints 8 and 6, and
retail has 9 and 7. The file records only the **link order**: the order of the descriptors, the order of the slots
within each block, and the order of the hint/name entries. No model of the linker derives those yet.

For each import the code calls directly, the linker wrote a `jmp dword ptr [slot]` stub (`FF 25`) in `.text`, where
that import library member fell in link order, then fill. There are three such places: `GetSaveFileNameA` and
`GetOpenFileNameA` after `jonesString`'s code, the four DirectX creators (`DirectDrawEnumerateA`, `DirectDrawCreate`,
`DirectSoundCreate` by ordinal 1, `DirectInputCreateA`) before the DirectInput data formats, and `RtlUnwind` (called by
LIBC's `exsup.obj`) at the end of `.text` with the section's zero padding. The slot addresses come from the generated
import address table.

### Resources

`Jones3D/Indy3D.rc` is a resource script written for this project from the retail templates: 26 dialogs, 3 `DLGINIT`
blocks and 25 icons, with numeric identifiers and statements in the order the resource data has. It is compiled by
[Scripts/pe_rsrc.py](../Scripts/pe_rsrc.py), a Python implementation of RC.EXE's semantics that lays the section
out as CVTRES and the linker do: the directory tree with identifiers ascending, then the data entries, then the data on
8-byte boundaries in script order.

Cross-check: the period Visual Studio 97 SP3 `rc.exe`, run once under Wine, compiles the same script to the same 79
resources, byte for byte, and their layout is the retail section. That check also found one RC rule: a `COMBOBOX` keeps
its default `WS_TABSTOP` only when the statement has no style, so the script spells `WS_TABSTOP`. `rc.exe` cannot run
under wibo (its `rcdll.dll` imports `user32` functions that wibo does not implement), which is why the build uses the
Python compiler.

**The icons are copied from the retail executable.** The 25 icons are game artwork: 147,176 bytes of icon image data in the
165,888-byte section. They are not in this repository. When the version's executable is present, the build extracts
them from it (after checking its SHA-256) into the ignored `build/<version>/assets/icons/` as the `.ico` files the script
names, and the resource compiler reads them there. Nothing else reads the executable for this. The image builder counts
those bytes as `user_asset_bytes`, not as generated or source bytes, and **leaves them out of every progress figure**,
numerator and denominator alike. The generated section is still checked by the whole-image hash like every other
fragment. Everything else in the section (the dialogs, the icon group directories and the resource tree) is generated
from the script.

## The frozen layout

The ordinary build reads the layout of fragments, ranges, ownership and imports from the split of the original. The
repository also carries a frozen copy, `config/indy3d-v1.0/layout.json`: the addresses, sizes, ownership, imports and
hashes, and none of the original's payload. Only `ninja v1.0-source-only` reads it, and it must be refreshed whenever
splits or symbols change. 1.2 has no frozen copy yet: its `ninja source-only` reads the layout extracted from your 1.2
executable.

## What is still copied from the retail executable

Everything the build still copies from the retail executable is listed in `build/<version>/link.json` and `progress.json`: today **2,627** of the 1,391,896
counted bytes in 1.2 and 2,643 in 1.0 (the 147,176 bytes of icon artwork are not counted). Almost all of it is library
code that retail linked in already compiled, and the rest is bytes nothing reads. Why none of it can come from source
or from the libraries we have is in [DECISIONS.md](DECISIONS.md#10-library-code-taken-from-the-original); the figures
below are 1.2's.

- **Intel's maths library, 731 bytes.** `floor` and `modf` *(571 bytes; their reconstructions in `Libs/intel/libm/`
  are compiled and checked, but their constant pools are in a different order)* and the statics of `support.obu`
  *(80 bytes each of `.data` and `.data1`, among them the `@(#)support.c 1.4.7.1` ident string; its five functions were
  discarded by the linker)*.
- **The C runtime, 1,772 bytes of `.text`.** `aw_cmp.obj`, `aw_map.obj` and `aw_str.obj` *(1,648 bytes)*, whose pooled
  wide-string literal lies in a 4-byte BSS slot (`0x0056B6F8`) that no file owns, and `longjmp.obj` *(124 bytes)*, whose
  only data symbol is never referenced and so has no address.
- **Unused bytes, 124 in 12 pieces** *(140 in 1.0, whose `.rdata` pieces are 16 bytes longer)*: unreferenced
  constants in `.rdata` and `.data` (one of them a 16-byte, GUID-shaped record) and zero gaps between files that no
  alignment explains.

`ninja map` lists every range. `ninja source-only` is the end-state gate: a rebuild that refuses every fallback to the
original and never reads an original COFF object, so it fails until none of the above is left.
