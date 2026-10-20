# Versions of Indy3D.exe

This project reconstructs two versions byte for byte: **1.2**, LucasArts' last official update (the default build),
and **1.0**, the executable on the original English CD. The other English builds are listed here so that you can tell
which one you have, together with the Italian release's debug build, the main source of evidence
([DEBUG-BUILD.md](DEBUG-BUILD.md)). Releases in other languages are not otherwise covered.

| Version | Where it comes from | Linked (UTC) | SHA-256 | Built here |
| --- | --- | --- | --- | --- |
| Italian debug | Italian CD, inside `Resource/CD1.GOB` as `indy3d.exe` | 1999-10-28 23:32:12 | `65f9165283b2e00a78baa93476073b3b302f5d561c240f2e0ae304fd98d028cb` | no; the evidence behind much of the source ([DEBUG-BUILD.md](DEBUG-BUILD.md)) |
| **1.0** | Original CD, `Resource/Indy3D.EXE` | 1999-10-29 19:09:40 | `3fbaf8cd401b4af80967cbe42e3420fb803288b336ebbe72a9a01b6dfd661a53` | `ninja v1.0` |
| 1.1 | LucasArts update `J3DUpdate1.1.exe` | 1999-11-22 19:04:41 | `5567574791fd91c14874e73bfd1fc1e02967d954b55ffe6075396074db60e2c8` | no |
| **1.2** | LucasArts update `J3DUpdate1.2.exe` | 2000-01-28 15:53:53 | `55fb00c0a2cb30793ff451dc6fd17681c5128aa32419aa711015fd5117d608bf` | `ninja` |
| Steam | Steam install | 2000-01-28 15:53:53 | `7faf42d593ab770c3bfbec14e0f1dbff738d3552d732e81561c78ac4054a75d4` | as 1.2, once unsigned ([below](#steam)) |
| GOG | GOG install | 2000-01-28 15:53:53 | `4075e655e0cf0db2d352265ba83a19a51fb373156cba8b3e43107fd6c6129ebb` | no ([below](#gog)) |

All of them are 1,539,072 bytes except Steam's (1,545,152) and the debug build (2,135,040).

## How the two versions are built

`Jones3D/` and `Libs/` are the 1.2 source, and `ninja` builds 1.2 from them with `config/indy3d-v1.2/`. 1.2 changed 14
files: 13 C files and the resource script `Jones3D/Indy3D.rc`. Each has a complete 1.0 copy at the same path under
`versions/1.0/` (`versions/1.0/Jones3D/Display/jonesConfig.c` for `Jones3D/Display/jonesConfig.c`), and `ninja v1.0`
builds 1.0 from the same tree with those copies in place of the shared files, and with `config/indy3d-v1.0/`. There is
no version conditional in the source: each version's file is that version's whole text. `versions/1.0/overlays.yml`
lists the copies and what 1.2 changed in each. A file name in a report, split or symbol table is always the shared path.
Every file is byte-exact in both versions except Intel's `floor.c` and `modf.c`, which are library code and not
counted ([DECISIONS.md](DECISIONS.md#10-library-code-taken-from-the-original)), and each build reads only its own
executable.

## The official updates

Each update is a self-extracting archive that installs a complete `Indy3D.exe`, not a binary patch, together with
level scripts *(the 1.2 update was found in an archive of LucasArts' old FTP site)*:

- **1.1** replaces `Indy3D.exe` and one Nub's Tomb script. Its readme lists fixes to the end-of-game statistics
  window, the trip to Peru after the Aetherium, the store window after the Aetherium and Infernal Machine levels, and
  Volodnikov and the Marduk statue in Nub's Tomb.
- **1.2** includes 1.1 and replaces `Indy3D.exe`, 51 level scripts and two installer files. Its readme lists fixes to
  the control configuration dialog, levels failing to open or opening with the wrong field of view, Infernal Machine
  part sounds that did not stop, and action and display problems in eight levels. Savegames made before the update
  are declared invalid.

### What changed in the executable

Compared function by function with 1.0 (addresses masked), 1.2 is a small set of source edits built with the same
compilers, flags and libraries, and linked in the same order. No function was added or removed:

- **13 functions with real code changes, all reconstructed:** five in `jonesConfig.c`, among them the key-name display rewritten around
  `GetKeyNameTextA`, bounds checks in the store cart and cursor handling in the insert-CD dialog; two more credit
  entries in `JonesHud_DrawCredits`; a special case for the Marduk statue in `sithPlayerControls_ProcessGeneralMove`;
  and one function each in `sithCamera`, `sithWeapon`, `sithCogParse`, `sithCogFunctionThing`, `sithActor` and
  `sithWhip`.
- **The rest follow from those edits:** shifted assert line numbers (21 functions), the build-date string (2), and
  register or ordering differences in the edited files (16, among them `jonesConfig_SetTextControl`, where only a
  declaration moved). In all, 52 functions in 13 source files differ.
- Most of the readme's fixes (Volodnikov, the level problems, the part sounds) have no counterpart in the executable
  and are presumably in the updated level scripts.

### The 1.2 changes as reconstructed

| File | 1.2 change |
| --- | --- |
| `jonesConfig.c` | the 42 character keys are named by `GetKeyNameTextA` instead of fixed strings; the store cart no longer reads in front of its item table when no bonus map is chosen; a purchase may spend the whole balance; the insert-CD dialog hides the cursor when it closes |
| `Indy3D.rc` | the key-remap dialog's controls moved and resized |
| `JonesHud.c` | two more entries in the credits |
| `sithPlayerControls.c` | pushing or pulling the Marduk statue in Nub's Tomb (`2marpushstatue`) searches from slightly lower |
| `sithCamera.c` | an external camera restarts its focus interpolation when the canvas is set |
| `sithWeapon.c`, `sithWhip.c` | the whip clears the deactivated weapon, and the whip thing on an early exit |
| `sithCogParse.c` | a vector leaf tests the node it got, not the address of the function that returned it |
| `sithCogFunctionThing.c` | `CreateLaser` flags an invalid end position as an argument error |
| `sithActor.c` | a player damaged by itself with three damage types counts as having played its damage sound |
| `JonesConsole.c`, `sithGamesave.c` | the build date |
| `JonesMain.c`, `jonesCog.c` | the line numbers their assertions and log calls embed |

The register and ordering differences in `jonesConfig.c` and `sithPlayerControls.c` that follow from these edits are
reproduced by re-fitting those files' declaration-count blocks for 1.2
([DECISIONS.md](DECISIONS.md#which-version-the-lists-describe)).

1.1 was built separately, from another build environment with a different link order. 1.2 carries most of its
changes, but not three of them: the end-of-level statistics update, a level test in the chapter-complete dialog, and
a change to `JonesHud_Startup`.

## Steam

The Steam executable is **1.2 unchanged**, with a Disney Interactive Studios Authenticode signature appended. Clear
the PE checksum and the security-directory entry and cut the file to 1,539,072 bytes, and it is byte-identical to
1.2.

## GOG

The GOG executable is **1.2 with 267 bytes of hand-made patches** to the code. Its header, imports and data are
unchanged; new code sits in the alignment padding between functions. The patches fall into seven groups:

| | Change | Function(s) |
| --- | --- | --- |
| A | Frame limiter: at least 15 ms per fullscreen flip (about 66 fps) | `stdDisplay_Update` |
| B | Minimum frame time raised from 1 ms to 5 ms | `sithTime_Advance` |
| C | Widescreen HUD: square health indicators, inventory item depth scaled by aspect ratio | `JonesHud_UpdateHUDLayout`, `JonesHud_UpdateItem`, `JonesHud_ResetMenuItems`, `JonesHud_InventoryItemChanged` |
| D | Display enumeration: the 64-mode limit removed; the device list cut from 16 to 2 entries to make room | `stdDisplay_EnumVideoModesCallback`, `stdDisplay_DDEnumCallback` and the device accessors |
| E | Settings: any colour depth accepted when choosing a mode; defaults of 32-bit colour, trilinear filtering and starting without the setup dialog; window mode ignored; registry moved from `HKEY_LOCAL_MACHINE` to `HKEY_CURRENT_USER` | `JonesDevDialog_FindClosestVideoMode`, `JonesDevDialog_LoadSettings`, `JonesMain_Startup` |
| F | Game shutdown moved from the window and startup-failure paths to process exit | `wkernel_Run`, `wkernel_MainWndProc`, CRT `doexit` |
| G | Streamed (cutscene) sound tracks forced to priority 0 | `sound_StartStream` |

The GOG build starts on current Windows where 1.0 does not. The likeliest reason, inferred from the code and not
tested, is D with E: current drivers report more than 64 display modes, and 1.0 and 1.2 keep only the first 64 and
accept only 16-bit ones. A to C are frame-rate and HUD fixes; the purpose of F and G is not known.
