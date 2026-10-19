# Indiana Jones and the Infernal Machine

**A 100% [byte-exact](docs/BYTE-BREAKDOWN.md) decompilation of LucasArts' *Indiana Jones and the Infernal Machine* (1999)**: C source that, built with the
same compilers LucasArts used, turns back into the original `Indy3D.exe`, byte for byte.

**This repository is not a port or a modernisation**; it is a reference for how the game was actually written, for
preservation and to help ports in development.

[OpenJones3D](https://github.com/smlu/OpenJones3D/) was our starting point, but only around 15% of its functions were
byte-exact. Partly that was because some of the game wasn't there yet *(the INSANE movie player, zlib, the SSE renderer
and a few hundred functions that still called into the retail executable)*, but also because they intentionally fixed
bugs and improved things along the way, drifting further from LucasArts' code.

Despite the drift, it still gave us a huge head start: the file names and module layout, function and
variable names, structures and types, and readable C for most of the game. That's far more than most decompilations
have on day one, and truly awe-inspiring work, the result of nearly a decade spent on this game.

Using AI assistance *(mostly Opus 5.5)* and tooling loosely based on
[decomp-toolkit](https://github.com/encounter/decomp-toolkit), it took a little over a month to track down the
exact compilers used in 1999 and rebuild the code. A byte match on its own proves very little, as code can be bent
until the bytes line up, so **the goal throughout was the source LucasArts actually wrote**, as close as we could get
it, not 100% for the sake of it.

[Strict rules](docs/METHODOLOGY.md#what-is-not-allowed) banned the usual tricks for forcing a match, and every decision the evidence couldn't settle on
its own was logged. Of the 3,300 functions, 3,102 rest on evidence alone; the other 198 depend on a measured choice,
with me as the final decision-maker. Each one is labelled in the source, and every
decision is [documented](docs/DECISIONS-LIST.md).

In the process, we also released [intel-cc-4.0-setup](https://github.com/andrewspode/intel-cc-4.0-setup), which
installs the Intel C/C++ 4.0 compiler that took some serious sleuthing to find *(if anyone has the retail version,
please get in touch)*, and a [fix to wibo's heap emulation](https://github.com/decompals/wibo/pull/153), both of which
should help other decompilation projects. All the files OpenJones3D was missing will also be contributed straight
back to it.

A big thank you to [smlu](https://github.com/smlu) of [OpenJones3D](https://github.com/smlu/OpenJones3D/) for their
help and support, and to [Jack Price-Burns](https://jpb.dev) of [Decomp Academy](https://decomp-academy.dev/), who set
up the initial toolchain and the anti-slop guidelines the agents worked under, based on his extensive experience bringing
[Star Fox Adventures](https://github.com/zcanann/SFA-Decomp) to 100%.

## What's in this repository

- **The source code** for versions 1.2 *(latest update)* and 1.0 *(retail CD)*, including the functions found only in the
  [debug build](docs/DEBUG-BUILD.md).
- **The toolchain**: a setup script that downloads and verifies every compiler the game was built with, and a build
  that recreates `Indy3D.exe` and checks it against your own copy of the game ([BUILDING.md](docs/BUILDING.md)).
- **The documentation**: how it was built, the decisions we made and what we learned about the original code.

| Page | What it covers |
| --- | --- |
| [BYTE-BREAKDOWN.md](docs/BYTE-BREAKDOWN.md) | Where every byte of the rebuilt executable comes from, and figures for every area and file |
| [BUILDING.md](docs/BUILDING.md) | How to set it up and build it, and which copies of the game work |
| [METHODOLOGY.md](docs/METHODOLOGY.md) | The rules we followed, and what counts as a match |
| [DECISIONS.md](docs/DECISIONS.md) | The evidence behind the source, and every kind of choice the evidence couldn't settle |
| [DECISIONS-LIST.md](docs/DECISIONS-LIST.md) | Every function that rests on one of those choices, and the choice made |
| [SOURCE-TREE.md](docs/SOURCE-TREE.md) | How LucasArts laid out their source, how we mirror it, and what else is in the repository |
| [HOUSE-STYLE.md](docs/HOUSE-STYLE.md) | The coding conventions LucasArts' programmers followed |
| [VERSIONS.md](docs/VERSIONS.md) | The releases of `Indy3D.exe` relevant here *(including Steam and GOG)*, and what 1.2 changed |
| [DEBUG-BUILD.md](docs/DEBUG-BUILD.md) | The debug build found on the Italian CD, and what it taught us |
| [INSANE.md](docs/INSANE.md) | The SMUSH/INSANE movie player built into the game |
| [COMPILERS.md](docs/COMPILERS.md) | How we identified all five build tools *(four compilers and an assembler)* and their flags |
| [COMPILER-INTERNALS.md](docs/COMPILER-INTERNALS.md) | What we learned about how Visual C++ 5.0 generates code |
| [BUILD-PIPELINE.md](docs/BUILD-PIPELINE.md) | How the build works, step by step, and what it still copies from the retail executable |
| [REPRODUCIBILITY.md](docs/REPRODUCIBILITY.md) | Every download the build depends on, and how each one is pinned |

## Additional acknowledgments

Thanks to [OpenJKDF2](https://github.com/shinyquagsire23/OpenJKDF2), whose work on the related Jedi Knight engine
informs many names, and to the authors of [wibo](https://github.com/decompals/wibo),
[objdiff](https://github.com/encounter/objdiff) and the compiler packages listed in
[REPRODUCIBILITY.md](docs/REPRODUCIBILITY.md). zlib 1.0.9 is by Jean-loup Gailly and Mark Adler.

## License

[AGPL v3](LICENSE), inherited from OpenJones3D.
