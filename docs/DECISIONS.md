# The evidence behind the source text

Byte identity proves that a function compiles to retail's code. It does not prove that its text is the text the
original programmers wrote: an optimising compiler maps many texts onto the same bytes. This page lists, by class,
what justifies the source text, and every class of choice that rests on measurement rather than on direct evidence:
the rule that admits it, the evidence it needs, the label to search for in the source, how many functions rely on it,
and a few notable cases. The labels in the source are the per-site record, and [DECISIONS-LIST.md](DECISIONS-LIST.md) lists every function in tiers 2 and 3 with its class and the choice made.

Figures are for the current build: all 3,300 game functions are byte-identical to retail after relocation, in each
of the two versions built ([VERSIONS.md](VERSIONS.md)). Intel's maths library is not counted
([section 10](#10-library-code-taken-from-the-original)). The tiers and the list describe 1.0's text
([below](#which-version-the-lists-describe)).

## The three tiers

Every exact function is counted in exactly one tier, by the weakest evidence it needs:

| Tier | Meaning | Functions |
| --- | --- | ---: |
| 1 | exact with evidenced text only | 3,102 |
| 2 | exact through a release-chosen spelling that the debug build cannot see (sections 3-7) | 70 |
| 3 | exact only through a labelled declaration-count fit (section 2); may also use tier-2 choices | 128 |

The tiers describe how a function became exact; every tier-2 and tier-3 function is listed in [DECISIONS-LIST.md](DECISIONS-LIST.md). They are not a target: no tier is preferred when choosing between
two spellings, and evidence always outranks the tier a function lands in.

### Which version the lists describe

The tier counts and [DECISIONS-LIST.md](DECISIONS-LIST.md) are measured on **1.0's text**: the shared tree with 1.0's
copies from `versions/1.0/` in place of the files 1.2 changed. Every file 1.2 did not change is the same text in both
versions, with the same choices. Two of the changed files carry fits of their own in 1.2, made for 1.2's declaration
count (section 2):

- `jonesConfig.c`: the `LoadControlScheme` count-fit block grows by about 400 declarations, the `AddStoreCartItem`
  block shrinks by 8, the `ClearStoreCart` block is removed, and two declaration orders differ (section 6):
  `SetTextControl` declares `rectDlg` last in its block, `SetDialogTitleAndPosition` declares `rectClient` before
  `rectWindow`.
- `sithPlayerControls.c`: five count-fit blocks are re-sized for 1.2 and one more is added, before `CanStrafeMove`.

What these choices close in 1.2 is the code that changed with no source change of its own: in `jonesConfig.c`
`SetTextControl`'s and `SetDialogTitleAndPosition`'s code, `AddStoreCartItem`'s message order and eight other
functions; in `sithPlayerControls.c` `CanAimAtPos`, `CanStrafeMove`, `CheckWaterLedge`, `ProcessJumpForwardMove`,
`ProcessSwimMove` and `TryBoardVehicle`. A per-function tier list of 1.2's text has not been made: it would need the
count-fit dependence of every function re-measured against the 1.2 executable.

**Before any fit is applied** (sections 2-7), the project first looks for a better, more natural or evidenced form and
records what was tried. A fit is the last step for a function, not the first.

## 1. Evidence that justifies source text (tier 1)

How release bytes rank against each other (immediates at call sites, frame size, instruction counts) is in
[METHODOLOGY.md](METHODOLOGY.md#how-evidence-is-ranked). On top of the release bytes, the source text rests on these
kinds of evidence, strongest first:

- **The debug build.** The Italian release ships an unoptimised build of the same source
  ([DEBUG-BUILD.md](DEBUG-BUILD.md)). Every function is compiled the way it was (`/Od /Ob0`) and compared with it: of
  2,905 paired game functions, 2,658 are identical and 189 identical apart from the order of stack slots (which follows
  a hash of each local's name); 58 differ, each recorded. The debug text shows statement order, control flow, the
  number and types of locals, macros against calls, and shared error blocks. In the three files that changed after it
  (`JonesMain.c`, `sithCamera.c`, `sithSector.c`) retail wins.
- **Text preserved in retail.** Assertion expressions (`SITH_ASSERTREL(bStartup)` names its variable), diagnostic
  messages, `__FILE__` paths with their exact case, and `__LINE__` values: allocation and assertion calls push their
  line numbers as immediates, and the source keeps them with `#line` directives. Generator version strings
  (`@(#)yaccpar 1.4`, flex's skeleton tables) identify generated code.
- **Lineage.** Published code that retail demonstrably contains: zlib 1.0.9 (the unmodified source gives 61 of its 95
  functions byte-identical; `gzio.c` is the one modified file), the byacc and flex skeletons, 4.4BSD libm's `rint`. The
  related Jedi Knight engine and the projects this one descends from (OpenJones3D, OpenJKDF2) supply names, structure
  layouts and spellings, checked against the retail and debug bytes.
- **House style.** Conventions that recur in the debug build ([HOUSE-STYLE.md](HOUSE-STYLE.md)), for example dialog
  procedures listing `WM_COMMAND` first (15 of 17 in `jonesConfig.c`). A convention only breaks a tie between two
  spellings that give the same bytes in both builds; it never overrides the function's own evidence.

Removing a construct that never had evidence is itself a correction: a parenthesis, cast, temporary or label kept only
because it improved a score is taken out, even when the function then stops matching.

## 2. Declaration-count fits (tier 3)

**What they are.** Visual C++ 5.0's front end numbers every identifier it declares, in source order, and the back end
uses those numbers as tie-break keys, for example which of two commutative operands is loaded first
([COMPILER-INTERNALS.md](COMPILER-INTERNALS.md#6-the-number-of-declarations-before-a-function-changes-its-code)). So
the code of some functions depends on how many declarations precede them, modulo a period (512 in most cases
measured, 2,048 in the longest). Most of those declarations come from headers, and the original headers do not
survive: each one was rebuilt from what the code uses, so its count differs from the original by an unknown amount.

**What a block is.** Placeholder declarations inserted where the missing ones would have been (after the `#include`s,
between two functions, or rarely inside a function), sized so that the count lands in the window where retail's code
appears. They stand for declarations the original had and the evidence cannot name. A block occupies lines that
were already blank (so no `__LINE__` moves) and is written with macros from `Libs/j3dcore/types.h` that add no entry
of their own:

```c
/* count-fit: placeholder declarations, not reconstructed source */
CF_PROTO(AudioLib_countfit_1, 14)
CF_PROTO(AudioLib_countfit_2, 0)
/* end count-fit */
```

`CF_PROTO(name, n)` is a prototype with `n` parameters (`1 + n` entries), `CF_PROTOS(stem, n, k)` repeats it `k`
times, and `CF_EXTERNS(stem, k)` declares `k` block-scope `extern`s.

**Rule.**

- Used only when every remaining difference of the function depends on the count: it is identical at some count and
  moves with the count. A function that is identical at no count has a different problem, and fitting is refused.
- Any size is allowed, provided the block is labelled and removable in one edit. Inside a fitted function the order of
  its local declarations may be searched together with the block size.
- A count that falls inside a window is never evidence for a header's text or a function's body. It measures only how
  many declarations are missing.
- Whenever a shared header changes, every fitted file that includes it is measured and fitted again.

**Removal.** Replace the block with as many empty lines. The function then differs from retail by its count-dependent bytes.

**Extent.** 99 blocks in 40 files, about 11,800 entries in all *(1.0's text)*. Half the blocks have 26 entries or
fewer; 28 have 100 or more. 128 functions are exact only with their file's blocks in place.

**Search for:** `count-fit`. **Functions:** the class `count-fit block` in [DECISIONS-LIST.md](DECISIONS-LIST.md).

**Notable cases.**

- `jonesConfig_LoadControlScheme`: the largest block, 3,086 entries (`CF_PROTOS(jonesConfig_countfit_30, 30, 99)` plus
  one 16-parameter prototype) on the line before the function, in a file whose 120 debug-paired functions all compile
  to the debug build's code.
- `sithCogFunctionThing.c`: `IsGhostVisible`, `SetThingHeadOrientation` and `GetThingJointPos` were 6, 6 and 1 bytes
  from retail and became exact at counts 8, 9, 22, 24, 25, 38, ... (mod 512), and at no count outside those windows.
- `sithThing.c` and `sithPlayerControls.c` once called `sithMaterial_Load` and `stdUtil_StrCmp` without a prototype.
  Including the real headers costs exact functions, and the blocks were fitted again around them: an implicit
  declaration that carries bytes is replaced by the header, never kept as a hidden fit.

## 3. Spelling-fits (tier 2)

**What they are.** Visual C++ 5.0 keeps a parenthesised float subexpression, and the `D3DVAL` conversion macro, as a
tree node of its own that blocks some algebraic rewrites and moves the scheduler's input
([COMPILER-INTERNALS.md](COMPILER-INTERNALS.md#7-parentheses-are-code)). The node has no bytes and is invisible under
`/Od`, so the debug build cannot say whether the original had it. A single site can therefore be the only text that
makes a release function exact while identical lines elsewhere take the other form.

**Rule.** Allowed only at a single site of a function whose other residue is zero; chosen by measurement (the function
becomes exact, nothing else moves, the debug-build comparison is unchanged); the line carries the trailing tag, one
per site, so the fit is removable in one edit. Untagged lines that are identical across sibling functions are written
alike.

**Extent.** 32 tagged lines in 10 files; 14 exact functions use one (7 in tier 2, 7 also need a count-fit).

**Search for:** `/* spelling-fit */`. **Functions:** the class `spelling-fit` in [DECISIONS-LIST.md](DECISIONS-LIST.md).

**Notable cases.** `rdClip_Face3TOrtho`: the near-plane block's `.z` row wrapped in `D3DVAL(..)`. `sithCamera_Update`:
one focus test written as a plain product where the file otherwise uses `RD_MULTIPLY`. `rdWallpaper_Draw`:
`sheight = D3DVAL(D3DVAL((float)height) / RD_REF_HEIGHT);`.

## 4. Conversion macros, parentheses and operand order (tier 2)

**Rule.** Where `/Od` cannot see the difference, these may be chosen by release bytes:

- the file's own conversion and arithmetic macros (`D3DVAL`, `RD_MULTIPLY`, `D3DMultiply`, `D3DDivide`), applied
  per family: lines that are identical across sibling functions take one form;
- `return (x);` and `(-x)`, as ordinary habits of the period. Bare grouping parentheses inside products or sums are
  not chosen this way;
- the operand order of a comparison (constant first or second).

These are not tagged in the source: a file-wide macro habit is ordinary text, and there is no single site to tag. Every function that relies on one is listed in [DECISIONS-LIST.md](DECISIONS-LIST.md) (classes `macro or parenthesis spelling`, `operand order`, `initialiser and comparison spelling`).

**Extent.** 45 exact functions use a macro or parenthesis choice (33 in tier 2), 1 an operand order and 1 an
initialiser and comparison spelling.

**Notable cases.** `sithPuppet_SynchMode`: the debug text's `curFrame / numFrames` costs 203 release bytes; with
`D3DVAL(curFrame / numFrames)` the function is exact and still identical under `/Od`. `rdFont`: nine of its ten
screen-scale lines are written uniformly as `D3DVAL((float)w / RD_REF_*)`; the tenth is used differently and stays
plain. `JonesMain_PlayIntroMovie`: a constant-first comparison.

## 5. Header-level forms and types (tier 2)

The functions that rely on these are listed in [DECISIONS-LIST.md](DECISIONS-LIST.md) (classes `header convention`, `release-chosen type`).

Choices made once, in a header or for a whole family of declarations, judged on the whole project:

- **Vector component macros.** Every component macro in `rdVector.h` (and two in `rdMath.h`) reads and writes
  components as `(&(v)->x)[0]`, `[1]`, `[2]` rather than `(v)->x/y/z`. `/Od` code is identical either way (59 files
  checked). Retail support: in `sithAIUtil_MakeRandPoint` the index form removes a member-select record that VC5
  otherwise reuses for `memcpy`'s block move, the only such reuse in the image. Project-wide it makes
  `sithPlayerActions_ActorGrabLedge` exact; the three functions it cost were made exact again by their own fits. **Search for:** the comment at the top of `Libs/rdroid/Math/rdVector.h` ("release-chosen header form").
- **Vertex-index arrays are `size_t*`.** All five of them: retail's bytes need the unsigned type, the change is
  `/Od`-neutral in 672 functions, and no signed compare anywhere witnesses `int`. A related release-chosen split in
  `sithPhysics_ProcessTrackFace` (three of four corner arrays `size_t`) is labelled `release-chosen type`.

## 6. Declaration order, prototype style and include order (tier 2)

Every function in these three classes is listed in [DECISIONS-LIST.md](DECISIONS-LIST.md) (classes `declaration order`, `prototype style`, `include order`).

None of these leave a trace in either binary: local slots in the debug build follow a hash of the name, not the order
of declaration; prototype parameter names compile to nothing; an `#include` leaves no bytes of its own.

- **Order of local declarations.** Chosen by release bytes at the file's real declaration count, because the
  declaration order sets the identifier numbers that break operand ties. Searched again when a shared header changes.
  Not marked in the source (any order is equally plausible text). 54 exact functions rely on one (25 in tier 2).
  Example: in `sithPhysics_GetWaterNormalAtPos`, exactly the 35 of 210 orders of three declarations that satisfy the
  ordinal chain `startPos < flt_538CD0 < moveDist` give retail's bytes (99 -> 0 differing bytes).
- **Unnamed parameters in forward prototypes.** A free authoring choice, picked by measurement and applied to a whole
  file, never name by name. `sithCogFunctionThing.c`'s 218 cog-verb prototypes are written `(SithCog*)`; this replaced
  975 count-fit entries with 3. `sithSoundMixer.c` likewise. 3 exact functions. **Search for:** `release-chosen
  prototype style`.
- **Include order.** Only for headers the file genuinely needs: `JonesMain.c` includes `<stdio.h>`, `<stdlib.h>` and
  `<string.h>` (it calls their functions) before its own header, which makes `JonesMain_HandleWMGetMinMaxInfo` exact
  and replaced a 257-entry count-fit block. This is the one place a declaration count was allowed to choose header
  text. 1 function. **Search for:** the comment at the top of `JonesMain.c`.

## 7. Release-chosen locals (tier 2)

**Rule.** A local that leaves no trace in either build (a pointer to a member, a cached value) may be added when it is
the only text found that gives retail's code, provided no debug code contradicts it: in game files the debug frame
must have room for the slot. It is labelled at its declaration and removable in one edit. In practice it is used in
the movie player, which has no debug build ([INSANE.md](INSANE.md)).

**Extent.** 3 exact functions. **Search for:** `release-chosen local`. **Functions:** the class `release-chosen local` in [DECISIONS-LIST.md](DECISIONS-LIST.md).

**Notable cases.** `decode_Begin`: `BitmapFormat* pFormat = &pBitmap->format;` gives the colour-mode cases retail's term
order. `dbloc16_DecodeFrame`: a local state aggregate whose members take retail's frame slots.

## 8. Names

Names survive only where retail spells them: assertion texts, diagnostic strings, `__FILE__` paths, the cog verb table.
Everything else comes from the earlier reverse-engineering projects (OpenJones3D, OpenJKDF2, Jones3DRE) or is
invented, and no name claims to be the original unless a string shows it. Jones3DRE is a separate reverse-engineering
project by the OpenJones3D maintainer; source comments that say "from Jones3DRE" take a name or a layout fact from it,
checked against the retail and debug bytes, and none of its code is used.

A name changes the bytes in two places, and only there is a name chosen to fit:

- **File-static BSS order.** VC5 lays out uninitialised file statics by a hash of the identifier's spelling, not by
  declaration order. Where no string pins a static, its name is chosen so that the hash gives retail's order (the
  movie player's statics are named this way).
- **Debug-build slot order.** Where the debug frame shows a local in a slot only some spellings reach, the name is
  chosen to fit it: a lineage name if one fits, otherwise a natural name for the same role. Example:
  `sithPhysics_GetWaterThrust`'s `pSurfSector`.

## 9. The listed exceptions: `goto`, inline assembly, `volatile`

These constructs are otherwise not allowed ([METHODOLOGY.md](METHODOLOGY.md#what-is-not-allowed)). Each site below is
named, limited to its functions and enforced by a test; a new site needs evidence of its own. None of them is a fit:
they count in tier 1.

**`goto`** (about 1,500 statements in 64 files):

- *Generated skeletons.* `sithCogYacc.c` and `sithCogFlex.c` are Berkeley yacc 1.4 and flex 2.3.6 output, generated
  from `cog.y` and `cog.l`; retail carries the `yaccpar` version string and flex's tables.
- *The text loaders' shared error blocks.* Ten named loaders (`rdModel3_LoadEntry`, `sithWorld_LoadEntryText`, ...)
  end with `return <ok>;` and one print block per failure kind after it, which every failing read enters. Retail's
  print order showed the blocks; the debug build shows the jumps. `std3D_AddToTextureCache` compiles byte-exact with
  its single label, against +32 and +336 bytes for the two goto-free forms.
- *Jumps only a `goto` produces.* The debug build, which neither threads nor merges tails, shows a jump no structured
  form produces: a block reached after the function's final `return`, or a jump out of a loop nest past the code that
  follows. Each site records its debug addresses and is checked identical under `/Od`. The cog verb registration
  bodies (`if ( bFailed ) goto error;` once per verb, six bodies in the debug image) are most of these.

**Inline assembly**, where retail uses instructions no compiler of the period emits from C:

- the `stdMath.h` rounding helpers (`sqrtf`, `floorf`, `ceilf`, `lrintf`) and `std3D`'s float-to-int helper, whose
  out-of-line copies the debug build contains (`sqrtf` at `0x0043FE70`, the `std3D` helper at `0x004FBEC0`);
- the FPU control-word block in `stdStartup` (`0x004B633E-0x004B6365`) and its restore in `stdShutdown`;
- one decoder loop in `AudioLib_UncompressBlock`: `cmp r32,0`, `mov r32,0`, `lea r32,[abs32]`, `push ecx`/`pop ecx`
  around the loop body, each the image's only instance; with the block the function is 672/672 bytes;
- the three CPU probes of the SSE renderer: `cpuid` inside `pushad`/`popad` within an exception handler (no compiler of
  the period has a `cpuid` intrinsic or emits `pushad`);
- `stdDisplay_SetPixels16/32`: `cld; rep stos`, and `cld` is an instruction no compiler emits.

**`volatile`**, on five declarations of the movie player. Four are state its time-critical timer thread shares with the
main thread: `timer_Alloc` loads its parameter only after the `busy` store, `sound_Open` clears the script variables
with a byte-store loop where VC6 otherwise emits `rep stosd`, and a track's `state` is reloaded at every read where VC6
otherwise keeps it in a register. The fifth is `framestream_Tick`'s `a9` parameter: retail's scheduler treats its load
as a barrier at all three `Present` calls, and no non-volatile form tried (address-taken uses, `const`, other types, a
one-member struct) produces that code.

## 10. Library code taken from the original

Apart from the icons, the build copies 2,627 bytes (1.2; 2,643 in 1.0) from the retail executable instead of building
them from source. Almost all of it is library code that retail linked in from Microsoft's and Intel's prebuilt
libraries rather than compiling. None of it can be taken from the libraries we have, and claiming it from source would
need text with no evidence behind it *(an invented function or a guessed address)*, or, for Intel's `floor` and
`modf`, a source form not yet found. The ranges
are listed in [BUILD-PIPELINE.md](BUILD-PIPELINE.md#what-is-still-copied-from-the-retail-executable).

| Piece | Bytes (1.2) | What claiming it would need |
| --- | ---: | --- |
| Intel libm's `floor.c` and `modf.c` | 571 | a source form against the evidence of how Intel wrote them |
| C runtime `aw_cmp`, `aw_map`, `aw_str` | 1,648 | an invented game function that uses an empty wide string |
| C runtime `longjmp` | 124 | a guessed address for a variable nothing references |
| Intel libm's `support.obu` statics | 160 | five full function bodies the linker discarded, and it would still not match |
| Unused bytes | 124 | nothing: they are never read, and no file can be shown to have emitted them |

### The two libm files that do not match

`floor.c` (`floor`, `ceil`, and `rint`, whose code the linker dropped) and `modf.c` reconstruct routines of Intel's
maths library, compiled with Intel C/C++ 4.0 under the build line recorded in Intel's own library. The code of `floor`
and `ceil` is byte-identical; what differs is the order of five 8-byte constants in `.data1`, and in `modf` the same
plus one register choice (61 of 63 instructions align). Retail has the constants in the reverse of the order the
compiler numbers them, and the same reversal appears in other members of retail's libm (`scalb`, `logb`).

Forms that do byte-match exist: separately named constant objects, one per use site, and an artificial NaN guard in
`modf`. They are not taken, because no member of Intel's libm is written that way and the matching revision of Intel's
own `floor` object stores the constants as literals. The source form Intel's build had is not yet found. Details:
[COMPILERS.md](COMPILERS.md#the-libm-helpers-that-do-not-byte-match).

### The C runtime's wide-string helpers

`aw_cmp`, `aw_map` and `aw_str`, from Visual C++ 5.0's `libc.lib`, share one empty wide-string literal (`L""`).
Retail pooled it into a 4-byte slot of uninitialised data at `0x0056B6F8`, between the blocks of `sithConsole.c` and
`sithWeapon.c`. The three members cannot be placed until some file owns that slot, and the file that owns it must be a
game file that used `L""`. No game source contains one and no retail code refers to the slot except the three members,
so the reference must have been in a function the linker discarded. Claiming it would mean adding such a function to
`sithConsole.c` with nothing to show what it was. The members stay original.

### `longjmp`

`longjmp` (called from INSANE's `Timer.c`) is byte-identical in `libc.lib`, but the member also defines a 4-byte
variable, `__setjmpexused`, that nothing references. The linker placed it among the C runtime's other uninitialised
variables, and four zero-filled gaps there could hold it. Nothing tells them apart, so placing the member would mean
guessing. It stays original.

### Intel libm's `support.obu`

The libm member `support.obu` contributes 80 bytes of initialised data and 80 bytes of `.data1` constants to retail;
its five functions (`signal_inf`, `scalb`, `copysign`, `logb`, `finite`) were all discarded by the linker. Intel's
own member has the same data but its constants in a different order (the same ordering question as `floor` and
`modf`), so it cannot be used as it is. A reconstructed `support.c` would need all five function bodies, none of which
retail contains, only to emit those constants, and would still not match. It stays original.

### Unused bytes

The remaining 124 bytes (1.2) are never read by anything in the executable: no code or data refers to them, except
one address used only as the end of a loop over the table before it. They are:

- **58 bytes of zeros in seven gaps between files**, six of them among the C runtime's members and one between the C
  runtime's start-up data and `jonesCog.c`. No alignment rule explains them.
- **66 bytes of constants between game files**, probably left by functions the linker discarded:

| Bytes (1.2) | Value | Between |
| ---: | --- | --- |
| 14 | the float `0.001`, with zeros | `sithCogFlex.c` and `stdControl.c` |
| 4 | the float `100.0` (its address ends the loop over `stdHashtbl.c`'s prime table) | `stdHashtbl.c` and `stdGuid.c` |
| 16 | the GUID `{87051A80-13FC-11D1-97C0-00A024293005}` | `rdroid.c` and `rdCanvas.c` |
| 4 | `-1` | `sithPhysics.c` and `sithActor.c` |
| 28 | the ints `0, 2, 1, 0, 2, 1, 0` | `rdPuppet.c` and `rdKeyframe.c` |

Each could be given to a neighbouring file only by inventing the code that used it.
