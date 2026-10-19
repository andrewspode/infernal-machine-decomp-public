# House style: the original programmers' conventions

The unoptimised debug build of `Indy3D.exe` ([DEBUG-BUILD.md](DEBUG-BUILD.md)) shows how the original programmers
wrote their code in many places the optimised release build hides. This document collects the conventions that recur.

**How they are used.** A convention never overrides evidence about the function itself:

1. The function's own debug code, its release bytes, or a text clue always come first.
2. A convention decides only between two spellings that give the same bytes in both builds.
3. A convention with counter-examples is a default, not a law.
4. A convention never justifies a change that costs release bytes, and never justifies a `goto`, padding or any other
   forcing trick.
5. **Nothing here is about names.** Local, label and helper names do not survive in either binary; where the source's
   names come from is in [DECISIONS.md](DECISIONS.md#8-names).

**Strength:**

| Label | Meaning |
| --- | --- |
| strong | ten or more independent witnesses and no unexplained counter-example |
| solid | a clear majority, with the counter-examples named |
| weak | two to four witnesses, or a split |

**Build** says which image shows a convention: *debug* (only the unoptimised build can tell), *both*, or *retail*
(read from the release bytes).

Addresses in the witness lists are in the **debug** executable unless they say otherwise.

What the debug build can and cannot show is set out in
[DEBUG-BUILD.md](DEBUG-BUILD.md#what-it-can-and-cannot-prove); sections 1 and 2 below are of the visible kind, section 3 is read from retail.

## 1. Control flow and error handling

### Loaders end with shared error blocks after the success return

A reader, loader or allocator with several failure points ends `return <ok>;`. Labelled blocks follow it: one print per
failure kind, then a shared `Close/Free; return <fail>;` tail. Every failing read jumps into them. The blocks follow the
success return in the order end-of-file, syntax, allocation, range, then the shared tail.

- **Strength:** solid. `sithSector_ReadSectorsListText` (`0x00478330`) has per-kind blocks after the success return,
  two of which end in a jump to one shared `return 1`.
- **Witnesses:**
  - `rdParticle_LoadEntry` (`0x005293C8`): open, end-of-file, syntax, allocation and range blocks each jump to one
    shared `Close; return 0`.
  - `sithWorld_LoadEntryBinary` (`0x004337F7`): `return 0` at `0x00433ABF`, an error block at `0x00433AC3` entered
    from six sites, one of them inside the section loop.
  - `sithSurface_ReadSurfacesListText` (`0x00476F7C`).
  - `JonesDisplay_Startup` (`0x00415B30`): ten failures lead to one `Shutdown(); return 1;` after the success tail.
- **Build:** both. Retail's print order showed the shared blocks first; the debug build proves the jumps.

### Cog verb registration: one result variable and one jump per verb

A `RegisterFunctions` body declares `int bFailed;` and, for every verb, writes
`bFailed = sithCog_RegisterFunction(...); if ( bFailed ) goto error;`, ending with `return 0; error: return 1;`.

- **Strength:** strong for the cog modules: six bodies in the debug image share the shape.
- **Witnesses:** `sithCogFunctionThing_RegisterFunctions` (`0x00486630`, 235 registrations), `sithCogFunctionAI`
  (`0x00482D10`), `sithCogFunctionSector` (`0x0047F310`), `sithCogFunctionSound` (`0x004804B0`),
  `sithCogFunctionPlayer` (`0x0047DA20`), `sithCogFunction` (`0x00493D80`).
- **Build:** debug only. Retail's bytes are produced equally by a `bFailed = bFailed || Register(...)` chain.
- **In this source:** every such body takes the `goto error;` form the debug build shows (a deep nest of empty `if`
  arms, which compiles identically, is not plausible written source).

### Early-return guards by default

`if ( !p ) return;` or `if ( bad ) return 0;` sits at the top, with the body at function level. A wrapped body appears
only where the debug code shows the branch landing on a shared final return.

- **Strength:** solid. The debug code showed an early return about 29 times where an earlier text had a wrapped body, and the
  reverse 3 times.
  The choice is per function, not per module.
- **Witnesses for guards:**
  - `sithConsole_PrintString`, `ExeCommand` and `RegisterCommand`;
  - `stdControl_Shutdown` and `Close`;
  - `Driver_SetPosAndVelocity`;
  - `sithFX_DestroyFairyDustDeluxe` (`0x004B15BE`);
  - `sithAIInstinct_SenseDanger` (`0x004B8DC9`).
- **Counter-witnesses (wrapped):** `sithItem_PlayerCollisionHandler`, `sithVoice_PlayThingVoice`,
  `stdUtil_StringSplit`.
- **Build:** debug only. Retail compiles both forms to the same bytes.

### Cog verbs test the good case first

The sector and surface cog verbs write `if ( pSector && flags ) { … } else { print }`, with no `return` in the `else`.
Loaders and `Sound_*` functions put errors first.

- **Strength:** weak: four functions in one file.
- **Witnesses:** `sithCogFunctionSector`'s `SetSectorSurfflags` (`0x0047FB5D`), `ClearSectorSurfflags`,
  `SetSectorFlags` and `ClearSectorFlags`.
- **Build:** debug only.

### Loops

- **Collision walks** are `while ( (pCollision = sithCollision_PopStack()) != NULL )`. This is strong: 68 loops, and
  one rotated exception in `sithExplosion_MakeBlast` (`0x004DDAB2`). Witnesses: `sithAIUtil_CheckPathToPoint`
  (`0x004CBC55`) and `sithPlayerControls_ProcessJewelFlyMove` (`0x0043A2F6`).
- **Count-down scans in `Sound.c`** decrement in the condition, `while ( (pChannel--, i-- != 0) )`, starting from
  `&Sound_apChannels[i]` (16 sites). Witnesses: `Sound_GetChannelHandle` (`0x0052D9C2`) and `Sound_GetSoundHandle`
  (`0x0052D8C6`).
- **Signed literal bounds where the compare is signed.** `STD_ARRAYLEN` is unsigned (`jae`). Where the debug compare is
  `jge`, the bound was a literal or a signed constant: `sithFX`'s `i < 4`, and `SITHSOUNDCLASS_MAXMODES` is `141`, not
  `141u`.

### Dialog procedures list `WM_COMMAND` first

`switch ( uMsg )` lists `WM_COMMAND` first, then `WM_INITDIALOG`, then other messages, and `WM_DESTROY` last.

- **Strength:** strong in `jonesConfig.c`: 15 of 17 procedures. The exceptions are `DialogInsertCDProc` and
  `AssignKeyDlgProc`, and outside the file `JonesDialog_SubclassDialogProc` and `JonesDevDialog_DevDialogProc`.
- **Witnesses:** `jonesConfig_MessageDialogProc` (`0x00402B54`), `ExitGameDlgProc` (`0x00406B8F`),
  `GamePlayOptionsProc` (`0x00407EFC`), `CreateControlSchemeProc` (`0x0040A744`), `StoreDialogProc` (`0x00410FEA`).
- **Build:** debug only. The release compiler turns a `switch` into a sorted compare tree, so case order is invisible
  there. This is the cleanest tie-breaker in this document.

### Dialog initialisers take the raw `LPARAM`

`int Init…(HWND hDlg, int a2, LPARAM lParam)` receives the dialog's `WM_INITDIALOG` `lParam` unchanged. A typed pointer
is a local copy made inside.

- **Strength:** strong: 14 initialisers. The exception is `InitDisplaySettingsDialog`.
- **Witnesses:** `InitStoreDialog` (`0x004115A3`), `InitMessageDialogData` (`0x00402D51`), `InitStatisticDialog`
  (`0x00410867`).

### Constant conditions

The debug code contains `mov r,1; test r,r; je` sequences, which are `if ( 1 )` blocks, in about eight places:
`sithRender_BuildSectorThingList` (`0x00455297`), `sithPuppet_StopForceMove` (`0x0046F60D`),
`sithPlayerControls_ProcessGeneralMove` (`0x00438ADE`), the external-camera code of `sithCamera`, and others. They
are probably a debug switch defined as 1. They are recorded and not recreated.

## 2. Locals, copies and helpers

### Shared function-scope locals; block scope only where the frame shows it

One `result`, `idx` or `taskNum` is reused across arms and loops. A block-scoped declaration is right only in two
cases: where the debug frame groups the slot after the function-scope slots, or where the release build needs an
address-taken local out of scope at an earlier call (see
[COMPILER-INTERNALS.md](COMPILER-INTERNALS.md#1-a-pointer-returned-by-a-call-may-point-at-any-address-taken-local-in-scope)).

- **Strength:** solid.
- **Function-scope witnesses:**
  - `JonesMain_ProcessStartGame` and `ProcessGame`: one `int result` holds both results;
  - `sithEvent_Process`: one `taskNum` for both loops;
  - the `sithDSS` receivers: one shared `idx`.
- **Block-scope witnesses:** `sithPlayerControls_CheckWaterLedge` (`0x0043D710`), `JonesHud_DrawCredits`,
  `sithWeapon_GetAimOrient`.

### Extra locals and dead reads the authors left

The authors cached element pointers and values in named locals that optimised builds fold away:

- `pChannel = &Sound_apChannels[i]`;
- `pCamera = &sithCamera_aCameras[i]`, `pItems`, `pActorInfo`;
- `hr`/`result` locals holding a call result.

They also left unused locals and discarded call results, for example a dead read of `pStatistics` in
`DrawStatisticDialogIQPoints` (`0x004104C1`), an unused `SithWorld* pWorld` in `jonesCog_EndLevel`, and `bMoved = 0`
in `sithAIInstinct_Roam`.

- **Strength:** strong for the pattern; each instance is a single witness. Names are inferred.
- **Build:** debug only: release emits nothing for them. Such dead code is kept: the debug build of the same source shows it,
  and it forces nothing in release (the rule against dummy operations targets code added only to force a match).

### Vector copies: `rdVector_Copy3` for any pointer, global or array side; `a = b` between locals

A copy where one side is a global, an array element or a pointer target is written with the copy macros, which the
debug build calls as `push 0Ch; …; call memcpy`. Local-to-local copies stay inline as `a = b`.

- **Strength:** strong. The tree has about 690 `rdVector_Copy3` sites; the debug image has 526 twelve-byte `memcpy` calls.
- **Why the macro and not raw `memcpy`:** the debug build carries unused 23-byte `rdVector_Copy2/3/4` bodies in
  `rdVector.c`, and the related Jedi Knight engine spells copies the same way.
- **Counter-examples:** `rdMatrix.c`'s own copies stay raw. `sithCamera_Update` is in a file that changed after the
  debug build.
- **Witnesses:** `sithAIAwareness_CreateEvent` (`0x004E0E9A`), and `sithCamera_Update` (`0x0044B096`: 20 of 26 vector
  copies).

### Macros, not inline helpers, for anything the debug code writes out

The debug build expands no `__inline` function. So a helper that appears **expanded** in the debug code was a macro,
and one that appears as a **call** was a function.

- **Macros:**
  - the `rdVector` arithmetic: `Set3`, `Neg3`, `Add3`, `Add3Acc`, `Sub3`, `Scale3`, `Scale3Acc`, `Cross3`, `Dot3`,
    `ScaleAdd3`, `Sub4`, `Lerp3`, `Dist3`, `Copy2/3/4`, `Len3`;
  - `rdMatrix_Copy34`, `STDMATH_CLAMP`, `SITH_RAND`, `sithAIInstinct_Rand`, `AUDIOLIB_SWAP16/32`.
- **Functions:**
  - the `stdMath.h` assembly helpers (`sqrtf` `0x0043FE70`, `floorf` `0x004990B0`, `ceilf` `0x004566D0`, `lrintf`
    `0x0042D180`);
  - nine `sithCollision` helpers (`0x004DAB38`..`0x004DB49F`, in source order);
  - `stdControl_UpdateKeyState` (`0x004EF100`, defined after `SetAxisFlags`);
  - `stdDisplay_SetPixels16/32` ("pick any" COMDATs, so declared `__inline` with external linkage).
- **Strength:** strong. Compiled the debug build's way, 1,434 functions are identical, and no function needs inlining.
- **Witnesses:** `sithCogFunction_VectorDist` (`0x00495EB5`) computes every difference twice and has no `dx` slot
  (`Dist3`). `rdCamera_NewEntry` (`0x00505929`) and `SetFOV` (`0x00505B92`) write the clamp out (`STDMATH_CLAMP`).
  The `0x004E0D8E` site reads the products straight from the operand (`Len3`).

### Clamps

A two-sided clamp is `STDMATH_CLAMP(v, lo, hi)`. A one-sided bound is `J3DMAX`/`J3DMIN`. A hand-written ternary or
`if`/`else` appears only where the debug frame lacks the macro's temporaries.

- **Strength:** strong: about 186 `STDMATH_CLAMP` uses.
- **Witnesses:** `jonesCog_UpdateDifficulty` (`0x00401568`), `rdCamera_NewEntry`, `rdMath_DeltaAngleNormalized`
  (`0x005129B6`).

### Float conversions

A parsed float is written as `D3DVAL(atof(...))` straight into its field; the cast leaves a conversion temporary in
the debug frame. Integer-to-float conversions in canvas arithmetic are `D3DVAL(int)`, as in
`D3DVAL(D3DVAL(left) + (D3DVAL(right - left + 1) / 2.0f))` (`rdCanvas_NewEntry`).

- **Witnesses:** `sithAIClass_LoadEntry` (`0x0046C397`), `sithSoundClass_LoadEntry` (`0x004B4D6B`, three conversion
  temporaries), `sithPhysics_ParseArg` (`0x0049F66D`).
- **Build:** both. File-level habits of `D3DMultiply`/`D3DDivide` were read from the retail bytes, not the debug code.

### Types follow what the code does with them

A parameter or constant is typed by the debug compare or push: `jb`/`ja`/`jae` means unsigned, `jl`/`jge` signed,
and an `int16` or enum argument pushed as a dword is a 32-bit type.

- **Witnesses:** `stdControl_EnableAxis(size_t axis)` (`0x004EE71C`, a `jb` compare), the signed
  `SITHSOUNDCLASS_MAXMODES`, and `sithDSSThing_Fire`'s 32-bit enum parameters (`0x0049AE11`).
- A cast is never added at one call site to work around a header constant's type.

### Return style

- `return x == y;` written as `if ( x != y ) return 0; return 1;` in `Sound.c` (weak).
- Single-result functions use one `result`/`hr` local and one final `return` (`JonesFile`).
- Parenthesised returns such as `return (expr);` and negations `(-x)` are invisible to the debug build
  ([DECISIONS.md](DECISIONS.md#4-conversion-macros-parentheses-and-operand-order-tier-2)).

## 3. Conventions that come from retail, not the debug build

Parentheses and the conversion macros do not show under `/Od`, so every such convention in the source (the value
parenthesis in the `sqrtf` helper, `((float)atof(…))`, `(x * K) + y` for constant-scaled products, per-file
`D3DVAL`/`D3DMultiply` habits) was derived from the release bytes, file by file. The debug build neither supports nor
refutes them; the rules for choosing them are in [DECISIONS.md](DECISIONS.md#4-conversion-macros-parentheses-and-operand-order-tier-2).

## 4. Not conventions

- Names of locals, labels and macros.
- A general `switch` case order outside dialog procedures, or a per-module rule for early return against wrapping, or
  for declaration order.
- Include order.
- Debug-only code (asserts, debug prints, debug verbs) is recorded, never added.
- Anything in the three files that changed between the debug build and the English release (`JonesMain.c`,
  `sithCamera.c`, `sithSector.c`). There the debug text is the earlier version, and retail wins.
