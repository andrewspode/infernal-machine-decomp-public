# Building

How to set up the toolchain, which copy of the game you need, and what each build target does. Every pinned input
the setup script downloads is listed in [REPRODUCIBILITY.md](REPRODUCIBILITY.md).

## The original executable

Each version's build needs only its own executable:

| Version | Build | Executable | SHA-256 |
| --- | --- | --- | --- |
| **1.2** | `ninja` (the default) | `Indy3D.exe` from LucasArts' update `J3DUpdate1.2.exe` | `55fb00c0a2cb30793ff451dc6fd17681c5128aa32419aa711015fd5117d608bf` |
| **1.2** | `ninja` (the default) | `Indy3D.exe` from Steam, which the setup script turns into the file above *(see Steam, below)* | `7faf42d593ab770c3bfbec14e0f1dbff738d3552d732e81561c78ac4054a75d4` |
| **1.0** | `ninja v1.0` | `Indy3D.exe` from the original CD (`Resource/Indy3D.EXE`) | `3fbaf8cd401b4af80967cbe42e3420fb803288b336ebbe72a9a01b6dfd661a53` |

It is needed for three things:

1. **Verification.** Every source file is compiled and compared against the matching slice of the original; that
   comparison is what "byte-exact" means.
2. **Artwork.** The executable's icons are LucasArts' artwork, so the build copies them from the retail executable
   rather than shipping them.
3. **A little library code and some unused bytes** *(2,627 bytes in 1.2)*: Intel's `floor` and `modf`, three C runtime
   string helpers, `longjmp`, a few statics of Intel's maths library, and some constants and gaps nothing reads. The
   build can't take these from the libraries we have, so it copies them from the retail executable
   ([why](DECISIONS.md#10-library-code-taken-from-the-original)).

**Steam** ships 1.2 unchanged with a Disney Interactive Studios Authenticode signature appended. The setup script
removes it: it clears the PE checksum (4 bytes at file offset `0x128`) and the security-directory entry (8 bytes at
`0x168`) and cuts the file to 1,539,072 bytes, which gives 1.2's SHA-256.

**GOG** ships 1.2 with GOG's own binary patches to its code, so it is not suitable.

**1.1**, the first official update, is not suitable either. It was built separately, with a different link order, and
1.2 replaced it two months later, so we chose to reconstruct 1.2 instead. [VERSIONS.md](VERSIONS.md) lists the other
builds and what each update changed.

**1.0** is on the original CD. The disc image preserved on archive.org,
[Indiana Jones and the Infernal Machine (LucasArts), Disc 1](https://archive.org/details/Indiana_Jones_and_the_Infernal_Machine_LucasArts_Disc_1),
holds it. Extract `Indy3D.exe` yourself; no script in this repository downloads the game.

## Setting up and building

Requirements: Linux x86_64 (tested) or macOS (Apple Silicon through Rosetta 2); on Windows, build inside
[WSL](https://learn.microsoft.com/windows/wsl/install). Python 3.10 or later, git, [Ninja](https://ninja-build.org/),
7-Zip, [unshield](https://github.com/twogood/unshield) 1.4 or later, `make`, a C++20 compiler and network access for
the first setup.

```sh
# Debian/Ubuntu: apt install python3 python3-venv git ninja-build 7zip unshield g++ make
# macOS:         brew install ninja sevenzip unshield cmake
python3 Scripts/setup_toolchain.py --exe /path/to/Indy3D.exe   # 1.2 or 1.0; repeat --exe for both
ninja         # 1.2 (needs the 1.2 executable)
ninja v1.0    # 1.0 (needs the 1.0 executable)
```

The setup script installs every compiler, [wibo](https://github.com/decompals/wibo) (which runs them), objdiff and the
DirectX 6.1 SDK into ignored directories of the checkout, checks each download against a pinned SHA-256 or commit,
recognises each executable you pass by its SHA-256 (removing Steam's signature first) and copies it to
`orig/Indy3D-1.2.exe` or `orig/Indy3D.exe`. It can be run again at any time. Without `--exe` it sets up everything
else and `ninja converted` works; `--icl-installer PATH` uses a local copy of the Intel evaluation installer.

## Build targets

| Command | Result |
| --- | --- |
| `ninja` | 1.2: compile, split, rebuild, verify the SHA-256, record progress |
| `ninja converted` | compile every source file; needs no original |
| `ninja match` | strict check of every matching file's code, data and BSS |
| `ninja check` | rebuild the executable and require the original's SHA-256 and SHA-1 |
| `ninja progress` | write `build/indy3d-v1.2/progress.json` (verified files and bytes) |
| `ninja report` | write objdiff's per-function scores to `build/indy3d-v1.2/report.json` |
| `ninja objdiff` | build the comparison objects for the [objdiff](https://github.com/encounter/objdiff) GUI |
| `ninja map` | write the address ownership map of the rebuilt executable |
| `ninja source-only` | the end-state gate: a rebuild with no original fallback (fails until everything is source) |
| `ninja debug` | an unoptimised build with the debug-only code compiled in; it is not byte-matched ([DEBUG-BUILD.md](DEBUG-BUILD.md#building-it)) |
| `ninja v1.0`, `v1.0-check`, `v1.0-match`, `v1.0-report`, ... | the same targets for 1.0 |

1.2's outputs go to `build/indy3d-v1.2/`, 1.0's to `build/indy3d-v1.0/`. After changing build options, run
`.venv/bin/python configure.py` again. To compare one function in a terminal:

```sh
build/tools/objdiff-cli diff -p . -u Jones3D/Play/jonesCog.c _jonesCog_Startup
```

## How it works

The build splits your executable into one object per original source file, compiles each file, checks it against its
split, and reassembles the image at the original addresses. [BUILD-PIPELINE.md](BUILD-PIPELINE.md) describes each step.
