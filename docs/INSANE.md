# SMUSH/INSANE: the movie player inside the executable

The cut-scene movies of the game are played by a LucasArts library called **INSANE**, the player half of the SMUSH
movie format. Its code is linked into `Indy3D.exe` between the Intel helpers and the C runtime: about 50 KB of `.text`
from `0x004E30B0`, driven by a small piece of game-side glue, `SmushPlay`. This document describes what the library is,
how the evidence shows its structure, what compiler built it, and the hand-written assembler objects inside it.

## Where it comes from

- **A separate project.** The strings in its objects name `E:\smush\INSANE\SOURCE\...`, not `C:\Jones3D`
  ([SOURCE-TREE.md](SOURCE-TREE.md)). The library was built on another drive and linked into the game as finished
  objects. The Jedi Knight executables from the same engine family contain no INSANE strings, so among the engine
  family's executables only this game links it.
- **No debug view.** The Italian release's debug executable ([DEBUG-BUILD.md](DEBUG-BUILD.md)) links the same
  release-built INSANE objects, so the library has no unoptimised build to read. Its source is recovered from the
  release bytes alone.
- **zlib travels with it.** The movie format can be compressed. The zlib 1.0.9 objects are interleaved with the INSANE
  ones in the image, and the file-I/O module `Zfilio` reaches zlib's `gzio.c` through INSANE's own file table. That is
  the one modification of zlib in the image: `gzio.c` reads through the table where the published source calls stdio.

## How it is organised

INSANE is built from modules that register themselves with a small framework (the `Lec` code). Each module's
registration function pushes a string `LECMODULE=insane_<name>`, and modules publish interfaces under names such as
`READANIM`, `FILIO`, `STREAMER`, `ANIMFRAME`, `FLOBDEC`, `BITMAP` and `MEMORY`. Other modules subscribe to a name or to a
`NAME=provider` pair. A module's interface is a table of function pointers; its first `.data` words are the four-entry
table `{register, unregister, startup, shutdown}`.

The game starts the player in `SmushPlay_SysStartup`, which hands the framework the module list and the request
`READANIM=insane_auto FILIO=insane_zfilio`, configures the file reader for `.snm` and `.znm` files, hands the
DirectSound object to the sound driver, and starts every module. The module table in `.data` at `0x0054BDE0` lists the
modules in link order, as `{register, dependency name}` pairs. The same order gives each module's code, data and BSS
range, which is how the library was split into units: each unit begins at its registration function.

## The units

`SmushPlay.c`, twenty C library files compiled with Visual C++ 6.0 RTM, and six assembler files assembled with
Microsoft's ML 6.13 ([below](#the-assembler-objects)). All 27 units match.

| Unit | Module | What it does |
| --- | --- | --- |
| `SmushPlay.c` | glue | Start-up, `PlayMovie`, shutdown and the global volume, called by `JonesMain` |
| `Image/Readauto.c` | `insane_auto` | Chooses the movie reader (`READANIM=insane_auto`) by recognising the file's format |
| `Animate/Pcplay.c` | `insane_animplay` | The player object: pacing, the frame clock, the service pointers shared by the frame code |
| `Animate/Frame.c` | `insane_animframe` | Walks the frame chunks (`load`, `ftch`, `npal`, `stor`), reassembles fragments, draws |
| `File/Cdstream.c` | `insane_streamer` | A ring-buffer file streamer |
| `Sound/Engine.c` | `insane_sound` | The sound engine: tracks, the script-driven callback, stream start, voice handling |
| `Sound/Dsnd.c` | `insane_dsnd` | The DirectSound output driver |
| `Flobject/D_wave.c` | `insane_wave_flobdec` | The decoder for audio chunks: variable-bit-rate ADPCM |
| `Flobject/D_bloc16.c` | `insane_blocky16_flobdec` | The decoder for the 16-bit "blocky" frame objects |
| `File/Stdfile.c`, `File/Stdio.c` | `insane_stdfile`, `insane_stdio` | File access through the C runtime and the game's platform layer |
| `File/Zfilio/Zfilio.c` | `insane_zfilio` | Compressed-file access through zlib |
| `Bitmap/Bitmap.c` | `insane_bitmap` | Allocation of bitmaps in the display's pixel formats |
| `Timer/Timer.c` | `insane_timer` | 19 software timers driven by a time-critical thread |
| `Memory/Memory.c` | `insane_memory` | The allocator and its debug-reallocation wrapper |
| `Lec/LecShare.c` | `insane_lecshare` | The framework's registry: publish, subscribe, enumerate |
| `File/Framestream.c` | `insane_framestream` | The chunked frame stream (`FLHD`, `ANNO`, `FRME`, `SHDR`, `paws`, `wait`) |
| `Sound/Mixer.c` | `insane_mixer` | Sample mixing and format conversions |
| `Decode/Decode.c` | `insane_decode` | Run-length and bit-stream decoders, pixel-format converters |
| `Util/List.c`, `Bitmap/Blit.c` | helpers | Intrusive lists; the blit and clipped-blit wrappers |
| `Sound/MixerAsm.asm` | (assembler) | The mixer's buffer copy, fill and lookup-table procedures, and the `_TEXT16` segment |
| `Flobject/Bloc16Asm.asm` | (assembler) | The `blocky16` block decoder and its subroutines |
| `Bitmap/MemCopy.asm`, `Bitmap/MemFill.asm` | (assembler) | Memory copy and byte fill |
| `Decode/DecodeAsm.asm` | (assembler) | The pixel-format conversion loops |
| `Bitmap/BlitAsm.asm` | (assembler) | The rectangle copy behind `Blit.c` |

