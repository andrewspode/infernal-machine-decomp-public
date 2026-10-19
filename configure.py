#!/usr/bin/env python3
"""Configure the MSVC/PE decompilation build, following the DTK project layout."""
import argparse
from pathlib import Path
import shlex
import subprocess
import sys

from Scripts.pe_project import OVERLAY_DIRS
from tools.project import Object, ProgressCategory, ProjectConfig, calculate_progress, generate_build

# indy3d-v1.2: the official 1.2 update (orig/Indy3D-1.2.exe), the shared source tree and the default build.
# indy3d-v1.0: retail CD 1.0 (orig/Indy3D.exe), the same tree with the files of versions/1.0/ in place of the ones
# they mirror (README.md, "Versions"). A configure of the default version also builds every other version that has
# a config/<version>/ directory, under its own target prefix (`ninja v1.0`, README.md). The default is the first
# version with a configuration. Each version's build reads only its own executable.
VERSIONS = ['indy3d-v1.2', 'indy3d-v1.0']
TARGET_PREFIXES = {'indy3d-v1.2': 'v1.2-', 'indy3d-v1.0': 'v1.0-'}


def default_version():
    root = Path(__file__).resolve().parent
    return next((v for v in VERSIONS if (root / 'config' / v / 'config.yml').is_file()), VERSIONS[0])


def parse_version(value):
    for version in VERSIONS:
        if value.lower() == version.lower():
            return version
    raise argparse.ArgumentTypeError(f'Unknown version: {value}')


