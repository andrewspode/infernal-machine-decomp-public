# The Italian release's debug executable

The Italian release of the game, *Indiana Jones e la Macchina Infernale*, ships an unoptimised **debug build** of
`Indy3D.exe` next to its release build. It was compiled from the same source tree as the release, a few minutes
earlier. It is the strongest outside evidence this project has: a second compile of the original source with almost no
optimisation, so its code follows the original text closely. This document describes what the debug build is, what it
can and cannot prove, what it settled, and how to build the game with the same debug switch (`ninja debug`).

Thanks to [smlu](https://github.com/smlu), the OpenJones3D maintainer, for pointing us to it: we would never have known
it was there. It was also the basis for much of OpenJones3D's own work, because unoptimised code is far easier to turn
back into readable C.

## Where it comes from

The disc set is preserved on archive.org (item `2-indiana-jones`, archive `2_Indiana_Jones.rar`, folder
`La Macchina Infernale`). The executables are not loose files. They sit inside the resource archives
`Resource/CD1.GOB` and `Resource/CD2.GOB`, and both archives hold identical copies:

| File | Size | MD5 | Linked (PE timestamp, UTC) | What it is |
| --- | ---: | --- | --- | --- |
| `indy3d.exe` | 2,135,040 | `7b4623536572731044f80782101b9063` | 1999-10-28 23:32:12 | debug build |
| `indy3dr.exe` | 1,539,072 | `4b86cd6dbd2de5502ece7bcf37a495f7` | 1999-10-28 23:45:54 | Italian release build |
| `Indy3D.exe` (English CD v1.0, which this project rebuilds alongside the 1.2 update) | 1,539,072 | (SHA-256 `3fbaf8cd…1a53`) | 1999-10-29 19:09:40 | English release |

These GOB files use format version 0x14. The file starts with `"GOB "`, then a 32-bit version (0x14), the value 0x0C,
and a file count at offset 12. Then follow `count` entries of `{u32 offset, u32 size, char name[128]}` starting at
offset 0x10. The Italian release build lies at offset 0x291C90 of `CD1.GOB`.

## What it is

- **A debug compile of the same tree.** The debug directory has a CodeView `NB10` entry naming
  `C:\Jones3D\Product\Indy3D\Debug\Indy3D.pdb` (the PDB was not shipped), an FPO table and a misc entry. There are no
  symbols.
- **Unoptimised code.** Functions use `ebp` frames and keep their locals in memory. Debug-only code is compiled in:
  1,505 assert sites against retail's 1,332, plus debug prints and whole debug-only functions. Sites present only in
  the debug build are the plain `SITH_ASSERT`-family macros, which compile to nothing in release; the asserts kept in
  release carry a `REL` suffix (the same split OpenJones3D's source uses). The debug C runtime
  (`dbgheap.c`, `dbgrpt.c`, …) is linked. `.text` is 0x162F46 bytes, against retail's 0x1039C6.
- **Compiler and flags.** Visual C++ 5.0, `/Od` with **no inline expansion** (`/Ob0`) and without `/Gy`. How this was
  established is in [COMPILERS.md](COMPILERS.md#the-debug-builds-flags). Every `__inline` function is called, not
  expanded. Static ones are emitted in their own file's code. External ones become one "pick any" COMDAT placed after
  the code of the first file that uses them. So anything that looks expanded in the debug code was a macro, or text
  written out in the original.
- **The same source, line for line.** Every diagnostic passes `__FILE__` and `__LINE__`, so each image carries a set of
  (file, line) keys: 3,356 in all. 2,827 keys occur in both images, and wherever a site can be aligned the line
  numbers agree.
- **The same tree as the Italian release.** The debug build and the Italian release (linked 13 minutes later) agree on
  every `__LINE__` in every file.
- **Close to the English release.** The English release was linked about 20 hours later. Its code differs from the
  Italian release in only three files, `JonesMain.c`, `sithCamera.c` and `sithSector.c`, the three files whose
  `__LINE__`s drift between the builds. Every function of the English retail was located in the Italian release and
  compared with addresses normalised. The only genuine differences are in `JonesMain_HandleWMKeydown`,
  `sithCamera_Update` and the two `sithSector` sector-list readers. The other byte-level hits are switch-table and
  function-folding artefacts. Outside those three files, the debug build is therefore a
  compile of the English retail's source. In those three files the debug text is the previous revision, and retail
  wins.

### What its strings reveal about the original tree

The debug image names 125 game source files, 122 of which map onto this repository's files by path; the others, and
the original directory layout, are in [SOURCE-TREE.md](SOURCE-TREE.md). 470 (file, line) keys exist only in the debug
image: lines whose statements the release build dropped (210 asserts, 232 log prints, 28 allocation calls).

## What it can and cannot prove

Under `/Od` almost every source construct leaves a trace, but not all of them:

| Visible in the debug build | Not visible in the debug build |
| --- | --- |
| statement order, control-flow shape (early return or wrapped body), `switch` case order | **parentheses**: a parenthesised subexpression compiles the same with or without its parentheses under `/Od`, but not under `/O2` (see below) |
| the number and types of locals (the frame size); block nesting, through how slots are grouped | **declaration order and local names**: slots are ordered by a hash of the identifier, not by declaration |
| a macro versus a function call | which of two spellings of a macro argument the author typed |
| shared error blocks placed after the success `return`, and jumps out of loop nests (no tail merging, source-order layout) | the names of labels |
| parameter types, through compares (`jb` against `jl`) and pushed widths | exactly where in a function a block-scoped declaration sits |
| conversion temporaries (`(float)atof(…)`), `memcpy` calls for vector copies | parenthesised value nodes that leave no temporary |

So the debug code is evidence about the *text*, and the release bytes remain the test. A change taken from the debug
code must still compile to retail's optimised bytes, and where the two disagree the disagreement has to be explained.

### When the debug text and the release bytes disagree

Some functions, written as the debug code shows them, did not compile to retail. Each such conflict was examined as a
set. In no case was it a source change between the builds:

- **Some were misreadings.** For example, a "file-scope descriptor" in `stdDisplay_CreateZBuffer` turned out to be a
  member of a global in both builds.
- **Some were parentheses.** Visual C++ 5.0 keeps a parenthesised subexpression as its own tree node, and that node
  blocks the optimiser's algebraic rewrites. `/Od` emits the same code either way, so the debug build cannot show the
  parentheses. `/O2` rewrites `x * K + K` as `(x - -1) * K` unless the product is parenthesised, and rewrites
  `a + (int)(x * K)` as `a - ftol(x * -K)`. The debug text of `jonesConfig_SetDialogTitleAndPosition` produces
  retail's pooled −128.0 and −96.0 constants through exactly that rewrite. `SoundDriver_GetIntPan` matches both builds
  as `(int)((pan * 128.0) + 128.0)`.
- **Some were scope.** In `sithWeapon_GetAimOrient` the debug text matches in both builds once three locals whose
  address is taken are declared inside `if ( pTarget )`, after the call that returns `pTarget`. The release compiler
  assumes that a pointer returned by a call may point at any address-taken local *in scope at that call*. See
  [COMPILER-INTERNALS.md](COMPILER-INTERNALS.md#1-a-pointer-returned-by-a-call-may-point-at-any-address-taken-local-in-scope).

A few such conflicts are not fully explained: the function is byte-exact in release, but its text still departs from
the debug code where a named compiler-model gap lies *(for example an x87 value that retail keeps on the FPU stack
where the debug text's `float` local would not keep it)*.

## How it was used

### Pairing functions

The debug image has no symbols, so its functions were paired with the named functions of the retail build:

- by a (file, `__LINE__`) key that occurs in exactly one function of each image (1,389 pairs);
- by a unique string literal (168 more; where both keys exist they agree in 865 of 866 functions);
- by identical instruction skeletons, by call-graph propagation (96% right on a held-out check) and by order between
  anchors.

The pair table was then confirmed by compiling each candidate under `/Od` and testing identity against every debug
function start.

### The conformance check

Every function of the project's source is compiled the way the debug build was (`/Od /Ob0`, asserts on) and compared
with its debug counterpart. A function conforms when it is identical, or identical except for which stack slot each
local has (slot order follows a hash of the local's name). Of the 2,905 paired game functions, **2,658 are identical
and 189 identical apart from slot order**; 58 differ, each recorded with its first difference.

When the check was first run, about half of the functions in fully matched files were byte-identical to retail but
**not** written as the original was: optimised code hides many differences in the text. They were rewritten to the
debug text and re-verified against the release bytes. Where a change the debug code proves cost a release match, the
change was kept and the file matched again later.

### What it settled

- **Error labels in loaders.** Loaders and readers end with `return <ok>;` followed by labelled print blocks, one per
  failure kind, and a shared `Close/Free; return <fail>;` tail. Failing reads jump into them with `goto`. In the debug
  code these blocks sit after the success `return` and are entered from inside loops, which no structured `if`/`else`
  produces. Examples are `rdParticle_LoadEntry` and `sithWorld_LoadEntryBinary` (debug `0x004337F7`: error block at
  `0x00433AC3`, entered from six sites). The release build had already suggested this from the order of the prints'
  line numbers. The debug build proves the jumps.
- **Macros versus inline helpers.** With no inlining in the debug build, the test is direct. If the debug code calls a
  helper, the helper is a function. If the debug code writes the helper out, it is a macro. The vector arithmetic
  (`rdVector_Add3`, `Sub3`, `Scale3`, `Dot3`, `Cross3`, `Dist3`, `Copy3`, …) and `STDMATH_CLAMP` are macros. For
  example, `sithCogFunction_VectorDist` computes every component difference twice and has no `dx` slot. The `stdMath.h`
  float helpers, nine collision helpers and `stdControl_UpdateKeyState` are functions.
- **The inline-assembly math helpers are original.** The release build rounds and converts floats with
  `fldcw`/`frndint`/`fistp` sequences no compiler generates from C, which this project writes as small `__asm` helpers
  in `stdMath.h`. The debug build contains their out-of-line copies (`sqrtf` at `0x0043FE70`, `floorf` `0x004990B0`,
  `ceilf` `0x004566D0`, `lrintf` `0x0042D180`). Each is byte-identical to the project's helper compiled `/Od` and uses
  the same block of control-word globals. A plain CRT call, an intrinsic or a debug/release macro switch would leave
  something else in the debug image.
- **Functions the release linker discarded.** Unreferenced functions were removed from the release build by
  `/OPT:REF` but survive in the debug build. Bodies restored from it include eight `stdLinkList` functions (all
  identical to the debug code), the inventory "bin" functions of `sithPlayer`, `stdDisplay`'s `ClearRect` and
  `VBufferCopy`.
- **Debug-only console commands, under the original's debug switch.** Some functions are registered only by statements
  the release build does not have: the debug `sithCommand_RegisterCommands` registers 39 more console commands than
  retail, `JonesConsole_Open` 9 more and the cog verb table 5 more (`DebugPrint`, `DebugInt`, `DebugWaitForKey`, ...).
  Those registrations and the handlers only they reach are written under `#ifdef J3D_DEBUG` (the switch `j3d.h`
  defines when `NDEBUG` is not), so the release objects do not contain them and the `/Od` comparison, which compiles
  without `NDEBUG`, does. The three registering functions are identical to the debug code with them. The handlers'
  prototypes stay unconditional: `sithCommand.c`'s release code needs their declaration count.
- **Debug-only statements inside retail functions, under the same switch.** The debug build prints a message when an
  invulnerable jeep would have died (in the physics, collision and attach code) and when a raft goes under water,
  places marker things at every AI move target, registers and binds eight more controls (some only in editor mode),
  and its cog `print*` verbs actually print. Each of these statements is written in place under `#ifdef J3D_DEBUG`,
  together with the functions only such code calls (the console's level jump and restart, the sound dump, the PVS
  toggle). The sound library also has its own millisecond timer, which the release linker merged with an identical
  function of the platform layer; it is restored, and the release image still calls the merged address.
- **Debug-only functions and a debug-only file.** The debug build has whole functions retail never compiled: the
  world writer `sithWrite.c` (eleven functions that write a loaded level back out as text) with the sector and template
  writers it calls, a hash-table statistics dump, a multiplayer end-of-level hook and the lobby state it reads, and
  AudioLib's WAV/iMUS/AIFF header writer with its compression helpers. A function whose constants or static data have
  no place in the retail image was not in the release build; such functions are written under `#ifdef J3D_DEBUG`, and
  `sithWrite.c`, of which retail has nothing, is a source file outside the release link. Each is identical to the debug
  code under `/Od`. Where retail does keep an orphaned constant of such code (AudioLib's three AIFF sample rates), the
  constant is defined unconditionally, which also replaces filler declarations the release code had needed.
- **Locals and types.** Frame sizes show the number of locals, and compares show signedness: a `jb` means the original
  type was unsigned. Header prototypes and constants were corrected this way, for example `SITHSOUNDCLASS_MAXMODES`
  is a signed `141`.
- **Coding conventions.** Where two spellings give identical bytes in both builds, the conventions the debug build
  shows are used as a tie-breaker. They are collected in [HOUSE-STYLE.md](HOUSE-STYLE.md).

### What is only recorded

Debug-only text the switch above does not cover is recorded but not added to the release source: asserts already
compile to nothing in release, and constant `if ( 0 )`/`if ( 1 )` blocks have no release bytes to verify them
against. In the three files that changed after the debug build, the release bytes take precedence over the debug text.

## Building it

`ninja debug` compiles the game the way this executable was compiled: every source file of the Jones3D project with
its release flags but unoptimised (`/Od`, no inline expansion) and without `NDEBUG`, so the asserts are live and the
`J3D_DEBUG` code above is compiled in, together with the debug-only file `sithWrite.c`. The separately built libraries
(INSANE, zlib, the Intel-compiled renderer and maths helpers, `stdUtil_StrCmp`, the assembler objects) keep their
release flags, because the debug executable links them optimised too. Each file uses its usual compiler under wibo;
the objects go to `build/indy3d-v1.2-debug/obj/` (`ninja v1.0-debug`: the 1.0 source, `build/indy3d-v1.0-debug/`).
`ninja` never builds it.

`ninja debug-link` links those objects into `build/indy3d-v1.2-debug/Indy3D_debug.exe` (`ninja v1.0-debug-link` for
1.0) with the period linker and resource compiler of the Visual C++ 5.0 SP3 package, which need
[Wine](https://www.winehq.org/), and the release C runtime `libc.lib` (the objects do not use the debug runtime). The
icons come from your own executable, as in the release build.

The result is **not byte-matched**: nothing compares it with the Italian debug executable, whose layout, link order
and C runtime differ. It is the game built with the debug code switched on, not a reconstruction of that file; it has not been played here. With
no display server, it runs under Wine to the same place as the retail executable: the window creation fails and the
process exits.
