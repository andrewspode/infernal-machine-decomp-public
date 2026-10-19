# Reproducibility: every pinned input

A byte-for-byte claim is only worth something if anyone can repeat it. This document lists every external input of the
build, how `Scripts/setup_toolchain.py` fetches it and what pins it. The one thing no script
downloads is the game: you supply your own `Indy3D.exe`, 1.2 or 1.0 (or both).

## The rule

Every input is pinned by a SHA-256 or a git commit and is checked **before** it is used. The setup script never
accepts a file that does not match and leaves an existing but different checkout alone with an error. It can be run
again at any time: finished steps are verified and skipped. Nothing is installed outside the checkout: everything goes
into ignored directories *(the toolchains under `tools/`, plus `build/`, `.venv/` and the DirectX SDK's `include/` and
`lib/` under `Libs/external/DirectX61c/`)*.

## The inputs

| Input | Version and pin | Fetched from | Used for |
| --- | --- | --- | --- |
| The original executables | 1.2 (LucasArts' update, or Steam's copy unsigned), SHA-256 `55fb00c0a2cb30793ff451dc6fd17681c5128aa32419aa711015fd5117d608bf`; 1.0 (the CD release), SHA-256 `3fbaf8cd401b4af80967cbe42e3420fb803288b336ebbe72a9a01b6dfd661a53` | you supply them (`--exe`); a 1.0 disc image is preserved on archive.org | each version's split, strict check and final hash; a version's build reads only its own |
| Visual C++ 5.0 SP3 | driver 11.00.7022, front end `c1.dll` 11.00.7113, back end `c2.exe` **11.00.7303**; git commit `259a03f0bc863de5baf657ec064cb60cb20a2cdc`; `c1.dll` SHA-256 `7f12a4a8…983d5d`, `c2.exe` `e75aecaf…4515fae` | [archaic-msvc/msvc500sp3](https://github.com/archaic-msvc/msvc500sp3) | the game code, the C runtime library and the platform import libraries |
| Visual C++ 4.20 | C1/C2 10.20.6166; git commit `b42c244f0a83ba15ba2ffb62b0dc240d7b2dea50`; `CL.EXE` SHA-256 `c5bf7ad8…50585f`, `C1.EXE` `c5a62937…643b90`, `C2.EXE` `674fb9e4…273ae9`; `MSVCRT40.DLL` (SHA-256 `ab55a2de…30f030`) and `MSVCRT.DLL` (`e390b052…43d210`) are copied from the VC5 SP3 package's `redist` directory, because the drivers import them and wibo does not provide them | [archaic-msvc/msvc420](https://github.com/archaic-msvc/msvc420) | `stdUtil_StrCmp`, the one separately built helper object |
| Visual C++ 6.0 RTM | 12.00.8168; git commit `93d5bbe2582aa5b78a3a8dddd9498530fdd74061` | [isledecomp/MSVC600-8168](https://github.com/isledecomp/MSVC600-8168) | INSANE, SmushPlay, zlib |
| Intel C/C++ 4.0 | build 99100, evaluation installer `compeval.exe` (5,102,735 bytes) SHA-256 `931fecd143c7532dc944bad4d2a7a1aadf76d143c08161eec25ba8801131746e`; driver patched to `icl_notrial.exe` SHA-256 `fed5dc869b8d1053778586684fb3bfe4091d22399f2d8aa9028dc538ba64ad81` | [andrewspode/intel-cc-4.0-setup](https://github.com/andrewspode/intel-cc-4.0-setup), git commit `5adedde9ddf0a539f5d8bd5bc6227ebd7b2d29f6`, which downloads the installer from archive.org; `--icl-installer PATH` uses a local copy | `rdModel3K`, and the reference reconstructions of Intel's `floor` and `modf` |
| MASM 6.13 | `ml.exe` 6.13.7299, SHA-256 `3159512d1fb4f8b9e4c9ec4125d06c3e9d9adc5a675715670f4ea705f5472705`, built from the Windows 98 SE DDK (`win98se_ddk.exe`, SHA-256 `08318354cbac0b17d2e976a70b901ceb336947e76e4af8c869efdc6327a6a5dc`; its ML 6.11d) and Microsoft's ML613 patch (`MASM 6.13.zip`, SHA-256 `61ab1f2e14622a3721097e32aaadc436f8ebd385d361c5944063d7b1fa76c8ce`) | archive.org (`win98se_ddk.exe`, `masm-6.13`); the patch's DOS program runs under [emu2](https://github.com/dmsc/emu2) at commit `9d8698d06e67359bc8b230c2d19d5b1503e60819`; 7-Zip 24.09's official console binary (SHA-256 pinned per host) unpacks the archives | INSANE's assembler files |
| wibo | 1.1.0, release binaries pinned per host; Linux x86_64 SHA-256 `b9ab9f2c5eb014bc79a97f1a2bee9a7ab177d905a7d2f637da5735820021c40c` | [decompals/wibo](https://github.com/decompals/wibo) releases | runs the Windows compilers on Linux and macOS |
| wibo with the NT heap model | wibo source at commit `b80feaae19c1b0b0759d9abf2b6afabc48957c3d` (tag 1.1.0) plus `tools/icl/wibo-heap-model.patch`, built locally | the same repository, built by `Scripts/build_wibo_heapmodel.py` | every compile of the production build |
| CMake | 3.30.5, Linux x86_64 archive SHA-256 `f747d9b23e1a252a8beafb4ed2bc2ddf78cff7f04a8e4de19f4ff88e9b51dc9d` (other hosts: a `cmake` 3.13 or later on `PATH`) | [Kitware/CMake](https://github.com/Kitware/CMake) releases | building the heap-model wibo |
| libclang | `libclang==18.1.1` wheel, SHA-256 `c533091d…aa2a0b` on Linux | PyPI | wibo's trampoline generator |
| objdiff | 3.5.1, binaries pinned per host; Linux x86_64 SHA-256 `8f032cd27420488710c14ae367cad682cde7fe1181809b8f02ab3540ef9b74c5` | [encounter/objdiff](https://github.com/encounter/objdiff) releases | comparison reports; not used by the strict check |
| DirectX 6.1 SDK | `dx61sdkimage.exe`, SHA-256 `4ce3b6737683cb2ee4a59edd1fbc25f73691acdaa3d5b80d7135d2c0b5dd39b1` | archive.org, extracted with 7-Zip (`include/` and `lib/` only) | headers; `ddraw.lib`, `dsound.lib`, `dinput.lib` |
| Python packages | pinned by version, not by hash: `pefile==2024.8.26`, `capstone==5.0.7`, `PyYAML==6.0.3`, `pyte==0.8.2` | PyPI, in `.venv` | the split, matcher and image builder |

The SHA-1 of each executable is in its `config/<version>/config.yml` and `build.sha1`: 1.2
`60ebd56d454197683d9891b13e48c87205fc5d92`, 1.0 `396d984a55df19bfa7137040bd1e90fbff2d22c0`. The build requires both
digests.

### Pins inside the build

The build pins further files itself, so a wrong archive fails even if the setup script was bypassed:

- `libraries.yml` records the SHA-256 of `libc.lib` (`3b5f43e920c37c8da086b0d3ef0396e6517d44fcdcb4e7d03a6058620bc13ee3`)
  and of the SDK's `dinput.lib` (`4164d8a8f4f866d0826f24a2a1a80819836c08e5a5b1fa07614c286f2285d211`); an archive is
  hashed before any member is extracted ([BUILD-PIPELINE.md](BUILD-PIPELINE.md#library-units)).
- `image/imports.yml` records the SHA-256 of each of the twelve import libraries, among them `ddraw.lib`
  (`37cadc2a…db9aeab`), `dsound.lib` and `dinput.lib`.
- The resource compiler refuses an executable whose SHA-256 is not the expected one before it extracts the icons.

## What the setup script does

The commands are in [BUILDING.md](BUILDING.md). In order, the script:

1. clones the Visual C++ 6.0 RTM, 5.0 SP3 and 4.20 packages at their commits (checking the head against the pin),
   hashes the service pack's updated `c1.dll` and `c2.exe` (the driver banner is the RTM one) and the 4.20 drivers, and
   copies the two runtime DLLs the 4.20 drivers import from the VC5 SP3 package;
2. downloads the wibo binary for the host and checks its hash;
3. runs the three Microsoft compilers once through wibo and requires their version strings;
4. installs Intel C/C++ 4.0 through the setup repository (7-Zip and `unshield` unpack the installer, no Wine and no
   installer run) and requires the driver hash and the banner `Version 4.0  99100`;
5. rebuilds MASM 6.13: unpacks ML 6.11d from the Windows 98 SE DDK, applies Microsoft's ML613 patch with its own DOS
   patch program under emu2 (cloned at the pinned commit and built with `make`), and requires the result's hash and the
   banner `Version 6.13.7299`;
6. builds the heap-model wibo from the pinned source and patch, then checks that it runs the VC5 compiler, and builds
   the optional Cog parser generators ([below](#optional-the-cog-parser-generators); a failure is only reported);
7. downloads objdiff and the DirectX SDK image and hashes them;
8. recognises each executable passed with `--exe` by its SHA-256 and copies 1.2 to `orig/Indy3D-1.2.exe` and 1.0 to
   `orig/Indy3D.exe`; a file that matches neither but carries an Authenticode signature (Steam's) is checked again with
   the signature removed (the PE checksum at file offset `0x128` and the security-directory entry at `0x168` cleared,
   the file cut where the signature starts, 1,539,072 bytes). It then creates `.venv` with the pinned packages and
   generates the build files.

Without `--exe` it sets up everything except the original, and `ninja converted` is available.

The host tools the script cannot pin *([requirements](BUILDING.md#setting-up-and-building))* run the build but are
not part of what the compilers see. The heap-model wibo is built from pinned source but is not itself hash-checked; a
stamp file records the source commit and the patch's hash and triggers a rebuild when they change.

## Why the host does not change the output

The compilers are 1990s programs run under an emulation layer, so reproducibility needs an argument as well as a
checksum. [COMPILERS.md](COMPILERS.md#running-1990s-compilers-on-a-modern-host) gives the evidence:

- The Microsoft compilers' output does not depend on the host. 162 Visual C++ units were recompiled under sixteen
  deliberately different memory layouts (2,592 compiles) and were byte-identical every time, and five units compiled
  natively on Windows 11 were identical to the wibo objects in every section.
- Intel's compiler orders operands by the address of its own heap records, so its output **does** depend on the host
  heap. The patched wibo gives it a model of the 1999 NT heap, which reproduces what the compiler gives on Windows; every
  compile in the build runs under it, Microsoft's included.

## Checking a build

`ninja` (or `ninja v1.0`) ends with `verify.ok` only when the rebuilt `Indy3D.exe` has the original's SHA-256 and
SHA-1, after each matching file has passed the strict check ([METHODOLOGY.md](METHODOLOGY.md#what-matching-means)).
The other targets are in [BUILDING.md](BUILDING.md#build-targets). To check that a result is independent of your
checkout, build twice, in two directories, or on two hosts, and compare the objects under `build/indy3d-v1.2/obj/`. A
COFF object carries a compile timestamp that differs between runs; the section contents and relocations do not.

### What a public export has been checked for

This repository is generated from the development repository. An export is verified by building it in a fresh
directory with the toolchain of a checkout that ran the setup script: the setup script verifies every pin and downloads
nothing, `ninja` and `ninja v1.0` rebuild each version's `Indy3D.exe` with its original's SHA-256, every object is
byte-identical to the development build's (COFF timestamp masked), and in a second export with no `orig/`,
`ninja converted` compiles every object.

## Optional: the Cog parser generators

`Libs/sith/Cog/sithCogYacc.c` (with the generated part of `sithCogYacc.h`) and `sithCogFlex.c` are generator output,
checked in so that the build never needs the generators. Their sources are `cog.y`, `cog.l` and the scanner skeleton
`flex.skel` beside them. To regenerate them:

```sh
python3 Scripts/cog_generators.py build    # or: ninja cog-generators
python3 Scripts/cog_generators.py check    # exit 1 if the checked-in files differ from the generators' output
python3 Scripts/cog_generators.py regen    # or: ninja regen-cog
```

The setup script runs `build` once. It downloads the comp.sources.unix postings of Berkeley yacc 1.4 (volume 21, issues 78-82, 1990) and flex 2.3 at
patchlevel 6 (volume 23, issues 37-46) from `sources.vsta.org`, checks each shar part against a pinned SHA-256 (in the
script), unpacks them without running them and builds both natively with the host C compiler, into
`build/cog-generators/`. Before use, each must reproduce its own distribution's shipped output (byacc its test parsers,
flex its bootstrap scanner).

## What cannot be reproduced by anyone

- The game: the repository holds no game assets and no original code. The icons of the `.rsrc` section, and the
  2,627 bytes of library code and unused bytes the build cannot take from source or the libraries (1.2), are copied
  from your executable at build time
  ([BUILD-PIPELINE.md](BUILD-PIPELINE.md#what-is-still-copied-from-the-retail-executable)).
- Sources of the compilers: only the shipped binaries exist, so the toolchain is "reproducible" in the sense that the
  same bytes, checked by hash, always reach the build.
- Third-party hosting: several inputs are on GitHub or archive.org. If one disappears, the hash or commit says exactly
  what to look for. `--icl-installer` lets you supply the Intel installer yourself, and the executable is always yours.