def make_config(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('mode', choices=['configure', 'progress'], default='configure', nargs='?')
    parser.add_argument('-v', '--version', type=parse_version, default=default_version())
    parser.add_argument('--build-dir', type=Path, default=Path('build'))
    parser.add_argument('--wrapper', '--wibo', type=Path, default=Path('build/tools/wibo'),
                        help='Path to the pinned release wibo (tools only; compiles use --wibo-heapmodel)')
    parser.add_argument('--wibo-heapmodel', type=Path, default=Path('build/wibo-heapmodel/wibo-heapnt'),
                        help='Path to the heap-model wibo every compile runs under (Scripts/build_wibo_heapmodel.py)')
    parser.add_argument('--compilers', type=Path, default=Path('tools/msvc500sp3-7303'), help='MSVC500 SP3 package directory (default compiler, see docs/compiler-default.md)')
    parser.add_argument('--compiler', type=Path, help='Override the exact CL.EXE path')
    parser.add_argument('--objdiff', type=Path, default=Path('build/tools/objdiff-cli'))
    parser.add_argument('--ninja', type=Path)
    parser.add_argument('--map', action='store_true', help='Generate the PE ownership/contribution map')
    parser.add_argument('--debug', action='store_true', help='Compile with debug flags; matching validation still applies')
    parser.add_argument('--verbose', action='store_true', help='Show per-library progress')
    parser.add_argument('--warn', choices=['all', 'off', 'error'], default='all')
    parser.add_argument('--no-progress', dest='progress', action='store_false')
    parser.add_argument('--cflags', help='Override the default compiler flag list')
    parser.add_argument('--public', action='store_true',
                        help='Generate the public build graph (no test, parsecheck or audit targets); '
                             'the default when tests/ is absent, as in the public export')
    parser.add_argument('--only-version', action='store_true',
                        help='Build only --version (by default a configure of the default version also '
                             'builds the other configured versions under their target prefixes)')
    args = parser.parse_args(argv)

    config = ProjectConfig()
    config.version = args.version
    config.overlay_dir = OVERLAY_DIRS.get(args.version)
    config.build_dir = args.build_dir
    config.wrapper = args.wrapper
    # Every compile (VC5 SP3, VC6 RTM and Intel C 4.0) runs under wibo 1.1.0 with the NT heap model
    # (docs/crt.md, "A principled 1999 heap model"). mcpcom orders
    # commutative operands by its symbol records' heap addresses, so ICL output depends on the host heap;
    # the model gives the object the compiler produces on Windows. MSVC output is byte-identical either way.
    config.compile_wrapper = args.wibo_heapmodel
    config.compile_env = {'WIBO_HEAP_MODEL': 'nt'}
    config.compilers_path = args.compilers
    config.compiler_path = args.compiler
    config.compilers = {
        # The project-wide compiler (docs/compiler-default.md).
        # VC5 RTM, VC5 SP1, VC6 RTM (as a *compiler* - its CRT libraries are still needed to
        # link/check, see below), VC6 SP3 and VC6 beta 2 are ruled out and no longer configured
        # here; their pinned installs remain on disk under tools/ (git-ignored) but nothing
        # references them.
        'VC5_SP3': {'path': Path('tools/msvc500sp3-7303/bin/cl.exe'), 'version': '11.00.7022',
                    'flags': ['/B1c1.dll', '/B2c2.exe']},
        # VC6 RTM (12.00.8168): the compiler of retail's zlib 1.0.9 (docs/crt.md, "zlib"): built from the
        # published source at `/O2 /Zp1` (the INSANE project's settings). The zlib and SMUSH/INSANE units use it.
        'VC6_RTM': {'path': Path('tools/msvc600-8168/VC98/Bin/CL.EXE'), 'version': '12.00.8168',
                    'include_dir': 'Include', 'flags': []},
        # Intel C/C++ 4.0 (evaluation build 99100, trial check patched out of the driver by
        # Scripts/setup_toolchain.py): the compiler of rdModel3K and the Intel libm helpers
        # (docs/crt.md). A hard requirement like the MSVC packages; it takes ICL options (`-`),
        # its own SSE headers by include path, and the MSVC/DirectX headers for the game types.
        'ICL40': {'path': Path('tools/intel-cc-4.0/Compiler_Bin_Files/icl_notrial.exe'),
                  'version': 'Version 4.0  99100', 'kind': 'icl', 'include_dir': 'Compiler_Include_files',
                  'flags': []},
        # Visual C++ 4.20 (C1/C2 10.20.6166, archaic-msvc/msvc420, installed by Scripts/setup_toolchain.py
        # with VC5 SP3's redistributable MSVCRT40.DLL beside it): the compiler of the separately built,
        # optimised stdUtil_StrCmp object linked into both the release and the debug build
        # (config/indy3d-v1.0/stdUtil.md). Every Microsoft C compiler before VC5 (2.0..4.20) gives its bytes;
        # VC5 and VC6 emit `or eax,-1` and schedule the second call's argument early.
        'VC4_20': {'path': Path('tools/msvc420/bin/CL.EXE'), 'version': '10.20.6166', 'include_dir': 'include',
                   'flags': []},
        # Microsoft Macro Assembler 6.13.7299 (ML613 patch over the Win98 SE DDK's 6.11d, installed by
        # Scripts/setup_toolchain.py): the assembler of INSANE's six MASM objects, whose @comp.id is the
        # retail Rich header's six Masm613 7299 entries (docs/crt.md, "MASM 6.13"). COFF output, no listing.
        'ML613': {'path': Path('tools/masm613/ml.exe'), 'version': '6.13.7299', 'kind': 'masm',
                  'flags': ['/coff']},
    }
    config.objdiff_path = args.objdiff
    if args.ninja:
        config.ninja_path = args.ninja
    config.generate_map = args.map
    config.progress = args.progress
    config.progress_each_module = args.verbose
    config.public = args.public or not (config.root / 'tests').is_dir()
    # Save canonical arguments for Ninja regeneration, including progress-mode invocations.
    config.configure_args = ['configure', '-v', args.version, '--build-dir', str(args.build_dir),
                             '--wrapper', str(args.wrapper), '--wibo-heapmodel', str(args.wibo_heapmodel),
                             '--compilers', str(args.compilers),
                             '--objdiff', str(args.objdiff), '--ninja', str(config.ninja_path), '--warn', args.warn]
    if args.compiler:
        config.configure_args += ['--compiler', str(args.compiler)]
    for enabled, flag in ((args.map, '--map'), (args.debug, '--debug'),
                          (args.verbose, '--verbose'), (not args.progress, '--no-progress')):
        if enabled:
            config.configure_args.append(flag)
    if args.cflags is not None:
        config.configure_args += ['--cflags', args.cflags]
    if args.public:
        config.configure_args.append('--public')
    if args.only_version:
        config.configure_args.append('--only-version')

    # The one release flag set of the Jones3D project and its Libs (docs/compiler-flags-audit.md): every
    # VC5 game TU uses it unchanged. It is VC5's `/O2 /G5 /ML /DNDEBUG` written out: VC5's /O2 itself
    # implies /Ob1 /Oi /Ot /Oy /Gf /Gy, and the bare `/O2 /G5 /ML /DNDEBUG /W3` compile is byte-identical
    # to this list on all 131 VC5 TUs. /Gf (writable pooled strings) is that /O2 default, not a
    # per-file choice: retail keeps every game TU's string literals in .data. The expansion stays
    # explicit because the /Od diagnostic tools (tools/probes/dbgverify.py) drop only the optimisation
    # flags and must keep /Gf and /Gy.
    # /ML: retail links the single-threaded CRT (no TlsAlloc/TlsGetValue import; Driver.c tests the
    # global errno, sithCogFlex.c inlines putc). Byte-neutral elsewhere; docs/declctx-tus.md (setup1).
    cflags_base = ['/O2', '/Ob1', '/Oi', '/Ot', '/Oy', '/G5', '/ML', '/Gf', '/Gy', '/DNDEBUG']
    if args.debug:
        cflags_base = ['/Od', '/Zi', '/G5', '/MT', '/Gy', '/D_DEBUG']
    cflags_base += {'all': ['/W3'], 'off': ['/W0'], 'error': ['/W3', '/WX']}[args.warn]
    if args.cflags is not None:
        cflags_base = shlex.split(args.cflags)
    config.cflags = cflags_base

    Matching = True
    NonMatching = False

    def MatchingFor(*versions):
        return config.version in versions

    # A Matching entry selects the ENTIRE TU: text, constants, initialized data
    # and BSS must all pass the strict linker. No per-function selection exists.
    # Object(MatchingFor('indy3d-v1.0'), 'path.c', extra_cflags=[...]) is supported.
    # MatchingFor('indy3d-v1.0') marks a TU the 1.2 update changed (docs/version-diff.md) whose 1.2 source (the
    # shared file) is not recovered yet: Matching for 1.0 (its copy in versions/1.0/), NonMatching for 1.2. A
    # changed TU is Matching for both once the shared file passes the 1.2 match (config/indy3d-v1.2/README.md).
    #
    # The source manifest, by original project and compiler (docs/compiler-flags-audit.md). Object order
    # within a project follows retail's link order where it is known; it affects no byte (the image is
    # placed at retail's fixed addresses), only the order of generated lists.
    #
    # 1. The game: the Jones3D project and its libraries (C:\Jones3D\Product\Indy3D, C:\Jones3D\Libs\...),
    #    Visual C++ 5.0 SP3 with the one release flag set `cflags_base`. No game TU has a per-file flag;
    #    an Object here names only its source and, where retail evidences it, its original path
    #    (`original_filename` supplies the __FILE__ text through /DJ3D_FILE, not a flag choice).
    game = {'compiler': 'VC5_SP3', 'cflags': cflags_base}
    # 2. Third-party and separately built objects, each a project of its own by evidence and uniform within:
    #    Visual C++ 4.20 (one prebuilt object), Intel C/C++ 4.0 (Katmai, libm), Visual C++ 6.0 RTM
    #    (zlib, SMUSH/INSANE) and MASM 6.13 (INSANE's assembler).
    vc6_insane = {'compiler': 'VC6_RTM', 'cflags': ['/O2', '/Zp1', '/Gy']}
    config.libs = [
        {'lib': 'Jones3D', **game, 'objects': [
            Object(Matching, 'Jones3D/Display/JonesConsole.c',
                   original_filename=r'C:\Jones3D\Product\Indy3D\Display\JonesConsole.c'),
            Object(Matching, 'Jones3D/Display/JonesDisplay.c',
                   original_filename=r'C:\Jones3D\Product\Indy3D\Display\JonesDisplay.c'),
            Object(Matching, 'Jones3D/Display/JonesHud.c',
                   original_filename=r'C:\Jones3D\Product\Indy3D\Display\JonesHud.c'),
            Object(Matching, 'Jones3D/Display/jonesConfig.c',
                   original_filename=r'C:\Jones3D\Product\Indy3D\Display\jonesConfig.c'),
            Object(Matching, 'Jones3D/Gui/JonesDialog.c',
                   original_filename=r'C:\Jones3D\Product\Indy3D\Gui\JonesDialog.c'),
            Object(Matching, 'Jones3D/Main/JonesFile.c',
                   original_filename=r'C:\Jones3D\Product\Indy3D\Main\JonesFile.c'),
            Object(Matching, 'Jones3D/Main/JonesMain.c',
                   original_filename=r'C:\Jones3D\Product\Indy3D\Main\JonesMain.c'),
            Object(Matching, 'Jones3D/Main/JonesDevDialog.c'),
            Object(Matching, 'Jones3D/Main/jonesString.c',
                   original_filename=r'C:\Jones3D\Product\Indy3D\Main\jonesString.c'),
            Object(Matching, 'Jones3D/Play/JonesControl.c'),
            Object(Matching, 'Jones3D/Play/jonesCog.c',
                   original_filename=r'C:\Jones3D\Product\Indy3D\Play\jonesCog.c'),
            Object(Matching, 'Jones3D/Play/jonesInventory.c',
                   original_filename=r'C:\Jones3D\Product\Indy3D\Play\jonesInventory.c'),
        ]},
        {'lib': 'rdroid', **game, 'objects': [
            Object(Matching, 'Libs/rdroid/Engine/rdCamera.c',
                   original_filename=r'C:\Jones3D\Libs\rdroid\Engine\rdCamera.c'),
            Object(Matching, 'Libs/rdroid/Engine/rdCanvas.c',
                   original_filename=r'C:\Jones3D\Libs\rdroid\Engine\rdCanvas.c'),
            Object(Matching, 'Libs/rdroid/Engine/rdClip.c',
                   original_filename=r'C:\Jones3D\Libs\rdroid\Engine\rdClip.c'),
            Object(Matching, 'Libs/rdroid/Engine/rdClipFace.c'),
            Object(Matching, 'Libs/rdroid/Engine/rdKeyframe.c',
                   original_filename=r'C:\Jones3D\Libs\rdroid\Engine\rdKeyframe.c'),
            Object(Matching, 'Libs/rdroid/Engine/rdLight.c'),
            Object(Matching, 'Libs/rdroid/Engine/rdMaterial.c',
                   original_filename=r'C:\Jones3D\Libs\rdroid\Engine\rdMaterial.c'),
            Object(Matching, 'Libs/rdroid/Engine/rdPuppet.c',
                   original_filename=r'C:\Jones3D\Libs\rdroid\Engine\rdPuppet.c'),
            Object(Matching, 'Libs/rdroid/Engine/rdQClip.c',
                   original_filename=r'C:\Jones3D\Libs\rdroid\Engine\rdQClip.c'),
            Object(Matching, 'Libs/rdroid/Engine/rdThing.c',
                   original_filename=r'C:\Jones3D\Libs\rdroid\Engine\rdThing.c'),
            Object(Matching, 'Libs/rdroid/Main/rdroid.c', original_filename=r'C:\Jones3D\Libs\rdroid\Main\rdroid.c'),
            Object(Matching, 'Libs/rdroid/Math/rdMath.c'),
            Object(Matching, 'Libs/rdroid/Math/rdMatrix.c',
                   original_filename=r'C:\Jones3D\Libs\rdroid\Math\rdMatrix.c'),
            Object(Matching, 'Libs/rdroid/Math/rdVector.c',
                   original_filename=r'C:\Jones3D\Libs\rdroid\Math\rdVector.c'),
            Object(Matching, 'Libs/rdroid/Primitives/rdFont.c',
                   original_filename=r'C:\Jones3D\Libs\rdroid\Primitives\rdFont.c'),
            Object(Matching, 'Libs/rdroid/Primitives/rdModel3.c',
                   original_filename=r'C:\Jones3D\Libs\rdroid\Primitives\rdModel3.c'),
            Object(Matching, 'Libs/rdroid/Primitives/rdParticle.c',
                   original_filename=r'C:\Jones3D\Libs\rdroid\Primitives\rdParticle.c'),
            Object(Matching, 'Libs/rdroid/Primitives/rdPolyline.c',
                   original_filename=r'C:\Jones3D\Libs\rdroid\Primitives\rdPolyline.c'),
            Object(Matching, 'Libs/rdroid/Primitives/rdPrimit2.c'),
            Object(Matching, 'Libs/rdroid/Primitives/rdPrimit3.c'),
            Object(Matching, 'Libs/rdroid/Primitives/rdSprite.c',
                   original_filename=r'C:\Jones3D\Libs\rdroid\Primitives\rdSprite.c'),
            Object(Matching, 'Libs/rdroid/Primitives/rdWallpaper.c',
                   original_filename=r'C:\Jones3D\Libs\rdroid\Primitives\rdWallpaper.c'),
            Object(Matching, 'Libs/rdroid/Raster/rdCache.c',
                   original_filename=r'C:\Jones3D\Libs\rdroid\Raster\rdCache.c'),
            Object(Matching, 'Libs/rdroid/Raster/rdFace.c'),
        ]},
        {'lib': 'sith', **game, 'objects': [
            Object(Matching, 'Libs/sith/AI/sithAI.c', original_filename=r'C:\Jones3D\Libs\sith\Ai\sithAi.c'),
            Object(Matching, 'Libs/sith/AI/sithAIAwareness.c',
                   original_filename=r'C:\Jones3D\Libs\sith\Ai\sithAIAwareness.c'),
            Object(Matching, 'Libs/sith/AI/sithAIClass.c',
                   original_filename=r'C:\Jones3D\Libs\sith\Ai\sithAIClass.c'),
            Object(Matching, 'Libs/sith/AI/sithAIInstinct.c',
                   original_filename=r'C:\Jones3D\Libs\sith\Ai\sithAIInstinct.c'),
            Object(Matching, 'Libs/sith/AI/sithAIMove.c', original_filename=r'C:\Jones3D\Libs\sith\Ai\sithAiMove.c'),
            Object(Matching, 'Libs/sith/AI/sithAIUtil.c', original_filename=r'C:\Jones3D\Libs\sith\Ai\sithAIUtil.c'),
            Object(Matching, 'Libs/sith/Cog/sithCog.c', original_filename=r'C:\Jones3D\Libs\sith\Cog\sithCog.c'),
            Object(Matching, 'Libs/sith/Cog/sithCogExec.c',
                   original_filename=r'C:\Jones3D\Libs\sith\Cog\sithCogExec.c'),
            Object(Matching, 'Libs/sith/Cog/sithCogFlex.c'),
            Object(Matching, 'Libs/sith/Cog/sithCogFunction.c',
                   original_filename=r'C:\Jones3D\Libs\sith\Cog\sithCogFunction.c'),
            Object(Matching, 'Libs/sith/Cog/sithCogFunctionAI.c',
                   original_filename=r'C:\Jones3D\Libs\sith\Cog\sithCogFunctionAI.c'),
            Object(Matching, 'Libs/sith/Cog/sithCogFunctionPlayer.c',
                   original_filename=r'C:\Jones3D\Libs\sith\Cog\sithCogFunctionPlayer.c'),
            Object(Matching, 'Libs/sith/Cog/sithCogFunctionSector.c',
                   original_filename=r'C:\Jones3D\Libs\sith\Cog\sithCogFunctionSector.c'),
            Object(Matching, 'Libs/sith/Cog/sithCogFunctionSound.c',
                   original_filename=r'C:\Jones3D\Libs\sith\Cog\sithCogFunctionSound.c'),
            Object(Matching, 'Libs/sith/Cog/sithCogFunctionSurface.c',
                   original_filename=r'C:\Jones3D\Libs\sith\Cog\sithCogFunctionSurface.c'),
            Object(Matching, 'Libs/sith/Cog/sithCogFunctionThing.c',
                   original_filename=r'C:\Jones3D\Libs\sith\Cog\sithCogFunctionThing.c'),
            Object(Matching, 'Libs/sith/Cog/sithCogParse.c',
                   original_filename=r'C:\Jones3D\Libs\sith\Cog\sithCogParse.c'),
            Object(Matching, 'Libs/sith/Cog/sithCogYacc.c',
                   original_filename=r'C:\Jones3D\Libs\sith\Cog\sithCogYacc.c'),
            Object(Matching, 'Libs/sith/Devices/sithComm.c',
                   original_filename=r'C:\Jones3D\Libs\sith\Devices\sithComm.c'),
            Object(Matching, 'Libs/sith/Devices/sithConsole.c',
                   original_filename=r'C:\Jones3D\Libs\sith\Devices\sithConsole.c'),
            Object(Matching, 'Libs/sith/Devices/sithControl.c',
                   original_filename=r'C:\Jones3D\Libs\sith\Devices\sithControl.c'),
            Object(Matching, 'Libs/sith/Devices/sithSound.c',
                   original_filename=r'C:\Jones3D\Libs\sith\Devices\sithSound.c'),
            Object(Matching, 'Libs/sith/Devices/sithSoundMixer.c',
                   original_filename=r'C:\Jones3D\Libs\sith\Devices\sithSoundMixer.c'),
            Object(Matching, 'Libs/sith/Dss/sithDSS.c', original_filename=r'C:\Jones3D\Libs\sith\Dss\sithDSS.c'),
            Object(Matching, 'Libs/sith/Dss/sithDSSCog.c',
                   original_filename=r'C:\Jones3D\Libs\sith\Dss\sithDSSCog.c'),
            Object(Matching, 'Libs/sith/Dss/sithDSSThing.c',
                   original_filename=r'C:\Jones3D\Libs\sith\Dss\sithDSSThing.c'),
            Object(Matching, 'Libs/sith/Dss/sithGamesave.c',
                   original_filename=r'C:\Jones3D\Libs\sith\Dss\sithGamesave.c'),
            Object(Matching, 'Libs/sith/Dss/sithMulti.c', original_filename=r'C:\Jones3D\Libs\sith\Dss\sithMulti.c'),
            Object(Matching, 'Libs/sith/Engine/sithAnimate.c',
                   original_filename=r'C:\Jones3D\Libs\sith\Engine\sithAnimate.c'),
            Object(Matching, 'Libs/sith/Engine/sithCamera.c',
                   original_filename=r'C:\Jones3D\Libs\sith\Engine\sithCamera.c'),
            Object(Matching, 'Libs/sith/Engine/sithCollision.c',
                   original_filename=r'C:\Jones3D\Libs\sith\Engine\sithCollision.c'),
            Object(Matching, 'Libs/sith/Engine/sithIntersect.c',
                   original_filename=r'C:\Jones3D\Libs\sith\Engine\sithIntersect.c'),
            Object(Matching, 'Libs/sith/Engine/sithParticle.c',
                   original_filename=r'C:\Jones3D\Libs\sith\Engine\sithParticle.c'),
            Object(Matching, 'Libs/sith/Engine/sithPathMove.c',
                   original_filename=r'C:\Jones3D\Libs\sith\Engine\sithPathMove.c'),
            Object(Matching, 'Libs/sith/Engine/sithPhysics.c',
                   original_filename=r'C:\Jones3D\Libs\sith\Engine\sithPhysics.c'),
            Object(Matching, 'Libs/sith/Engine/sithPuppet.c',
                   original_filename=r'C:\Jones3D\Libs\sith\Engine\sithPuppet.c'),
            Object(Matching, 'Libs/sith/Engine/sithRender.c',
                   original_filename=r'C:\Jones3D\Libs\sith\Engine\sithRender.c'),
            Object(Matching, 'Libs/sith/Engine/sithRenderSky.c'),
            Object(Matching, 'Libs/sith/Engine/sithShadow.c',
                   original_filename=r'C:\Jones3D\Libs\sith\Engine\sithShadow.c'),
            Object(Matching, 'Libs/sith/Gameplay/sithEvent.c',
                   original_filename=r'C:\Jones3D\Libs\sith\Gameplay\sithEvent.c'),
            Object(Matching, 'Libs/sith/Gameplay/sithFX.c',
                   original_filename=r'C:\Jones3D\Libs\sith\Gameplay\sithFX.c'),
            Object(Matching, 'Libs/sith/Gameplay/sithInventory.c',
                   original_filename=r'C:\Jones3D\Libs\sith\Gameplay\sithInventory.c'),
            Object(Matching, 'Libs/sith/Gameplay/sithOverlayMap.c',
                   original_filename=r'C:\Jones3D\Libs\sith\Gameplay\sithOverlayMap.c'),
            Object(Matching, 'Libs/sith/Gameplay/sithPlayer.c',
                   original_filename=r'C:\Jones3D\Libs\sith\Gameplay\sithPlayer.c'),
            Object(Matching, 'Libs/sith/Gameplay/sithPlayerActions.c',
                   original_filename=r'C:\Jones3D\Libs\sith\Gameplay\sithPlayerActions.c'),
            Object(Matching, 'Libs/sith/Gameplay/sithPlayerControls.c',
                   original_filename=r'C:\Jones3D\Libs\sith\Gameplay\sithPlayerControls.c'),
            Object(Matching, 'Libs/sith/Gameplay/sithTime.c',
                   original_filename=r'C:\Jones3D\Libs\sith\Gameplay\sithTime.c'),
            Object(Matching, 'Libs/sith/Gameplay/sithVehicleControls.c',
                   original_filename=r'C:\Jones3D\Libs\sith\Gameplay\sithVehicleControls.c'),
            Object(Matching, 'Libs/sith/Gameplay/sithWhip.c',
                   original_filename=r'C:\Jones3D\Libs\sith\Gameplay\sithWhip.c'),
            Object(Matching, 'Libs/sith/Main/sithCommand.c',
                   original_filename=r'C:\Jones3D\Libs\sith\Main\sithCommand.c'),
            Object(Matching, 'Libs/sith/Main/sithMain.c', original_filename=r'C:\Jones3D\Libs\sith\Main\sithMain.c'),
            Object(Matching, 'Libs/sith/Main/sithString.c',
                   original_filename=r'C:\Jones3D\Libs\sith\Main\sithString.c'),
            Object(Matching, 'Libs/sith/World/sithActor.c',
                   original_filename=r'C:\Jones3D\Libs\sith\World\sithActor.c'),
            Object(Matching, 'Libs/sith/World/sithExplosion.c',
                   original_filename=r'C:\Jones3D\Libs\sith\World\sithExplosion.c'),
            Object(Matching, 'Libs/sith/World/sithItem.c',
                   original_filename=r'C:\Jones3D\Libs\sith\World\sithItem.c'),
            Object(Matching, 'Libs/sith/World/sithMaterial.c',
                   original_filename=r'C:\Jones3D\Libs\sith\World\sithMaterial.c'),
            Object(Matching, 'Libs/sith/World/sithModel.c',
                   original_filename=r'C:\Jones3D\Libs\sith\World\sithModel.c'),
            Object(Matching, 'Libs/sith/World/sithPVS.c', original_filename=r'C:\Jones3D\Libs\sith\World\sithPVS.c'),
            Object(Matching, 'Libs/sith/World/sithSector.c',
                   original_filename=r'C:\Jones3D\Libs\sith\World\sithSector.c'),
            Object(Matching, 'Libs/sith/World/sithSoundClass.c',
                   original_filename=r'C:\Jones3D\Libs\sith\World\sithSoundClass.c'),
            Object(Matching, 'Libs/sith/World/sithSprite.c',
                   original_filename=r'C:\Jones3D\Libs\sith\World\sithSprite.c'),
            Object(Matching, 'Libs/sith/World/sithSurface.c',
                   original_filename=r'C:\Jones3D\Libs\sith\World\sithSurface.c'),
            Object(Matching, 'Libs/sith/World/sithTemplate.c',
                   original_filename=r'C:\Jones3D\Libs\sith\World\sithTemplate.c'),
            Object(Matching, 'Libs/sith/World/sithThing.c',
                   original_filename=r'C:\Jones3D\Libs\sith\World\sithThing.c'),
            Object(Matching, 'Libs/sith/World/sithVoice.c',
                   original_filename=r'C:\Jones3D\Libs\sith\World\sithVoice.c'),
            Object(Matching, 'Libs/sith/World/sithWeapon.c',
                   original_filename=r'C:\Jones3D\Libs\sith\World\sithWeapon.c'),
            Object(Matching, 'Libs/sith/World/sithWorld.c',
                   original_filename=r'C:\Jones3D\Libs\sith\World\sithWorld.c'),
        ]},
        {'lib': 'sound', **game, 'objects': [
            Object(Matching, 'Libs/sound/AudioLib.c'),
            Object(Matching, 'Libs/sound/Driver.c'),
            Object(Matching, 'Libs/sound/Sound.c'),
        ]},
        {'lib': 'std', **game, 'objects': [
            Object(Matching, 'Libs/std/General/std.c'),
            Object(Matching, 'Libs/std/General/stdBmp.c', original_filename=r'C:\Jones3D\Libs\std\General\stdBmp.c'),
            Object(Matching, 'Libs/std/General/stdCircBuf.c',
                   original_filename=r'C:\Jones3D\Libs\std\General\stdCircBuf.c'),
            Object(Matching, 'Libs/std/General/stdColor.c',
                   original_filename=r'C:\Jones3D\Libs\std\General\stdColor.c'),
            Object(Matching, 'Libs/std/General/stdConffile.c',
                   original_filename=r'C:\Jones3D\Libs\std\General\stdConffile.c'),
            # VC5, not VC6: VC5 pools the 1.0f that Reset stores as an immediate, and retail has exactly
            # that 4-byte pool at stdEffect's link position (0x00509784); .text is identical under both.
            Object(Matching, 'Libs/std/General/stdEffect.c',
                   original_filename=r'C:\Jones3D\Libs\std\General\stdEffect.c'),
            Object(Matching, 'Libs/std/General/stdFileUtil.c',
                   original_filename=r'C:\Jones3D\Libs\std\General\stdFileUtil.c'),
            Object(Matching, 'Libs/std/General/stdFnames.c'),
            Object(Matching, 'Libs/std/General/stdHashtbl.c',
                   original_filename=r'C:\Jones3D\Libs\std\General\stdHashtbl.c'),
            Object(Matching, 'Libs/std/General/stdLinkList.c',
                   original_filename=r'C:\Jones3D\Libs\std\General\stdLinklist.c'),
            Object(Matching, 'Libs/std/General/stdMath.c'),
            Object(Matching, 'Libs/std/General/stdMemory.c',
                   original_filename=r'C:\Jones3D\Libs\std\General\stdMemory.c'),
            Object(Matching, 'Libs/std/General/stdPlatform.c'),
            Object(Matching, 'Libs/std/General/stdStrTable.c',
                   original_filename=r'C:\Jones3D\Libs\std\General\stdStrTable.c'),
            Object(Matching, 'Libs/std/General/stdUtil.c',
                   original_filename=r'C:\Jones3D\Libs\std\General\stdUtil.c'),
            Object(Matching, 'Libs/std/Win95/std3D.c', original_filename=r'C:\Jones3D\Libs\std\Win95\std3D.c'),
            Object(Matching, 'Libs/std/Win95/stdComm.c', original_filename=r'C:\Jones3D\Libs\std\Win95\stdComm.c'),
            Object(Matching, 'Libs/std/Win95/stdConsole.c'),
            Object(Matching, 'Libs/std/Win95/stdControl.c',
                   original_filename=r'C:\Jones3D\Libs\std\Win95\stdControl.c'),
            Object(Matching, 'Libs/std/Win95/stdDisplay.c',
                   original_filename=r'C:\Jones3D\Libs\std\Win95\stdDisplay.c'),
            Object(Matching, 'Libs/std/Win95/stdGob.c', original_filename=r'C:\Jones3D\Libs\std\Win95\stdGob.c'),
            # Code-less: the DirectX GUIDs defined under INITGUID (no original filename is evidenced).
            Object(Matching, 'Libs/std/Win95/stdGuid.c'),
            Object(Matching, 'Libs/std/Win95/stdWin95.c', original_filename=r'C:\Jones3D\Libs\std\Win95\stdWin95.c'),
        ]},
        {'lib': 'w32util', **game, 'objects': [
            Object(Matching, 'Libs/w32util/wuRegistry.c', original_filename=r'C:\Jones3D\Libs\w32util\wuRegistry.c'),
        ]},
        {'lib': 'wkernel', **game, 'objects': [
            Object(Matching, 'Libs/wkernel/wkernel.c'),
        ]},
        # Visual C++ 4.20 (stdUtil.md): a separate, precompiled object (retail 0x004E09A0, debug build 0x0052A7A0,
        # byte-identical and optimised in both). Its flags are fixed rather than cflags_base because the debug
        # build links the same optimised object; they are the release set (measured inert here). File name and
        # path are inferred.
        {'lib': 'strcmp', 'progress_category': 'std', 'compiler': 'VC4_20',
         'cflags': ['/O2', '/Ob1', '/Oi', '/Ot', '/Oy', '/G5', '/ML', '/Gf', '/Gy', '/DNDEBUG', '/W3'], 'objects': [
            Object(Matching, 'Libs/std/General/stdUtil_StrCmp.c'),
        ]},
        # Intel C/C++ 4.0: the Katmai rdModel3K library (docs/crt.md, rdModel3K.md). Retail's assertion text names
        # the source C:\Jones3D\Libs\Katmai\source\rdModel3K.c; the flag set is measured (-QxM against -QxK).
        {'lib': 'katmai', 'progress_category': 'rdroid', 'compiler': 'ICL40', 'cflags': ['-O2', '-QxM'], 'objects': [
            Object(Matching, 'Libs/rdroid/Primitives/rdModel3K.c',
                   original_filename=r'C:\Jones3D\Libs\Katmai\source\rdModel3K.c'),
            # The second Intel object of the range (probes + availability flags, retail 0x004E2D80-0x004E2F30):
            # its SEH scope tables land behind rdModel3K.obj's string COMDATs in .data1. Name and boundary are
            # inferred (docs/crt.md, "rdModel3K split").
            Object(Matching, 'Libs/rdroid/Primitives/rdModel3K_cpu.c'),
        ]},
        # Intel C/C++ 4.0's libm, which retail linked in place of LIBC's floor/ceil/modf (docs/crt.md). No archive
        # found holds retail's build and Intel's C for these routines is not available, so they are reconstructed
        # C counted with the game TUs (owner ruling 2026-10-17: unfinished functions, a source form not yet
        # found), never `library`. Separate objects with separate flags, as in retail: floor/ceil use floor.obu's
        # recorded build line (its `.comment`), modf a no-aligned-prologue build (config/indy3d-v1.0/modf.md).
        {'lib': 'intel', 'compiler': 'ICL40', 'objects': [
            Object(NonMatching, 'Libs/intel/libm/floor.c', cflags=['-w', '-Zl', '-Di386', '-QIfdiv-', '-Op']),
            Object(NonMatching, 'Libs/intel/libm/modf.c', cflags=['-O2', '-QxM', '-Qsfalign-']),
        ]},
        # zlib 1.0.9 (Jean-loup Gailly and Mark Adler; its licence permits redistribution of the source), built
        # by the game's authors with VC6 RTM `/O2 /Zp1 /Gy` and linked as a library: only the members retail's
        # link pulled in, in link (reference) order, so each object owns several separate ranges (docs/crt.md).
        {'lib': 'zlib', **vc6_insane, 'objects': [
            Object(Matching, 'Libs/zlib/gzio.c'),
            Object(Matching, 'Libs/zlib/inflate.c'),
            Object(Matching, 'Libs/zlib/deflate.c'),
            Object(Matching, 'Libs/zlib/crc32.c'),
            Object(Matching, 'Libs/zlib/zutil.c'),
            Object(Matching, 'Libs/zlib/infblock.c'),
            Object(Matching, 'Libs/zlib/adler32.c'),
            Object(Matching, 'Libs/zlib/trees.c'),
            Object(Matching, 'Libs/zlib/infcodes.c'),
            Object(Matching, 'Libs/zlib/inftrees.c'),
            Object(Matching, 'Libs/zlib/infutil.c'),
            Object(Matching, 'Libs/zlib/inffast.c'),
        ]},
        # SmushPlay glue and the LucasArts SMUSH/INSANE movie-player library (docs/crt.md, "INSANE"): one object per
        # `LECMODULE=insane_*` registration in retail link order, VC6 RTM `/O2 /Zp1 /Gy` (config/indy3d-v1.0/insane.md
        # records the evidence). The six MASM 6.13 objects sit in that order too, the only per-object compiler here.
        {'lib': 'smush', **vc6_insane, 'objects': [
            Object(Matching, 'Libs/smush/SmushPlay.c'),
            Object(Matching, 'Libs/smush/insane/Image/Readauto.c',
                   original_filename=r'E:\smush\INSANE\SOURCE\Image\Readauto.c'),
            Object(Matching, 'Libs/smush/insane/Animate/Pcplay.c',
                   original_filename=r'E:\smush\INSANE\SOURCE\ANIMATE\Pcplay.c'),
            Object(Matching, 'Libs/smush/insane/Animate/Frame.c',
                   original_filename=r'E:\smush\INSANE\SOURCE\ANIMATE\Frame.c'),
            Object(Matching, 'Libs/smush/insane/File/Cdstream.c',
                   original_filename=r'E:\smush\INSANE\SOURCE\File\Cdstream.c'),
            Object(Matching, 'Libs/smush/insane/Sound/Engine.c',
                   original_filename=r'E:\smush\INSANE\SOURCE\Sound\Engine.c'),
            Object(Matching, 'Libs/smush/insane/Sound/Dsnd.c'),
            Object(Matching, 'Libs/smush/insane/Flobject/D_wave.c',
                   original_filename=r'E:\smush\INSANE\SOURCE\Flobject\D_wave.c'),
            Object(Matching, 'Libs/smush/insane/Flobject/D_bloc16.c',
                   original_filename=r'E:\smush\INSANE\SOURCE\Flobject\D_bloc16.c'),
            Object(Matching, 'Libs/smush/insane/File/Stdfile.c'),
            Object(Matching, 'Libs/smush/insane/File/Zfilio/Zfilio.c',
                   original_filename=r'E:\smush\INSANE\SOURCE\File\Zfilio\Zfilio.c'),
            Object(Matching, 'Libs/smush/insane/File/Stdio.c'),
            Object(Matching, 'Libs/smush/insane/Bitmap/Bitmap.c',
                   original_filename=r'E:\smush\insane\source\Bitmap\Bitmap.c'),
            Object(Matching, 'Libs/smush/insane/Timer/Timer.c'),
            Object(Matching, 'Libs/smush/insane/Memory/Memory.c'),
            Object(Matching, 'Libs/smush/insane/Lec/LecShare.c'),
            Object(Matching, 'Libs/smush/insane/File/Framestream.c'),
            # INSANE's MASM 6.13 objects (docs/crt.md, "MASM 6.13"); file names are inferred.
            Object(Matching, 'Libs/smush/insane/Sound/MixerAsm.asm', compiler='ML613', cflags=[]),
            Object(Matching, 'Libs/smush/insane/Sound/Mixer.c',
                   original_filename=r'E:\smush\INSANE\SOURCE\Sound\Mixer.c'),
            Object(Matching, 'Libs/smush/insane/Flobject/Bloc16Asm.asm', compiler='ML613', cflags=[]),
            Object(Matching, 'Libs/smush/insane/Decode/Decode.c'),
            Object(Matching, 'Libs/smush/insane/Util/List.c'),
            Object(Matching, 'Libs/smush/insane/Bitmap/Blit.c'),
            Object(Matching, 'Libs/smush/insane/Bitmap/MemCopy.asm', compiler='ML613', cflags=[]),
            Object(Matching, 'Libs/smush/insane/Bitmap/MemFill.asm', compiler='ML613', cflags=[]),
            Object(Matching, 'Libs/smush/insane/Decode/DecodeAsm.asm', compiler='ML613', cflags=[]),
            Object(Matching, 'Libs/smush/insane/Bitmap/BlitAsm.asm', compiler='ML613', cflags=[]),
        ]},
    ]
    config.progress_categories = [
        ProgressCategory('Jones3D', 'Jones3D'),
        ProgressCategory('rdroid', 'rdroid'),
        ProgressCategory('intel', 'intel (Intel C/C++ 4.0 libm helpers)'),
        ProgressCategory('smush', 'smush (SmushPlay + INSANE, VC6 RTM)'),
        ProgressCategory('zlib', 'zlib 1.0.9 (VC6 RTM, from the published source)'),
        ProgressCategory('sith', 'sith'),
        ProgressCategory('sound', 'sound'),
        ProgressCategory('std', 'std'),
        ProgressCategory('w32util', 'w32util'),
        ProgressCategory('wkernel', 'wkernel'),
        # Statically linked library members matched from the shipped archives
        # (config/indy3d-v1.0/libraries.yml, docs/crt.md); not game source.
        ProgressCategory('library', 'library (statically linked, from the shipped archives)'),
    ]

    # PE headers and the import table, generated from config/<version>/image/ and the pinned import
    # libraries (Scripts/pe_generate.py); config.yml's image_sources replaces their preserved fragments.
    if (config.root / 'config' / args.version / 'image').is_dir():
        import sys
        from Scripts import pe_generate
        from tools.project import command
        generated = args.build_dir / args.version / 'generated'
        config.custom_build_rules = [{
            'name': 'image_fragment', 'description': 'GENERATE $out',
            'command': command([sys.executable, '-m', 'Scripts.pe_generate', '--version', args.version]) +
                       ' $generator --output $out'}]
        config.custom_build_steps = {'pre-compile': [
            {'rule': 'image_fragment', 'outputs': str(generated / name), 'implicit': pe_generate.inputs(args.version),
             'variables': {'generator': shlex.join(generator)}}
            for name, generator in pe_generate.fragments(args.version).items()]}
        # The resource section, compiled from Jones3D/Indy3D.rc. Its icons are game artwork and stay out
        # of the repository: a declared user-supplied asset, extracted from the user's own executable
        # (config.yml `object`, checked against its SHA-256) and reported apart from every progress figure.
        assets = args.build_dir / args.version / 'assets'
        from tools.config import read_yaml
        version_config = read_yaml(config.root / 'config' / args.version / 'config.yml')
        config.custom_build_rules.append({
            'name': 'user_assets', 'description': 'EXTRACT user-supplied icons from $in',
            'command': command([sys.executable, '-m', 'Scripts.pe_rsrc', '--extract-icons']) +
                       ' $in --sha256 $sha256 --icons $icons --output $out'})
        config.custom_build_steps['pre-compile'] += [
            {'rule': 'user_assets', 'inputs': version_config['object'],
             'outputs': str(assets / 'icons.stamp'), 'implicit': ['Scripts/pe_rsrc.py'],
             'variables': {'sha256': version_config['pe']['sha256'], 'icons': str(assets / 'icons')}},
            {'rule': 'image_fragment', 'outputs': str(generated / 'resources.bin'),
             'implicit_outputs': str(generated / 'resources.assets.json'),
             'implicit': [*pe_generate.resource_inputs(args.version), str(assets / 'icons.stamp')],
             'variables': {'generator': shlex.join(['resources', '--icons', str(assets / 'icons'),
                                                    '--assets', str(generated / 'resources.assets.json')])}}]
    return args.mode, config


def make_build(argv=None):
    """The configuration of the requested version plus, for the default version, the other configured
    versions (each built from the same build.ninja under its target prefix)."""
    mode, config = make_config(argv)
    argv = list(argv if argv is not None else sys.argv[1:])
    if config.version == default_version() and '--only-version' not in config.configure_args:
        for version in VERSIONS:
            if version == config.version or not (config.root / 'config' / version / 'config.yml').is_file():
                continue
            # the same options, this version, its target prefix
            _, extra = make_config([a for a in argv if a not in ('configure', 'progress')] +
                                   ['-v', version, '--only-version'])
            extra.target_prefix = TARGET_PREFIXES[version]
            extra.rule_suffix = '_' + version.rsplit('-', 1)[-1]
            config.extra_versions.append(extra)
    return mode, config


def main():
    mode, config = make_build()
    try:
        if mode == 'configure':
            generate_build(config)
        else:
            calculate_progress(config)
    except (ValueError, OSError, KeyError, subprocess.CalledProcessError) as exc:
        raise SystemExit(f'error: {exc}') from exc


if __name__ == '__main__':
    main()