The library has no debug build and no recorded names beyond its module and interface strings, so the names of
functions and variables are chosen to fit the evidence, and `lec.h`, `animplay.h` and the other headers under
`Libs/smush/insane/` are reconstructions of structure layouts read from the code. Where the release bytes alone decide
a local or an aggregate, the choice is labelled ([DECISIONS.md](DECISIONS.md#7-release-chosen-locals-tier-2)).

## The compiler: Visual C++ 6.0 RTM

Unlike the game, the library is Visual C++ 6.0 output: two consecutive `cdecl` calls share one `add esp`, `void`
functions end `call f; pop ecx; ret`, and prologues are shrink-wrapped past early-out tests (`readauto_Read`), none of
which VC5 SP3 produces. The flags are zlib's, `/O2 /Zp1 /Gy`. The evidence and the flag sweep are in
[COMPILERS.md](COMPILERS.md#visual-c-60-rtm-for-insane-smushplay-and-zlib).

## What the layout shows

- **Objects are 16-aligned and `NOP`-filled.** Function starts are 16-aligned addresses after a `ret` or `jmp`. A
  switch's case bodies sit between its `jmp [reg*4+table]` and the table, so a scan has to decode through them.
- **Strings are writable COMDATs, pooled across objects.** The first user in link order owns the string:
  `"MEMORY.workspace"` at `0x0054BFD4` belongs to the first module, though every later module pushes it.
- **Aggregates of 8 bytes or more make a `.data` section 8-aligned** in VC6, which fixes where the padding between
  objects falls.
- **Static BSS variables ascend through `0x017E4690-0x017EED30` in module order**, in each object in VC's
  identifier-hash order. The original names of statics are unknown, so names are chosen that give retail's order.
  Uninitialised globals are COMMON symbols that the linker places in the far region `0x0183B9C4-0x0185E8A8`.
- **Identical bodies fold across objects.** The no-op `0x004E74A0` is shared by several modules' start-up and shut-down
  slots and by the framework's initial error handler.
- **Original line numbers.** Allocation calls carry retail's `__LINE__` values (`#line 251`, `280`, `290`, `310` in
  `Readauto.c`; `211`, `266`, `292` in `Zfilio.c`), and the build gives each unit its recorded `__FILE__` path.

## Variables shared with the timer thread

The timer module runs a time-critical thread. Four variables it shares with the main thread, and one parameter of
`framestream_Tick`, are `volatile` in the source: the bytes reload them where an optimiser would cache them. These are
the only `volatile` declarations the project permits; the evidence for each is in
[DECISIONS.md](DECISIONS.md#9-the-listed-exceptions-goto-inline-assembly-volatile).

## The assembler objects

Six of the library's objects are not compiler output but hand-written assembler. The Rich header records six objects
from Microsoft's ML 6.13 assembler, build 7299 ([COMPILERS.md](COMPILERS.md#what-the-executable-records)). Their
procedures have unaligned entry points, frames like `push ebp; mov ebp,esp; push esi; push edi; push es; push ecx;
push ds; pop es`, `rep movs`/`rep stos` copies and `lodsb`/`loop` table-lookup loops. Each is an assembler source
file, assembled by ML 6.13.7299 and checked byte for byte like any other source file. The pinned assembler stamps the
same build number into each object as the Rich header records.

| File | Range | What it is |
| --- | --- | --- |
| [`Sound/MixerAsm.asm`](../Libs/smush/insane/Sound/MixerAsm.asm) | `0x004EC560-0x004EC720` | The mixer's copy, fill and lookup-table procedures: eight procedures at unaligned entries, two of them called from the sound engine (`0x004E746F`, `0x004E748F`). |
| [`Flobject/Bloc16Asm.asm`](../Libs/smush/insane/Flobject/Bloc16Asm.asm) | `0x004EDEF0-0x004EE6A0`, data `0x0054DC70-0x0054DCA8` | The `blocky16` codec's block decoder (`dbloc16_Codec2`, called by `D_bloc16.c`), its 8x8, 4x4 and 2x2 subroutines, and an unused 2x interpolating upscaler. |
| [`Bitmap/MemCopy.asm`](../Libs/smush/insane/Bitmap/MemCopy.asm) | `0x004F04D0-0x004F04F5` | The memory copy `blit_MemCopy`. |
| [`Bitmap/MemFill.asm`](../Libs/smush/insane/Bitmap/MemFill.asm) | `0x004F04F8-0x004F054F` | The byte fill `blit_MemCopyB`. |
| [`Decode/DecodeAsm.asm`](../Libs/smush/insane/Decode/DecodeAsm.asm) | `0x004F0590-0x004F09BA` | The pixel conversions (`decode_Lut8`, `decode_Lut16` and the RGBA to 555/565 conversions and back). |
| [`Bitmap/BlitAsm.asm`](../Libs/smush/insane/Bitmap/BlitAsm.asm) | `0x004F2220-0x004F22B7` | The 10-argument rectangle copy `blit_Masm`, linked among the zlib objects. |

The copy and the fill are two objects, not one: the linker's `0xCC` fill sits between them, and inside one assembler
object the alignment filler is a no-op instruction. The 16-bit code segment `_TEXT16` (`int 1Bh; iret; iret`, the PE
section of that name) is assembler output as well; nothing references it, and the image cannot tell which of the six
objects defined it, so it is placed in `MixerAsm.asm`. The file names, and the names of procedures that no C caller
names, are inferred: the image records none. ML 6.13 was only ever released as a patch to an earlier MASM; the setup
script applies Microsoft's patch to the ML 6.11d of the Windows 98 SE DDK ([REPRODUCIBILITY.md](REPRODUCIBILITY.md)).
