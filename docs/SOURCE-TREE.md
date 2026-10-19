# The original source tree and how this repository mirrors it

The executable records where its source lived. Assertion and diagnostic strings carry `__FILE__`, and the Italian
release's debug executable carries a CodeView record naming its program database. This document collects what those
paths say about the original tree, maps the modules to what they do, shows where this repository's directory layout
follows the original and where it does not, and lists what else the repository holds.

## What the binaries record

Three groups of paths appear in the images.

| Root | Contents | Where it is recorded |
| --- | --- | --- |
| `C:\Jones3D\Libs\` | the engine libraries: `sith`, `std`, `rdroid`, `w32util`, `Katmai` | `__FILE__` strings in the release and debug images |
| `C:\Jones3D\Product\Indy3D\` | the game itself: `Display`, `Gui`, `Main`, `Play` | the same, plus the debug image's `NB10` entry `C:\Jones3D\Product\Indy3D\Debug\Indy3D.pdb` |
| `E:\smush\INSANE\SOURCE\` | the SMUSH/INSANE movie player: `Image`, `ANIMATE`, `File`, `File\Zfilio`, `Sound`, `Flobject`, `Bitmap` | `__FILE__` strings of the movie-player objects |

The debug image names 125 source files. 122 of them map onto a file of this repository by path, comparing names
without regard to case. The other three are discussed under [Where the layout differs](#where-the-layout-differs) and
[Debug-only code](#debug-only-code-sithwritec). The
debug image is described in [DEBUG-BUILD.md](DEBUG-BUILD.md).

The INSANE paths show that the movie player was built on another drive as a separate project and linked into the game
as finished objects: they do not sit under `C:\Jones3D`. One of its files spells its directory in lower case
(`E:\smush\insane\source\Bitmap\Bitmap.c`), the others in upper case. Ten INSANE units record a path, and
the build gives each its recorded name (see [COMPILERS.md](COMPILERS.md#visual-c-60-rtm-for-insane-smushplay-and-zlib)
for the compiler).

Files that no string names (the sound library, `wkernel`, several `Jones3D` files, the GUID block, the third-party
code) take their names from the engine source this project began from, or are named by inference where the evidence
fixes the boundary but not the name. Such names are marked as inferred in the build's configuration.

## The module map

The code is laid out in the executable in link order. Reading `.text` from the start, the regions run: the game
(`Jones3D`) from `0x00401000`, then the `sith`, `std`, `w32util`, `wkernel`, `rdroid` and `sound` libraries (the
`sith` and `std` objects alternate), the DirectInput data formats from the SDK library at `0x004E0A10`, the "Katmai"
renderer at `0x004E1DA0`, the Intel `libm` helpers at `0x004E2F30`, SmushPlay and the INSANE objects from `0x004E30B0`
with the zlib objects interleaved among them, and the C runtime from `0x004F64B0` to the end of the section.

| Module | Directory | Compiler | What it does |
| --- | --- | --- | --- |
| Game | `Jones3D/` | Visual C++ 5.0 SP3 | The application: `JonesMain` (start-up, the main loop, window messages, command line), `JonesFile`, `jonesString`, `JonesDevDialog`; `JonesDisplay`, `JonesConsole`, `JonesHud` and `jonesConfig` (display set-up, console, heads-up display, the configuration dialogs); `JonesDialog`; `JonesControl`, `jonesCog` (the game's own COG verbs) and `jonesInventory` |
| `sith` | `Libs/sith/` | Visual C++ 5.0 SP3 | The game engine: world loading and the thing, sector, surface and template model (`World`); the COG scripting language, its parser and its verb tables (`Cog`); AI (`AI`); physics, collision, animation, cameras, the renderer front end and sky (`Engine`); players, inventory, weapons, the whip and events (`Gameplay`); input, console, sound mixing and the message layer for multiplayer (`Devices`); multiplayer state sync and save games (`Dss`); start-up and string tables (`Main`) |
| `rdroid` | `Libs/rdroid/` | Visual C++ 5.0 SP3 | The renderer library: cameras, canvases, clipping, lights, materials, things, puppets and keyframes (`Engine`); vector and matrix maths (`Math`); models, sprites, particles, polylines, fonts and the primitive drawers (`Primitives`); the polygon cache and face submission (`Raster`); the library's own start-up (`Main`) |
| `std` | `Libs/std/` | Visual C++ 5.0 SP3 | The platform layer. `General`: memory, files and file names, the configuration-file reader, hash tables, linked lists, string tables, colours, bitmaps, the platform function table. `Win95`: Direct3D (`std3D`), DirectDraw (`stdDisplay`), DirectInput (`stdControl`), DirectPlay (`stdComm`), the console, the GOB archive reader (`stdGob`) and the window and process glue (`stdWin95`) |
| `sound` | `Libs/sound/` | Visual C++ 5.0 SP3 | The sound system: a bank of loaded sounds (`Sound`), the DirectSound driver with 3D positioning (`Driver`) and the audio library (`AudioLib`: WAVE header parsing, the sample compressor and decompressor, lip-sync data) |
| `wkernel` | `Libs/wkernel/` | Visual C++ 5.0 SP3 | The Windows kernel layer: the window class, the message pump and the window procedure hook |
| `w32util` | `Libs/w32util/` | Visual C++ 5.0 SP3 | Registry access (`wuRegistry`) |
| Katmai | `Libs/rdroid/Primitives/rdModel3K*.c` | Intel C/C++ 4.0 | The SSE mesh renderer and its CPU probes ([below](#the-katmai-renderer)) |
| Intel `libm` | `Libs/intel/libm/` | Intel C/C++ 4.0, for reference only | The `floor`, `ceil` and `modf` helpers that the Intel compiler's maths library supplied instead of the C runtime's; the build copies their bytes from the retail executable ([DECISIONS.md](DECISIONS.md#10-library-code-taken-from-the-original)) |
| SMUSH/INSANE | `Libs/smush/` | Visual C++ 6.0 RTM; ML 6.13 for the six `.asm` files | The movie player and the `SmushPlay` glue ([INSANE.md](INSANE.md)) |
| zlib | `Libs/zlib/` | Visual C++ 6.0 RTM | zlib 1.0.9, the members the game pulls in, with `gzio.c` modified to read through INSANE's file-I/O table |
| C runtime | not compiled | prebuilt | Members of Visual C++ 5.0's `libc.lib` ([BUILD-PIPELINE.md](BUILD-PIPELINE.md#library-units)) |
| DirectX data | not compiled | prebuilt | Members of the DirectX 6.1 SDK's `dinput.lib` (the same) |

Two further directories are not original code: `Libs/j3dcore/` holds the shared headers and basic types the engine
source uses (and the count-fit macros, [DECISIONS.md](DECISIONS.md#2-declaration-count-fits-tier-3)), and
`Libs/external/DirectX61c/` is where the setup script installs the DirectX 6.1 SDK headers and libraries (not
committed).

### Naming conventions

Functions follow `module_Verb` names (`sithThing_Create`, `rdCamera_SetCanvas`, `stdFileOpen`), matching the strings the
debug build prints. Where the names come from, and the two places a name affects the bytes, is in
[DECISIONS.md](DECISIONS.md#8-names); the programmers' habits are in [HOUSE-STYLE.md](HOUSE-STYLE.md).

## How this repository mirrors it

The repository's directories follow the original tree below the roots:

| Original | This repository |
| --- | --- |
| `C:\Jones3D\Libs\sith\<Group>\<file>.c` | `Libs/sith/<Group>/<file>.c` |
| `C:\Jones3D\Libs\rdroid\<Group>\<file>.c` | `Libs/rdroid/<Group>/<file>.c` |
| `C:\Jones3D\Libs\std\<Group>\<file>.c` | `Libs/std/<Group>/<file>.c` |
| `C:\Jones3D\Libs\w32util\<file>.c` | `Libs/w32util/<file>.c` |
| `C:\Jones3D\Product\Indy3D\<Group>\<file>.c` | `Jones3D/<Group>/<file>.c` |
| `E:\smush\INSANE\SOURCE\<Group>\<file>.c` | `Libs/smush/insane/<Group>/<file>.c` |

`configure.py` lists every file once, with its compiler, its flags and, where the executable records it, its original
path (`original_filename`), which the build uses so that `__FILE__` strings carry the recorded text. Headers sit beside
their sources.

The tree is the source of version 1.2. `versions/1.0/` mirrors the same layout for the 14 files that 1.2 changed, each
a complete copy of the file as 1.0 had it (`versions/1.0/Libs/sith/World/sithActor.c`); the 1.0 build compiles those
instead ([VERSIONS.md](VERSIONS.md#how-the-two-versions-are-built)).

Outside the source, the repository holds what builds and checks it:

- `config/indy3d-v1.2/`, `config/indy3d-v1.0/`: the description of each executable *(splits, symbols, relocations,
  and the headers, imports and resources the build generates)*.
- `Scripts/`: the PE/COFF backend *(splitter, matcher, image builder, compiler driver and the setup script)*.
- `tools/`: helpers the build and setup script use.
- `progress/`: the figures behind [BYTE-BREAKDOWN.md](BYTE-BREAKDOWN.md), and each version's `report.json`.

### Where the layout differs

- **Case.** The original spelled the AI directory and some files `Ai` (`sithAi.c`, `sithAiMove.c`); the repository
  uses `AI` and `sithAI.c`, `sithAIMove.c`, matching the function names. INSANE's `ANIMATE` directory is `Animate`.
- **`stdPlatform.c`** lived in `Libs\std\Win95\`. The repository keeps it with the other platform-independent code, in
  `Libs/std/General/`.
- **The Katmai renderer** lived in its own library, `C:\Jones3D\Libs\Katmai\source\rdModel3K.c`. The repository files
  it under `Libs/rdroid/Primitives/`, because the code is called as part of the renderer and its header is `rdroid`'s.
- **Files with no recorded path** (the `sound` library, `wkernel`, the GUID block, `Libs/intel`, the zlib and INSANE
  units without an `__FILE__` string) sit where the code's role puts them.
- **`Jones3D/` holds `Indy3D.rc`**, the resource script written for this project (see
  [BUILD-PIPELINE.md](BUILD-PIPELINE.md#resources)). The original script and the original header files are not
  recorded.

## Debug-only code: `sithWrite.c`

`C:\Jones3D\Libs\sith\World\sithWrite.c` appears in the debug image and nowhere in the release image. It is a
whole debug-only file of world and thing writers: eight diagnostic sites, at lines 79, 83, 133, 201, 202, 511, 533
and 534, with assertion texts such as `pThing && pThing->moveType == SITH_MT_PATH`. Retail contains none of its
code, and the repository has no file of that name.

The writers' callees do exist in other modules. Four text writers (the COG list in `sithCog.c`, the PVS in `sithPVS.c`,
the sound list in `sithSound.c`, the AI class list in `sithAIClass.c`) were called only from `sithWrite.c`, so the
release linker discarded them. Their bodies are known from the debug build and are kept in their own files as
functions retail does not contain. This is the general pattern for debug-only code: where an object file's
unreferenced functions were removed by the linker, the source still defines them, because the debug image shows them and
an object file compiled from the source needs them.

470 `(file, line)` keys occur only in the debug image: 210 asserts, 232 log prints and 28 allocation calls, the lines
that the release build dropped from files that retail does contain ([DEBUG-BUILD.md](DEBUG-BUILD.md)).

## The Katmai renderer

The directory name `Katmai` is Intel's code name for the first Pentium III core, whose Streaming SIMD Extensions (SSE,
announced as the "Katmai New Instructions") the renderer uses. `rdModel3K` is a 4,496-byte block at `0x004E1DA0` of the release image: `rdModel3K_Startup`, the vertex
transform and projection routines, `rdModel3K_DrawMesh` and the availability and enable flags, which a CPU probe sets. Its compiler is Intel's, not Microsoft's (65 `movaps`, 28 `shufps`, `cvtps2pi`, `emms`; the
aligned-stack entry sequence), so it is the only game code the Visual C++ toolchain cannot produce.

It is a two-object module. `rdModel3K.c` holds the renderer; `rdModel3K_cpu.c` holds the CPU and operating-system
capability probes (`0x004E2D80-0x004E2F30`). The probes are a second Intel object because their exception-handler
scope tables sit in `.data1` after the first object's string constants, which a single compile does not produce.
The boundary and the second file's name are inferred. The evidence and the flags are in
[COMPILERS.md](COMPILERS.md#intel-cc-40-the-sse-renderer-and-libm).

## Other notable files

- **`stdGuid.c`** is a code-less unit that owns the 1,936-byte block of DirectX GUIDs in `.rdata`. 121 GUIDs appear in
  header-definition order, 98 of them unreferenced, so they come from one object compiled with `INITGUID` rather than
  from `dxguid.lib` ([COMPILERS.md](COMPILERS.md#what-the-executable-records)). The file name is inferred.
- **`sithCogYacc.c` and `sithCogFlex.c`** are the Cog script parser and scanner, generated by Berkeley yacc 1.4
  (`yacc -dl`) from `cog.y` and by flex 2.3.6 (`flex -Li -Sflex.skel`) from `cog.l` with the scanner skeleton
  `flex.skel`, all in `Libs/sith/Cog/`. The generators' version strings and tables are in retail. The generated files
  are checked in, so the build does not need the generators; `Scripts/cog_generators.py` builds them from their 1990
  postings and regenerates the files ([REPRODUCIBILITY.md](REPRODUCIBILITY.md#optional-the-cog-parser-generators)).
  Their `goto`s are the generators' ([DECISIONS.md](DECISIONS.md#9-the-listed-exceptions-goto-inline-assembly-volatile)).
- **`JonesHudConstants.h`** is a shared header whose store tables are compiled into three `.data` sections
  (`jonesCog.c`, `JonesMain.c`, `JonesDevDialog.c`), each a copy of the same ~7 KB table.
