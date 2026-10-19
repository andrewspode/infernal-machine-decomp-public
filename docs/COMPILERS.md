# How the original toolchain was identified

`Indy3D.exe` version 1.0 (SHA-256 `3fbaf8cd…1a53`) was built from the output of four compilers and one assembler,
plus prebuilt library members. This page gives the evidence for each, the alternatives tested and how they were ruled
out, the compiler flags and how they were established, and the build-host problems that had to be solved to reproduce
the original bytes. Every measurement was taken against the retail executable and, unless it says otherwise, is
relocation-masked: two functions count as identical when every byte is equal once the linker-filled address fields are
masked.

| Code | Compiler | Flags |
| --- | --- | --- |
| The game: `Jones3D/`, `Libs/sith`, `rdroid`, `std`, `sound`, `wkernel`, `w32util` (131 files) | Microsoft Visual C++ 5.0 with Service Pack 3: driver `cl.exe` 11.00.7022, front end `c1.dll` 11.00.7113, back end `c2.exe` **11.00.7303** | `/O2 /Ob1 /Oi /Ot /Oy /G5 /ML /Gf /Gy /DNDEBUG` for every file |
| `stdUtil_StrCmp`, the one function of its own object (`stdUtil_StrCmp.c`), see [below](#visual-c-420-for-one-helper-object) | Microsoft Visual C++ **4.20**, C1/C2 10.20.6166 | `/O2 /Ob1 /Oi /Ot /Oy /G5 /ML /Gf /Gy /DNDEBUG /W3` |
| The SMUSH/INSANE movie player and its `SmushPlay` glue, zlib 1.0.9 | Microsoft Visual C++ 6.0 RTM, **12.00.8168** | `/O2 /Zp1 /Gy` |
| The "Katmai" SSE renderer `rdModel3K` and three `libm` helpers (`floor`, `ceil`, `modf`) | Intel C/C++ Compiler **4.0**, build 99100 | per object, see [below](#intel-cc-40-the-sse-renderer-and-libm) |
| INSANE's six hand-written assembler files (`*.asm`) | Microsoft Macro Assembler **6.13.7299** | `/coff` |
| The C runtime | not compiled: members of Visual C++ 5.0 SP3's `libc.lib`, linked as shipped | |
| DirectInput data formats | not compiled: members of the DirectX 6.1 SDK's `dinput.lib`, linked as shipped | |

The executable was linked by LINK 5.10, the Visual Studio 97 Service Pack 3 linker.

The 1.2 update (SHA-256 `55fb00c0…d608bf`, [VERSIONS.md](VERSIONS.md)) was built with the same compilers, flags and
libraries and linked in the same order: every file 1.2 did not change compiles to its bytes from the same source. The
evidence below was measured on 1.0. Two period tools generated part of the source rather than compiling it: Berkeley
yacc 1.4 and flex 2.3.6 produced the Cog script parser and scanner (`sithCogYacc.c`, `sithCogFlex.c`) from `cog.y` and
`cog.l`. Their output is checked in; rebuilding the generators is optional
([REPRODUCIBILITY.md](REPRODUCIBILITY.md#optional-the-cog-parser-generators)).

## Contents

- [What the executable records](#what-the-executable-records)
- [Visual C++ 5.0 SP3 for the game code](#visual-c-50-sp3-for-the-game-code)
- [Visual C++ 4.20 for one helper object](#visual-c-420-for-one-helper-object)
- [Visual C++ 6.0 RTM for INSANE, SmushPlay and zlib](#visual-c-60-rtm-for-insane-smushplay-and-zlib)
- [Intel C/C++ 4.0: the SSE renderer and libm](#intel-cc-40-the-sse-renderer-and-libm)
- [The linked libraries](#the-linked-libraries)
- [Compiler flags and how they were established](#compiler-flags-and-how-they-were-established)
- [The debug build's flags](#the-debug-builds-flags)
- [Running 1990s compilers on a modern host](#running-1990s-compilers-on-a-modern-host)
- [Hypotheses, tests and results](#hypotheses-tests-and-results)
- [What is still open](#what-is-still-open)

## What the executable records

The PE header says the image was linked on **1999-10-29 at 19:09:40 UTC** by a linker reporting version **5.10**. The
DOS stub carries a *Rich header*, a table that the Microsoft linker writes. It counts the linked objects by the tool
and build that produced each one, using the `@comp.id` stamp that a compiler leaves in its objects. Decoded:

| Tool (product ID) | Build | Objects | What they are |
| --- | --- | ---: | --- |
| no stamp (0) | 0 | 530 | Objects with no `@comp.id`. Visual C++ 5.0's back end writes none (checked for RTM, SP1 and SP3, while VC6's `C2.DLL` does), so every VC5-compiled game file is counted here, together with the C runtime members (the VS97 SP3 `libc.lib` members carry no stamp either). |
| `Utc12_C` (10), VC6 C compiler | 8168 | 39 | VC6 RTM objects. zlib's units and the INSANE/SmushPlay units are the known VC6-built code. The count does not reconcile exactly. |
| `Utc12_C` (10) | 8447 | 1 | One object built by a VC6 SP3 compiler. Not attributed (see [What is still open](#what-is-still-open)). |
| `Masm613` (14) | 7299 | 6 | Objects from the ML 6.13 assembler: INSANE's six hand-written assembler files, reassembled byte for byte by the pinned ML 6.13.7299 ([INSANE.md](INSANE.md#the-assembler-objects)). |
| `Linker512` (19) | 8034 | 7 | Import-library members. The DirectX 6.1 SDK's import libraries carry 5.12.8034 stamps. |
| `Cvtres500` (6) | 1668 | 1 | The resource converter `cvtres.exe` 5.00.1668, byte-identical to the copy in the Visual C++ 5.0 SP3 package. |

The header pins the **linker**. It is written by the VS97 SP3 `link.exe` (5.10.7303), and the VC5 RTM linker writes
none. The linker version agrees with the PE field. The header does **not** pin the compiler of the game code, because
VC5 objects are unstamped. That had to be read from the code itself.

The image also tells which SDK headers were used. The `.rdata` section holds a block of 121 DirectX GUIDs in header
definition order (`dinput.h` 37, `ddraw.h` 13, `dplay.h` 12, `dplobby.h` 22, four GUIDs found in no SDK, `dsound.h`
10, `d3d.h` 23). 98 of them are never referenced, so this is one object compiled with `INITGUID`, not `dxguid.lib`. The
per-header counts match the **DirectX 6.1** SDK exactly. DirectX 6.0's `dplobby.h` has an extra `DPAID_MaxMessageSize`
between two GUIDs that are adjacent in retail. DirectX 7.0a's headers unconditionally define `IID_IDirectInput7A` and
others that are absent. DirectX 5 lacks `IID_IDirect3D3` and `IID_IDirectDraw4`, which are present. `stdControl` calls
`DirectInputCreateA(hInstance, 0x500, …)` (`push 0x500` at `0x004B789F`), which is `dinput.h`'s default
`DIRECTINPUT_VERSION` in the DirectX 5 and 6 headers.

## Visual C++ 5.0 SP3 for the game code

### First evidence: VC6 could not compile two simple files correctly

The project inherited the assumption that the game was built with Visual C++ 6.0. `stdCircBuf.c` had two functions
that no source rewrite could make identical under VC6 (multiplication operand order, an explicit size temporary and
the order of two independent field stores all changed nothing). The same, unmodified source compiled with Visual C++
5.0 (11.00.7022) reproduced all four functions and the file's `.data` byte for byte, and `rdPrimit2.c` behaved the same
way. Switching the default compiler to VC5 raised the exact functions of the time from 402 to 453 and lost none.

### Retail code shapes that separate VC5 from VC6

Three properties of the retail code are VC5 properties:

- **Argument cleanup after every call.** VC6 merges the `add esp,N` of consecutive calls, and VC5 pops after each
  call. Counting `add esp` per `call` over each file's retail `.text` matches VC5's output and not VC6's almost
  everywhere. For example, the ratio for `sithControl` is 0.94 in retail, 0.92 under VC5 and 0.35 under VC6; for
  `stdDisplay` it is 0.78 / 0.78 / 0.57.
- **Unpooled float literal blocks.** Retail `.rdata` holds a separate literal block per object, with repeated values:
  0.0, 1.0, 0.1 and 1000.0 recur in consecutive blocks at `0x005075B4-0x00507680`. VC6 emits `__real@` COMDATs that
  the linker would fold into one copy each. These blocks are VC5 `$T` output: `sithTime`'s retail block
  `0x00507400-0x00507414` is byte-identical to its VC5 `.rdata`.
- **Exact functions.** The 78 files still unfinished at the time were compiled with five builds (VC5 RTM, SP1, SP3,
  VC6 RTM and VC6 SP3) and scored per function. **VC6 RTM and VC6 SP3 each lost exact functions in 74 of the 78
  files and beat the configured VC5 build's code score in none.** In 27 files that had been left on VC6, switching to
  VC5 raised exact functions from 147 to 232 with none lost.

### RTM, SP1 or SP3: two tells in the retail bytes

The three Visual C++ 5.0 packages share one `cl.exe` driver (11.00.7022), but each has a different code generator
(`c2.exe`: RTM, 11.00.7153 in SP1/SP2, 11.00.7303 in SP3), and SP1 to SP3 share a `c1.dll` front end (11.00.7113).
Two instruction patterns separate RTM from the service packs:

- **The jump-table filler.** When a `switch` jump table needs three bytes of alignment, RTM pads with `nop; mov edi,edi`
  (`90 8B FF`) and the service packs with `lea ecx,[ecx]` (`8D 49 00`). An image-wide scan of every `jmp [reg*4+table]`
  finds **35** service-pack fillers and **zero** RTM fillers. The one- and two-byte pads are shared by both builds,
  so only the three-byte pad discriminates.
- **The x87 constant store.** RTM's back end never stores a single-use float constant through the x87 (`fld [K]` …
  `fstp [m]`). It always emits `mov [m],imm32`. SP1 and SP3 emit the x87 form inside x87-scheduled code (0 of about 230
  RTM probe cells, many SP cells). Retail has **62** such stores in 18 files.

32 files show one tell or both, and every one of them shows the service-pack form. Of the 2,910 functions measured at the time, **none is
exact only under RTM**. Under SP3 every matching file stayed matching; one function,
`sithCollision_ParticleAndActorCollisionHandler` (`0x004AAC90`), was then exact only under RTM and SP1.

No tell in the retail bytes separates SP1 from SP3. SP3 was chosen because the retail executable was linked with the
SP3 linker, and because five functions are exact only under SP3 (`rdVector_Dot3`, `rdFont_DrawTextLine`,
`sithPhysics_SetThingLook`, `sithFX_CreateThingOnSurface`, `sithThing_AttachThingToClimbSurface`) against one exact
only under SP1 (the `sithCollision` function above). The choice of SP3 over SP1 is an inference.

Every alternative build that was tested, and what refuted it, is in [the table below](#hypotheses-tests-and-results).

## Visual C++ 4.20 for one helper object

`stdUtil_StrCmp` (retail `0x004E09A0`, 79 bytes) is the one function of a separately built object: the debug executable
carries a byte-identical copy (apart from the `tolower` call displacement) that is optimised although every other
function around it is `/Od`, so the object was compiled once and linked into both builds. Its fingerprint is the image's
only `mov eax,0xFFFFFFFF; pop; ret` (VC5 and VC6 emit `or eax,-1`) and a second `tolower` argument loaded only after the
first call's `add esp,4` (VC5 and VC6 schedule it earlier). Every Microsoft C compiler from Visual C++ 2.0 to 4.20 gives
exactly these bytes at `/O2`; none of VC5 RTM, SP1, SP3 and VC6 RTM and SP3 does in 420 flag cells (best: 10
differing instructions). The project uses Visual C++ 4.20 (the newest of them), through
[archaic-msvc/msvc420](https://github.com/archaic-msvc/msvc420). The object's file name and path are inferred.

## Visual C++ 6.0 RTM for INSANE, SmushPlay and zlib

The movie player is LucasArts' SMUSH/INSANE library. Each module registers a `LECMODULE=insane_*` name, and its
assertion texts carry paths such as `E:\smush\INSANE\SOURCE\Image\Readauto.c`, so it was built in a different tree
from the game (`C:\Jones3D\…`). Three code shapes in this code are VC6 and not VC5:

1. **Merged cdecl cleanup.** Two consecutive calls end with one `add esp,0x10`, where VC5 emits `add esp,8` after each.
2. **`void` functions end with `call f; pop ecx; ret`.** An unused result lets VC6 clobber `ecx`, while VC5 always
   emits `add esp,4`.
3. **Shrink-wrapped prologues.** `push esi` comes after an early-out test (`readauto_Read`). This is a VC6 optimisation.

**zlib 1.0.9** settled the flags. The unmodified published sources compiled with VC6 RTM at `/O2 /Zp1` give 61 of
their 95 functions byte-identical to retail. These are the whole inflate path plus `gzopen`/`gzclose` and parts of
deflate; the other 34 are functions retail does not link. VC5 SP3 at the same flags reproduces 8 of the 30 inflate-path
functions. Without `/Zp1` the inflate state structures are laid out differently. `/Zp1` is not a zlib default, so zlib
was built with the INSANE project's settings, and the same settings reproduce INSANE: `/Zp1`, `/Gy` (16-byte-aligned
NOP-filled function COMDATs) and `/O2` are confirmed by its verified units. A sweep of `/O1 /Os /Ot /Ox /G3-/G6 /Ob0-2
/GF /Gf /GX /Oy-` on `insane_stdio` left only `/O2` and `/Ox` (the same option set) reproducing all five functions.
zlib's `gzio.c` was modified for the game: stdio is replaced by INSANE's FILIO function table.

## Intel C/C++ 4.0: the SSE renderer and libm

### Identification

A 4,496-byte block at `0x004E1DA0` is the "Katmai" renderer `rdModel3K` (`rdModel3K_Startup`, `DrawMesh`,
`IsAvailable`, …). Its original path, preserved in the debug build, is `C:\Jones3D\Libs\Katmai\source\rdModel3K.c`.
It is not Microsoft output:

- 65 `movaps`, 28 `shufps`, `cvtps2pi` and `emms`. No Microsoft compiler of 1999 emits SSE instructions.
- Every function has an aligned-stack dual entry: `push ebx; mov ebx,esp; sub esp,8; and esp,-16; add esp,8; jmp`,
  then a second entry.
- Padding between functions is `lea esi,[esi]` / `lea edi,[edi+0]`, not `0x90` or `0xCC`.

Three x87 `double` helpers right after it (`0x004E2F30`, `0x004E2FE0`, `0x004E3000`) have the same prologue and
padding and read constants from a section named `.data1`. They behave like `floor`, `ceil` (`-floor(-x)`) and `modf`.
The game's `floor`/`ceil` calls go to them, and LIBC's `floor.obj`/`ceil.obj` are not linked: the Intel compiler's
math library took precedence over Microsoft's.

Intel C/C++ 4.0 (1999) is the release that added Streaming SIMD Extensions support. Version 4.5 (2000) postdates the
game. Intel offered a free 14-day evaluation of 4.0. It survives as `design/perftool/icl/compeval.exe` on Intel's
*Developers Insight CD-ROM, May 1999*, which is preserved on archive.org (5,102,735 bytes, SHA-256
`931fecd1…801131746e`). Its compiler reports **Intel(R) C/C++ Compiler Version 4.0 99100**.

### Results

A test file using the `xmmintrin.h` intrinsics, compiled with `-O2 -QxK`, reproduced retail's exact dual-entry
prologue (with the same `0x38` frame as `rdModel3K_Startup`) and the `movss/shufps/movaps/xorps/emms` sequence. The
first functions then matched exactly:

| Retail | Function | Flags | Result |
| --- | --- | --- | --- |
| `0x004E1DA0` | `rdModel3K_Startup` | `-O2 -QxM` | identical, 352/352 bytes |
| `0x004E2ED0` | `rdModel3K_IsAvailable` | `-O2 -QxM` | identical, 48/48 |
| `0x004E2F00` | `rdModel3K_SetEnabled` | `-O2 -QxM` | identical, 32/32 |
| `0x004E2F20` | `rdModel3K_IsEnabled` | `-O2 -QxM` | identical, 16/16 |
| `0x004E2F30` | `floor` | `-w -Zl -Di386 -QIfdiv- -Op` | identical, 176/176 |
| `0x004E2FE0` | `ceil` | same | identical, 32/32 |
| `0x004E3000` | `modf` | `-O2 -QxM -Qsfalign-` | 61 of 63 instructions align; the register allocation differs |

How the flags were established:

- **`-QxM`, not `-QxK`, for rdModel3K.** `-QxK` emits the P6 compare `fcomip`, while retail's `DrawMesh` uses
  `fcomp; fnstsw ax; sahf`. The SSE intrinsics still compile under `-QxM`, and `Startup` is identical under both.
- **The libm helpers' build line is in Intel's own library.** Each member of the 99100 `libm.lib` has a `.comment`
  record with `-c -w -Zl -Di386 -QIfdiv-` (plus `-Op` for `floor.c`) and no `-Qx`. `floor`/`ceil` need `-Op` (the x87
  intermediates are spilled: `sub esp,0x48`, `fstp/fld [esp+0x10]`). `modf` is not `-Op` and needs `-Qsfalign-`
  (retail has no aligned prologue there: `push esi; push ebp; push ebx`). So the three helpers came from separate
  objects with different options.
- The shipped `libm.lib`'s `floor`/`ceil` *code* matches retail, but its `.data1` constant pool is laid out
  differently, and its `modf` (two pushes, dual entry) differs from retail's. The helpers are therefore
  reconstructed as C and compiled, not linked from the evaluation's library. Why they do not byte-match is
  [the next subsection](#the-libm-helpers-that-do-not-byte-match).
- Ineffective on the residues measured: `-O1 -Ox -Os -Ot -Ob0/2 -Qip -Oy- -G5 -G6 -Qvc6 -Zp -Qunroll0 -Qsfalign16
  -Qkscalar -Qvec -Qprec -Qprec_div -Qrestrict -Gs -GA -Ge`.

### The libm helpers that do not byte-match

`floor.c` (`floor`, `ceil` and `rint`, whose code LINK dropped as unreferenced) and `modf.c` are the only two source files
that are not byte-exact, and they are Intel's prebuilt library routines rather than game code. In `floor.c` the code of
`floor` and `ceil` is byte-identical after relocation and so is the `.data` block; only the order of the five 8-byte
floating-point constants in `.data1` differs: retail has them in the reverse of the order the 99100 compiler numbers
them, and the same reversal is in other members of retail's libm (`scalb`, `logb`). In `modf.c` the same applies to its
constants and one register choice (61 of 63 instructions align). The compiler places a constant when the statement that
uses it is lowered, so the order follows the order of the expression walk; no compiler option, source order, branch
direction or profile changed it. Source forms that do produce retail's bytes exist (separately named constant objects, an
artificial NaN guard in `modf`), but they are not how Intel wrote its library, and no Intel libm member uses that style,
so the project does not take them: the source form that Intel's build of these routines had is not yet found. Anyone
porting the game would use the platform's maths library for these routines.

The three CPU-probe functions of `rdModel3K` (`0x004E2D80`, `0x004E2E00`, `0x004E2E80`) use `cpuid` inside
`pushad`/`popad` within a structured exception handler. No compiler of the period has a `cpuid` intrinsic or emits
`pushad`, so these are inline assembly in the source. With the blocks as written they compile identically
(128/128, 128/128 and 80/80 bytes).

### The trial patch

The evaluation's driver `icl.exe` checks the 14-day limit and refuses to run afterwards. The compiler proper,
`mcpcom.exe`, has no trial code. The setup script ([andrewspode/intel-cc-4.0-setup](https://github.com/andrewspode/intel-cc-4.0-setup))
unpacks the installer with 7-Zip and unshield (no Wine and no installer run) and writes a copy of the driver,
`icl_notrial.exe`, in which the check function at `0x0040B4D0` is replaced by `mov eax,1; ret`. The patched driver is
verified by SHA-256 (`fed5dc86…ad81`). Nothing the compiler generates is affected: the patch only removes the date test.

## The linked libraries

**The C runtime is Visual C++ 5.0's LIBC, not VC6's.** It occupies the end of `.text`, from `sprintf` at
`0x004F64B0` to `0x005049C6`; the entry point `0x004F73C0` is LIBC's `WinMainCRTStartup`. Matching the members of each
candidate archive against this range: the VS97 SP3 `libc.lib` places and verifies 161 members and leaves 54 bytes
unexplained, while VC6 RTM's `LIBC.LIB` verifies 23 (the hand-written assembler members the two share) and leaves
50,441 bytes unexplained. It is the single-threaded LIBC (no `TlsAlloc`/`TlsGetValue` import), hence `/ML`. 159 of
the linked members are reproduced as library units: the build extracts them, unmodified, from the pinned archive, and
the same strict check as compiled code verifies them. Two cases show how closely the linker's choices were followed:

- **Identical-COMDAT folding crosses the game/runtime boundary.** LIBC `fpinit.obj`'s `ret`-only `__fpclear` is the
  game's `rdCanvas_FreeEntry` at `0x004EE700`: fpinit's function pointers hold that address.
- **A member can contribute only data.** `ieeemisc.obj` needs `_ldexp`, so `ldexp.obj` was linked. `/OPT:REF` then
  dropped its unreferenced code and left its two constants (-1.0, 0.0) at `0x0050BBF0`.

**DirectInput data formats.** `c_dfDIJoystick`, `c_dfDIKeyboard` and `c_dfDIMouse` (5,008 bytes at `0x004E0A10`) are
the DirectX 6.1 SDK `dinput.lib` members `dilib3.obj`, `dilib2.obj` and `dilib1.obj`. They are byte-identical after
relocation, and their 13 GUID references resolve to the game's GUID block.

**Import libraries.** Every import hint, ordinal and name in the rebuilt import tables comes from the libraries retail
linked: the VS97 SP3 platform libraries and the DirectX 6.1 SDK's `ddraw.lib`, `dsound.lib` and `dinput.lib`. The
SDK's `ddraw.lib` is required: VS97's own copy has hints 8/6, and retail has 9/7.

**Resources.** The `.rsrc` section is rebuilt from a resource script that compiles, with RC.EXE's rules, to a
byte-identical section. The period VS97 SP3 `rc.exe` compiles the same script to the same 79 resources.

## Compiler flags and how they were established

The game files all use `/O2 /Ob1 /Oi /Ot /Oy /G5 /ML /Gf /Gy /DNDEBUG`, and the warning level `/W3`. Each flag was
settled by a byte comparison:

| Flag | Evidence |
| --- | --- |
| `/G5` | `/G3`, `/G4` and no `/G` produce identical code, while `/G6` breaks previously exact functions (5 of 11 in `sithConsole.c`) |
| `/Ob1` | `/Ob2` breaks exact functions (3 of 11 in `sithConsole.c`). `/Ob1` expands every `__inline` function within VC5's size limits, which retail shows |
| `/ML` | retail calls the single-threaded LIBC (no TLS imports; `Driver.c` reads the global `errno`; `sithCogFlex.c` expands `putc` inline). Identical code everywhere else |
| `/Gf` | `/GF` puts pooled strings in read-only `.rdata` and `/Gf` in writable `.data`. Every game file's literals are in retail's `.data`, and VC5's `/O2` implies `/Gf` by itself. Eight files that had been missing their filename literal gained it with `/Gf`: `.data` went from 0-20% to 85-100% matched, with no code byte changed |
| `/Gy` | each function is a COMDAT, 16-byte aligned, as in retail |
| per-file filename | every `__FILE__` literal in retail is the original path, such as `C:\Jones3D\Libs\sith\Gameplay\sithEvent.c`. The build passes each one through `/DJ3D_FILE=…` with the exact case retail shows |

**The flag matrix.** Every other `/O`, `/G`, `/Zp` and ANSI-mode flag the compilers accept was appended to each file in
turn and measured in byte-exact functions. That was 1,136 flag × file cells, and `/Oi-` on `sithInventory.c` was the
only improvement (28/32 -> 30/32 there, net -254 exact functions project-wide). It turned out to be a declaration-count
effect: with `/Oi` VC5 hoists a field load above an `if`/`else` in two functions, and the file's labelled count-fit
block reproduces retail's per-arm loads under the uniform flags, so every game file builds with one flag set. `/Op` ("improve float consistency"), measured separately on the 74 files not yet matching at the time, gained no function
anywhere and would cost 248 exact functions if adopted project-wide.

**`#pragma optimize` regions.** Some functions were compiled with different optimisation, set inside the source:
whole-function `#pragma optimize("", off)` regions (`stdMath.c`, `sithAnimate.c`, `rdVector.c`, `jonesInventory.c`,
INSANE's `D_bloc16.c` and others). The evidence is in the bytes, in files whose other functions are `/O2` code: a
function in an unoptimised region has `[ebp-N]` locals in identifier-hash order and `0xCC` fill at the region's
boundary, and removing the pragma breaks it. Optimised functions that start with `push ebp; mov ebp,esp` (`55 8B EC`)
are not a pragma: each expands one of `stdMath.h`'s inline `__asm` helpers (`sqrtf`, `floorf`, ...), and a function
containing an `__asm` block gets an EBP frame from VC5 by itself, so no frame-pointer pragma is written around them.

## The debug build's flags

The Italian release ships a debug executable of the same source ([DEBUG-BUILD.md](DEBUG-BUILD.md)). It is Visual C++
5.0 `/Od` with **no inline expansion** (`/Ob0`, the compiler's default under `/Od`, which is the stock "Win32 Debug"
setting), and without `/Gy`. That was not obvious at first. Some of its functions look as if they expanded a helper, which
suggested `/Od /Ob1`. Under `/Od /Ob1`, however, VC5 expands *every* `__inline` function, while the debug image calls the static
and assembly helpers out of line. Under `/Ob0` the "expanded" sites turn out to be macros or text written out in the
original. The stack-slot layout proves it where the two can be told apart: a macro's temporaries interleave with the
caller's, while an inlined parameter's home is grouped. Compiling every paired function under `/Od /Ob0`, 1,434 were
at the time identical to the debug image against 1,417 under `/Od /Ob1`, and **no function needs `/Ob1`**. No added flag changes
VC5's inlining under `/Od`. `/Zi /Z7 /Zd /Gm /GX /Gf /GF /Gs /Gr /Gz /Ge /Gh /Gi /G3 /G4 /G6 /GB /MLd /MTd /MDd /Za /Zl
/Zp1 /Zp8 /J /Oi /Ow /Oa /Oy- /Op /Ot /Os /Og` and `#pragma auto_inline(off)` were all tried.

## Running 1990s compilers on a modern host

### wibo

All compilers run on Linux and macOS through [wibo](https://github.com/decompals/wibo) 1.1.0, a minimal loader for
Windows command-line programs. A natural worry is that the host changes the output.

**For Microsoft's compilers it does not.** Every VC5 SP3 and VC6 RTM unit (162 of them) was recompiled under 16
deliberately different memory layouts:

- the heap arena moved to two other bases;
- an NT-like first-fit allocator, with 8- and 16-byte headers;
- a mirrored allocator, so that later blocks get lower addresses;
- two seeded random placements;
- fresh memory filled with `0xCD`, `0xFF` and `0x5A`;
- `VirtualAlloc` reservations moved;
- the stack moved;
- the runtime's own `qsort`.

That was 2,592 compiles, **all byte-identical** to the production build. The compilers never call `qsort`, `rand`, or
the other routines whose results could depend on the host. Independently, five units compiled natively on Windows 11
(`rdVector`, `sithCollision`, `sithPhysics`, `rdClip`, `jonesConfig`) were identical to the wibo objects in every
section.

### The Intel compiler's heap problem

**Intel's compiler is different.** `mcpcom.exe` 99100 has a reassociation pass that sorts the operands of every
commutative chain by a key that is the **address of each leaf's symbol record in the compiler's heap** (a composite
leaf's key is the 30-bit sum of its leaves' keys, higher first). Operand order, and everything downstream of it
(address base and index, x87 order, register choice), therefore depends on where the host puts the compiler's heap.

- Under wibo the heap is a mimalloc arena at `0x6c000000`. There every record address is above 2^30, so every plain
  symbol outranks every composite. This was the rule earlier measurements had taken for the compiler's behaviour.
- The same source compiled natively on Windows 11 gave a **different** `DrawMesh` (2,000 bytes against wibo's 2,048,
  retail 2,048). A real NT heap changes the output, and Windows 11's is not 1999's.

The target is the heap of the 1999 build machine, so it was modelled from first principles rather than tuned. The
compiler's own runtime is a static VC5 CRT: it calls `HeapCreate(HEAP_NO_SERIALIZE, 0x1000, 0)`, keeps a small-block
heap with VC5's 0x1E0 threshold in 4 MiB reservations, and makes 242 `HeapAlloc` calls for `rdModel3K.c`. The model is
the documented NT 4/5 backend heap: 8-byte headers, exact-size free lists and a size-ordered list 0, coalescing,
decommit thresholds and 1 MiB segments. No lookaside list or low-fragmentation heap applies, because the heap is
`HEAP_NO_SERIALIZE`. It runs in the address layout of an NT4 console process for this image. The results:

- The model reproduces the native Windows 11 object **exactly**: all code sections and every relocation.
- Every principled variant gives that same object: Vista-style LIFO order, no decommit, other heap bases, the XP
  header size, Server 2003 segment growth, the CRT small-block heap off, address-ordered first fit, and a Win9x
  private arena with 4-byte headers.

The reason is that every heap of a 1999 Windows process sat at a few MiB (NT) or just above `0x00400000` (Win9x). There
no key sum wraps at 2^30, so the comparison depends only on the order of allocation, not on the layout.

The model is a patch to wibo (`WIBO_HEAP_MODEL=nt`) that gives every `HeapCreate`d heap the NT backend model. Every
compile in the build runs under it. The 162 Microsoft-compiled units are byte-identical under the patched and the
unpatched wibo, as the heap study predicts. Of the Intel units only `rdModel3K.c` changes, and it now equals the native
Windows object. With the faithful heap and its source corrected, `DrawMesh` has retail's 2,048 bytes and 499
instructions, and the whole file now matches. The patch and its
build script are part of this repository (`tools/icl/wibo-heap-model.patch`, `Scripts/build_wibo_heapmodel.py`), and
the patch is offered upstream as [decompals/wibo#153](https://github.com/decompals/wibo/pull/153).

## Hypotheses, tests and results

| Hypothesis | Test | Result |
| --- | --- | --- |
| The game is VC6 RTM (inherited assumption) | `stdCircBuf.c`, `rdPrimit2.c` under VC6 and VC5, source unchanged | VC6 cannot produce two of `stdCircBuf`'s functions with any rewrite; VC5 11.00.7022 reproduces the file. **Refuted** |
| Some game files are VC6 | the 78-file × 5-build probe; `add esp`/`call` ratios; literal pooling | VC6 worse in 74 of 78 files and better in none; retail pops per call and keeps per-object `$T` blocks. **Refuted for every game file** |
| The game is VC5 RTM | the jump-table filler and x87 constant-store tells; RTM-only exact functions | 35 SP fillers, 0 RTM fillers; 62 SP-only x87 stores; no function exact only under RTM. **Refuted** |
| SP1 rather than SP3 | per-function comparison; the linker version | no byte tell; 5 functions exact only under SP3, 1 only under SP1; the linker is SP3's. **SP3 adopted (inference)** |
| VC6 SP3 built game code (the 8447 Rich entry) | 10 files, every function | scores identically to VC6 RTM on every function; still merges pops (`sithCog`: 225 `add esp` against retail's 337). **Refuted for game code** |
| A pre-release VC6 (Beta 2, driver 12.00.8047.2) | stamp; x87 preload probe, 118 cells; `sithSoundClass.c` | Beta 2 stamps build 8047, absent from the Rich header; preload in 0 of 118 cells; its code equals VC6 RTM's (10/25 exact against VC5 SP3's 22/25). **Refuted** |
| A back end between VC5 SP3 and VC6 RTM explains the leftover scheduling residues | release history; all pinned builds on the residues | no released build exists in the interval; all builds agree with each other. The motivating `sithItem_ParseArg` (`0x004AF0C0`) and `sithExplosion_ParseArg` (`0x004ADC80`) shapes were reproduced under plain VC5 with an `int` value temporary assigned before an intervening statement. **Rejected** |
| Visual C++ 4.2 builds the game code | a matched file and eight others | `stdCircBuf.c` falls to 68.8% (0 of 4 exact); 206 -> 36 exact over the eight. **Refuted for the game code** (one helper object is built with it, see [below](#visual-c-420-for-one-helper-object)) |
| A different VC5 front end | all six crosses of the two VC5 front ends with the three back ends | byte-identical to the natural pairings. **Refuted** |
| Optimisation and CPU flags differ from `/O2 /G5` | 1,136 flag × file cells; `/Op` on 74 files | one positive cell (`/Oi-` on `sithInventory.c`). **Base flags confirmed** |
| `/MT` (LIBCMT) | imports and CRT call targets | no TLS imports; global `errno`. **`/ML` adopted** |
| The CRT is VC6's | member placement against both archives | VS97 SP3 `libc.lib` verifies 161 members, VC6 23. **VC5 LIBC** |
| zlib and INSANE are VC5 | the unmodified zlib 1.0.9 source under both | VC6 RTM `/O2 /Zp1`: 61 functions identical; VC5: 8 of 30. INSANE shows VC6-only shapes. **VC6 RTM** |
| rdModel3K is Intel C/C++ 4.0 | `-O2 -QxK` probe, then real functions | prologue reproduced; four functions and `floor`/`ceil` identical. **Confirmed** |
| The libm helpers can be linked from Intel's `libm.lib` | member comparison | code matches but the `.data1` layout and `modf` differ. **Not linkable; reconstructed as C. Source form of retail's constant order not yet found** (see [above](#the-libm-helpers-that-do-not-byte-match)) |
| Remaining MSVC residues are wibo artefacts | 16 layouts × 162 units; native Windows 11 | 2,592 identical compiles; native identical. **Refuted** |
| Intel output depends on the host heap | compiler internals read; native Windows 11; a 1999 heap model | the sort key is a heap address; the model reproduces native Windows exactly. **Confirmed; the build uses the model** |
| DirectX 6.0 or 7 headers | GUID block contents and `DIRECTINPUT_VERSION` | counts match DirectX 6.1 only. **DirectX 6.1** |
| The debug build is `/Od /Ob1` | static and assembly helpers called out of line; slot layout; whole census | `/Ob0` explains every site; no function needs `/Ob1`. **`/Od /Ob0`** |

## What is still open

- **The single `Utc12_C` 8447 object.** The Rich header proves that one object was compiled by a VC6 SP3-era compiler.
  It is not in the DirectX libraries and is not the C runtime. VC6 SP3 behaves identically to VC6 RTM on everything
  measured, so it could be one of the INSANE modules. INSANE has not yet been compiled with VC6 SP3.
- **SP1 against SP3** rests on the linker and on per-function counts, not on a byte tell.
- **The constant order in Intel's `floor` and `modf`.** The source form that gives retail's `.data1` constant order
  is not yet found ([above](#the-libm-helpers-that-do-not-byte-match)).
