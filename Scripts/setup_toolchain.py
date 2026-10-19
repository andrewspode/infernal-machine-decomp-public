#!/usr/bin/env python3
"""Install pinned MSVC toolchains, Intel C/C++ 4.0, MASM 6.13, wibo (release and heap-model), objdiff, and DirectX 6.1 locally."""
import argparse
import hashlib
import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys
import tempfile
import urllib.request
import venv

ROOT = Path(__file__).resolve().parent.parent
# VC6 RTM (kept: its CRT libraries and Rich-header stamp are still part of the linking/checking
# evidence, docs/compiler-default.md) and VC5 SP3 (the project-wide compiler)
# are the MSVC toolchains this script installs for the game (plus Visual C++ 4.20 for one
# precompiled object, below). VC5 RTM, VC5 SP1 and VC6 SP3 are
# ruled out and no longer set up here; their pinned commits are recorded in git history
# (see the commit that removed them) if they're ever needed again.
MSVC_COMMIT = '93d5bbe2582aa5b78a3a8dddd9498530fdd74061'
VC5_SP3_COMMIT = '259a03f0bc863de5baf657ec064cb60cb20a2cdc'
# Visual C++ 4.20 (C1/C2 10.20.6166): the compiler of the separate stdUtil_StrCmp object
# (config/indy3d-v1.0/stdUtil.md). Its drivers import MSVCRT40.DLL, which wibo does not provide, so the
# VC5 SP3 package's redistributable MSVCRT40.DLL (a forwarder) and MSVCRT.DLL are copied beside them.
VC420_COMMIT = 'b42c244f0a83ba15ba2ffb62b0dc240d7b2dea50'
VC420_SHA256 = {
    'CL.EXE': 'c5bf7ad84482e8a54d5753fcbd3e648d8a1192f5ca8b8cf1f5d23b651750585f',
    'C1.EXE': 'c5a62937d806fbd8663b05f15bd02670a43bdf983a50ee4080bcfd90a7643b90',
    'C2.EXE': '674fb9e410481378c6980c3f21914e513128c85001e8337aca73b587b6273ae9',
}
VC420_RUNTIME_SHA256 = {
    'msvcrt40.dll': 'ab55a2de2b6faf3daacd3e69473d385ceaead8033f7c79beb6bbf802f230f030',
    'msvcrt.dll': 'e390b0529fb9ee1214bb68302a4658b578134d92ab76bc5eb48f3badd843d210',
}
# Intel C/C++ 4.0 (evaluation build 99100): the compiler of rdModel3K and the libm helpers (docs/crt.md).
# The setup repository holds no Intel software; its setup_icl40.py downloads the 5 MB compeval.exe
# installer from archive.org (SHA-256 checked there), unpacks it without Wine, and patch_trial.py writes
# icl_notrial.exe, the driver with the 14-day trial check disabled.
ICL_SETUP_URL = 'https://github.com/andrewspode/intel-cc-4.0-setup.git'
ICL_SETUP_COMMIT = '5adedde9ddf0a539f5d8bd5bc6227ebd7b2d29f6'
ICL_COMPEVAL_SHA256 = '931fecd143c7532dc944bad4d2a7a1aadf76d143c08161eec25ba8801131746e'
ICL_NOTRIAL_SHA256 = 'fed5dc869b8d1053778586684fb3bfe4091d22399f2d8aa9028dc538ba64ad81'
# Microsoft Macro Assembler 6.13.7299: the assembler of INSANE's six MASM objects (retail's Rich header lists six
# Masm613 7299 objects; docs/crt.md, "MASM 6.13"). Microsoft shipped 6.13 only as a patch (ML613.EXE, an RTPatch
# file applied by its own DOS PATCH.EXE) to an installed MASM 6.11/6.11a/6.11d/6.12. Both inputs are period
# packages on archive.org: the Windows 98 SE DDK (1999) carries ML.EXE 6.11d in BINS_DDK.CAB, and the archived
# MASM 6.1 set carries Microsoft's ML613 patch (UPDATE.613, a RAR self-extractor). The patch program runs under
# emu2 (a small DOS emulator, built from a pinned commit); 7-Zip (the pinned official release, which reads RAR
# and CAB) unpacks the archives. Every input and the patched ML.EXE/ML.ERR are checked by SHA-256.
MASM_DDK_URL = 'https://archive.org/download/win98se_ddk.exe/win98se_ddk.exe'
MASM_DDK_SHA256 = '08318354cbac0b17d2e976a70b901ceb336947e76e4af8c869efdc6327a6a5dc'
MASM_PATCH_URL = 'https://archive.org/download/masm-6.13/MASM%206.13.zip'
MASM_PATCH_SHA256 = '61ab1f2e14622a3721097e32aaadc436f8ebd385d361c5944063d7b1fa76c8ce'
MASM_SHA256 = {
    'ml611d.exe': '33455c7e38348ffda7b73eb66b818f0c00e5d8d52f08eae665616e3a5c9c9b8b',
    'ml611d.err': '6d8ca8b5e41c601531639814bf8fa3b6c7db289c83bcfeee5c9fdab79905431f',
    'ml.exe': '3159512d1fb4f8b9e4c9ec4125d06c3e9d9adc5a675715670f4ea705f5472705',
    'ml.err': '5a873a840e93538dca2d92295b0f83b9fbf224bba23bc83453735a2367fe7c02',
}
EMU2_URL = 'https://github.com/dmsc/emu2.git'
EMU2_COMMIT = '9d8698d06e67359bc8b230c2d19d5b1503e60819'
SEVENZIP_SHA256 = {
    'linux-x64': '914c7e20ad5ef8e4d3cf08620ff8894b28fe11b7eb99809d6930870fbe48a281',
    'linux-x86': 'cb5e49caaf761df67add54729553bef89a38071b0c455461452578018625fee5',
    'linux-arm64': 'fbe331697c9417bbc06fc92d3f4576dca6a5a1442fad7ae810304446a9153e2c',
    'mac': '073b6fa8dc5d9adb6f742888d0d75f5767406b503199b9704ccbf61133a21ded',
}
SDK_URL ='https://archive.org/download/dx61sdkimage/dx61sdkimage.exe'
SDK_SHA256 = '4ce3b6737683cb2ee4a59edd1fbc25f73691acdaa3d5b80d7135d2c0b5dd39b1'
# The executables the build reconstructs, by SHA-256: each version's build needs only its own (README.md).
EXECUTABLES = {
    '55fb00c0a2cb30793ff451dc6fd17681c5128aa32419aa711015fd5117d608bf': ('1.2 (official update)', 'orig/Indy3D-1.2.exe'),
    '3fbaf8cd401b4af80967cbe42e3420fb803288b336ebbe72a9a01b6dfd661a53': ('1.0 (original CD)', 'orig/Indy3D.exe'),
}
WIBO_SHA256 = {
    'macos': '79d07d267f8410f6509addec3e19ccf79417ac29cfaf5f431bbcc8700a877fef',
    'x86_64': 'b9ab9f2c5eb014bc79a97f1a2bee9a7ab177d905a7d2f637da5735820021c40c',
    'i686': '1c5fc4623766e120ce293f3af97d1b7c400481ccdde40eb6a43d23233e6f9fcb',
}
OBJDIFF_SHA256 = {
    'macos-arm64': '945bfb2612c243546a2ad9e80d2ea32cd307e186d808ef918baecc0c46bb9c68',
    'macos-x86_64': '06c94322df24cd09a4af7d71e9845e092744f3a0c8df71fe17b81903d8cc7e95',
    'linux-x86_64': '8f032cd27420488710c14ae367cad682cde7fe1181809b8f02ab3540ef9b74c5',
    'linux-i686': 'd22f954edab9d8bff7dca5a202286faecb5befd500e786b0e5c34ff4f9aa94e0',
}


def verify(path, expected):
    digest = hashlib.sha256(path.read_bytes()).hexdigest()
    if digest != expected:
        raise SystemExit(f'Unexpected SHA-256 for {path}: {digest}')


def strip_signature(data):
    """The image without an appended Authenticode signature (Steam's copy of 1.2 is 1.2 plus one): the PE checksum and
    the security directory entry cleared and the file cut where the signature starts. None if there is none."""
    if len(data) < 0x40 or data[:2] != b'MZ':
        return None
    pe = int.from_bytes(data[0x3C:0x40], 'little')
    checksum, security = pe + 24 + 64, pe + 24 + 128          # PE32 optional header fields
    if len(data) < security + 8 or data[pe:pe + 4] != b'PE\0\0':
        return None
    offset = int.from_bytes(data[security:security + 4], 'little')
    size = int.from_bytes(data[security + 4:security + 8], 'little')
    if not offset or offset + size != len(data):
        return None
    out = bytearray(data[:offset])
    out[checksum:checksum + 4] = bytes(4)
    out[security:security + 8] = bytes(8)
    return bytes(out)


def install_executable(path):
    """Copy a supported Indy3D.exe to its orig/ name after checking its SHA-256 (a signed copy is unsigned first)."""
    data = path.read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    note = ''
    if digest not in EXECUTABLES:
        stripped = strip_signature(data)
        if stripped is not None and hashlib.sha256(stripped).hexdigest() in EXECUTABLES:
            data, digest, note = stripped, hashlib.sha256(stripped).hexdigest(), ' (Authenticode signature removed)'
        else:
            raise SystemExit(f'Unexpected SHA-256 for {path}: {digest} (neither 1.0 nor 1.2; see README.md, '
                             '"The original executable")')
    label, name = EXECUTABLES[digest]
    dest = ROOT / name
    dest.parent.mkdir(exist_ok=True)
    if not dest.exists() or dest.read_bytes() != data:
        dest.write_bytes(data)
    print(f'{path}: Indy3D.exe {label}{note} -> {name}')


def download(url, path, checksum):
    if not path.exists():
        path.parent.mkdir(parents=True, exist_ok=True)
        partial = path.with_name(path.name + '.partial')
        print(f'Downloading {url}', flush=True)
        with urllib.request.urlopen(url, timeout=60) as response, partial.open('wb') as output:
            shutil.copyfileobj(response, output)
        verify(partial, checksum)
        partial.replace(path)
    verify(path, checksum)


def install_intel_compiler(parser, installer):
    """Install Intel C/C++ 4.0 into tools/intel-cc-4.0 (idempotent; verified by SHA-256)."""
    tree = ROOT / 'tools/intel-cc-4.0'
    driver = tree / 'Compiler_Bin_Files/icl_notrial.exe'
    if not driver.exists():
        if not any(shutil.which(tool) for tool in ('7zz', '7z', '7za')):
            parser.error('Installing Intel C/C++ 4.0 needs 7-Zip (7z/7zz/7za) on PATH '
                         '(Debian/Ubuntu: apt install 7zip or p7zip-full; macOS: brew install sevenzip).')
        if not shutil.which('unshield'):
            parser.error('Installing Intel C/C++ 4.0 needs unshield (>= 1.4) on PATH '
                         '(Debian/Ubuntu: apt install unshield; macOS: brew install unshield; or build '
                         'https://github.com/twogood/unshield from source).')
        setup = ROOT / 'tools/downloads/intel-cc-4.0-setup'
        if not setup.exists():
            setup.parent.mkdir(parents=True, exist_ok=True)
            subprocess.run(['git', 'clone', '--no-checkout', ICL_SETUP_URL, str(setup)], check=True)
            subprocess.run(['git', '-C', str(setup), 'checkout', '--detach', ICL_SETUP_COMMIT], check=True)
        actual = subprocess.check_output(['git', '-C', str(setup), 'rev-parse', 'HEAD'], text=True).strip()
        if actual != ICL_SETUP_COMMIT:
            parser.error(f'{setup} is at {actual}; expected {ICL_SETUP_COMMIT}. Existing checkout left intact.')
        command = [sys.executable, str(setup / 'setup_icl40.py'), str(tree)]
        if installer:
            installer = installer.expanduser().resolve()
            verify(installer, ICL_COMPEVAL_SHA256)
            command.append(str(installer))
        subprocess.run(command, check=True)
        subprocess.run([sys.executable, str(setup / 'patch_trial.py'), str(tree)], check=True)
    verify(driver, ICL_NOTRIAL_SHA256)


def sevenzip_pinned(parser):
    """The official 7-Zip 24.09 console binary (reads the RAR self-extractor and the DDK cabinets)."""
    system, arch = platform.system(), platform.machine()
    asset = 'mac' if system == 'Darwin' else {'x86_64': 'linux-x64', 'i686': 'linux-x86', 'i386': 'linux-x86',
                                              'aarch64': 'linux-arm64', 'arm64': 'linux-arm64'}.get(arch)
    if asset not in SEVENZIP_SHA256 or system not in ('Darwin', 'Linux'):
        parser.error(f'No pinned 7-Zip binary for {system} {arch}.')
    tool = ROOT / 'build/tools/7zip/7zz'
    if not tool.exists():
        archive = ROOT / f'tools/downloads/7z2409-{asset}.tar.xz'
        download(f'https://github.com/ip7z/7zip/releases/download/24.09/7z2409-{asset}.tar.xz', archive,
                 SEVENZIP_SHA256[asset])
        import tarfile
        with tarfile.open(archive) as tar:
            member = tar.getmember('7zz')
            tool.parent.mkdir(parents=True, exist_ok=True)
            tool.write_bytes(tar.extractfile(member).read())
        tool.chmod(0o755)
    return tool


def install_masm(parser):
    """Install ML.EXE 6.13.7299 into tools/masm613 (idempotent; verified by SHA-256)."""
    tree = ROOT / 'tools/masm613'
    if all((tree / name).exists() for name in ('ml.exe', 'ml.err')):
        for name in ('ml.exe', 'ml.err'):
            verify(tree / name, MASM_SHA256[name])
        return
    if not shutil.which('make') or not (shutil.which('cc') or shutil.which('gcc') or shutil.which('clang')):
        parser.error('Installing MASM 6.13 builds the emu2 DOS emulator: install make and a C compiler.')
    sevenzip = sevenzip_pinned(parser)
    ddk = ROOT / 'tools/downloads/win98se_ddk.exe'
    download(MASM_DDK_URL, ddk, MASM_DDK_SHA256)
    patch = ROOT / 'tools/downloads/masm-6.13.zip'
    download(MASM_PATCH_URL, patch, MASM_PATCH_SHA256)
    emu2 = ROOT / 'build/tools/emu2'
    if not (emu2 / 'emu2').exists():
        if not emu2.exists():
            emu2.parent.mkdir(parents=True, exist_ok=True)
            subprocess.run(['git', 'clone', '--no-checkout', EMU2_URL, str(emu2)], check=True)
        subprocess.run(['git', '-C', str(emu2), 'checkout', '--detach', EMU2_COMMIT], check=True)
        subprocess.run(['make', '-C', str(emu2)], check=True)
    actual = subprocess.check_output(['git', '-C', str(emu2), 'rev-parse', 'HEAD'], text=True).strip()
    if actual != EMU2_COMMIT:
        parser.error(f'{emu2} is at {actual}; expected {EMU2_COMMIT}. Existing checkout left intact.')
    with tempfile.TemporaryDirectory(prefix='infernal-masm-') as temp:
        temp = Path(temp)
        quiet = ['-y', '-bso0', '-bsp0']
        # ML.EXE/ML.ERR 6.11d from the DDK's binaries cabinet.
        subprocess.run([str(sevenzip), 'x', *quiet, '-o' + str(temp / 'ddk'), str(ddk), 'BINS_DDK.CAB'], check=True)
        subprocess.run([str(sevenzip), 'e', *quiet, '-o' + str(temp / 'ddk'), str(temp / 'ddk/BINS_DDK.CAB'),
                        'BIN_WIN98_ML.EXE', 'BIN_WIN98_ML.ERR'], check=True)
        work = temp / 'patch'
        work.mkdir()
        shutil.copyfile(temp / 'ddk/BIN_WIN98_ML.EXE', work / 'ML.EXE')
        shutil.copyfile(temp / 'ddk/BIN_WIN98_ML.ERR', work / 'ML.ERR')
        verify(work / 'ML.EXE', MASM_SHA256['ml611d.exe'])
        verify(work / 'ML.ERR', MASM_SHA256['ml611d.err'])
        # Microsoft's ML613 patch (PATCH.EXE + PATCH.RTP) from the RAR self-extractor in the archived set.
        subprocess.run([str(sevenzip), 'e', *quiet, '-o' + str(temp / 'zip'), str(patch),
                        'MASM 6.13/UPDATE.613/update.613.exe'], check=True)
        subprocess.run([str(sevenzip), 'e', *quiet, '-o' + str(work), str(temp / 'zip/update.613.exe'),
                        'ML613/PATCH.EXE', 'ML613/PATCH.RTP'], check=True)
        subprocess.run([str(emu2 / 'emu2'), 'PATCH.EXE', 'PATCH.RTP'], cwd=work, check=True,
                       stdout=subprocess.DEVNULL)
        tree.mkdir(parents=True, exist_ok=True)
        for name in ('ml.exe', 'ml.err'):
            produced = next(p for p in work.iterdir() if p.name.lower() == name)
            verify(produced, MASM_SHA256[name])
            shutil.copyfile(produced, tree / name)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, action='append', default=[],
                        help='copy and verify an original Indy3D.exe: 1.2 (the official update, or Steam\'s copy) '
                             'to orig/Indy3D-1.2.exe, 1.0 (the original CD) to orig/Indy3D.exe; repeat for both')
    parser.add_argument('--icl-installer', type=Path, metavar='COMPEVAL.EXE',
                        help='use this local copy of the Intel C/C++ 4.0 evaluation installer '
                             '(compeval.exe) instead of downloading it from archive.org')
    args = parser.parse_args()
    os.chdir(ROOT)
    if not shutil.which('git') or not shutil.which('ninja'):
        parser.error('Install git and Ninja first (on macOS: brew install ninja).')

    compiler = ROOT / 'tools/msvc600-8168'
    if not compiler.exists():
        subprocess.run(['git', 'clone', '--no-checkout',
                        'https://github.com/isledecomp/MSVC600-8168.git', str(compiler)], check=True)
        subprocess.run(['git', '-C', str(compiler), 'checkout', '--detach', MSVC_COMMIT], check=True)
    actual = subprocess.check_output(['git', '-C', str(compiler), 'rev-parse', 'HEAD'], text=True).strip()
    if actual != MSVC_COMMIT:
        parser.error(f'{compiler} is at {actual}; expected {MSVC_COMMIT}. Existing checkout left intact.')

    vc5_sp3 = ROOT / 'tools/msvc500sp3-7303'
    if not vc5_sp3.exists():
        subprocess.run(['git', 'clone', '--no-checkout',
                        'https://github.com/archaic-msvc/msvc500sp3.git', str(vc5_sp3)], check=True)
        subprocess.run(['git', '-C', str(vc5_sp3), 'checkout', '--detach', VC5_SP3_COMMIT], check=True)
    actual = subprocess.check_output(['git', '-C', str(vc5_sp3), 'rev-parse', 'HEAD'], text=True).strip()
    if actual != VC5_SP3_COMMIT:
        parser.error(f'{vc5_sp3} is at {actual}; expected {VC5_SP3_COMMIT}. Existing checkout left intact.')
    # SP3 retains the RTM driver's banner; verify the actual updated phases too.
    verify(vc5_sp3 / 'bin/c1.dll', '7f12a4a889c5a0277f12c391ca462657dc81ef7be769faf629331bd117983d5d')
    verify(vc5_sp3 / 'bin/c2.exe', 'e75aecaf4073b0817ffb638cae5fff636b2e2d1a090daabf8f68dbc954515fae')

    vc420 = ROOT / 'tools/msvc420'
    if not vc420.exists():
        subprocess.run(['git', 'clone', '--no-checkout',
                        'https://github.com/archaic-msvc/msvc420.git', str(vc420)], check=True)
        subprocess.run(['git', '-C', str(vc420), 'checkout', '--detach', VC420_COMMIT], check=True)
    actual = subprocess.check_output(['git', '-C', str(vc420), 'rev-parse', 'HEAD'], text=True).strip()
    if actual != VC420_COMMIT:
        parser.error(f'{vc420} is at {actual}; expected {VC420_COMMIT}. Existing checkout left intact.')
    for name, digest in VC420_SHA256.items():
        verify(vc420 / 'bin' / name, digest)
    for name, digest in VC420_RUNTIME_SHA256.items():
        verify(vc5_sp3 / 'redist' / name, digest)
        target = vc420 / 'bin' / name
        if not target.exists():
            shutil.copyfile(vc5_sp3 / 'redist' / name, target)
        verify(target, digest)

    system, arch = platform.system(), platform.machine()
    asset = 'macos' if system == 'Darwin' else arch
    if system not in ('Darwin', 'Linux') or asset not in WIBO_SHA256:
        parser.error(f'No pinned wibo binary for {system} {arch}.')
    wibo = ROOT / 'build/tools/wibo'
    download(f'https://github.com/decompals/wibo/releases/download/1.1.0/wibo-{asset}',
             wibo, WIBO_SHA256[asset])
    wibo.chmod(0o755)
    version = subprocess.run([str(wibo), str(compiler / 'VC98/Bin/CL.EXE')],
                             capture_output=True, text=True, check=True)
    if '12.00.8168' not in version.stdout + version.stderr:
        parser.error('The compiler did not report version 12.00.8168.')
    print('Verified MSVC 12.00.8168 (VC6 RTM, for zlib and INSANE) through wibo.', flush=True)

    # The project-wide compiler (docs/compiler-default.md): every
    # game TU compiles with this SP3 build.
    version = subprocess.run([str(wibo), str(vc5_sp3 / 'bin/cl.exe')],
                             capture_output=True, text=True, check=True)
    if '11.00.7022' not in version.stdout + version.stderr:
        parser.error('The VC5 SP3 driver did not report version 11.00.7022.')
    print('Verified MSVC 5.0 SP3 (C2 11.00.7303) through wibo.', flush=True)

    version = subprocess.run([str(wibo), str(vc420 / 'bin/CL.EXE')], capture_output=True, text=True)
    if '10.20.6166' not in version.stdout + version.stderr:
        parser.error('The Visual C++ 4.20 driver did not report version 10.20.6166.')
    print('Verified Visual C++ 4.20 (10.20.6166) through wibo.', flush=True)

    # Intel C/C++ 4.0 compiles rdModel3K and the libm helpers (docs/crt.md); it is a hard requirement
    # like the MSVC packages. The 14-day trial check is patched out of the driver (icl_notrial.exe).
    install_intel_compiler(parser, args.icl_installer)
    version = subprocess.run([str(wibo), str(ROOT / 'tools/intel-cc-4.0/Compiler_Bin_Files/icl_notrial.exe')],
                             capture_output=True, text=True)
    if 'Version 4.0  99100' not in version.stdout + version.stderr:
        parser.error('The Intel driver did not report Version 4.0 99100.')
    print('Verified Intel C/C++ 4.0 99100 through wibo.', flush=True)

    # MASM 6.13.7299 assembles INSANE's MASM units (docs/crt.md, "MASM 6.13").
    install_masm(parser)
    version = subprocess.run([str(wibo), str(ROOT / 'tools/masm613/ml.exe')], capture_output=True, text=True)
    if '6.13.7299' not in version.stdout + version.stderr:
        parser.error('ML.EXE did not report Version 6.13.7299.')
    print('Verified MASM 6.13.7299 through wibo.', flush=True)

    # Every compile runs under wibo 1.1.0 with the NT heap model (WIBO_HEAP_MODEL=nt;
    # docs/crt.md "A principled 1999 heap model"), built per checkout into build/wibo-heapmodel/wibo-heapnt
    # from pinned inputs. Needs CMake (fetched on Linux x86_64), a C++20 compiler and network access.
    subprocess.run([sys.executable, str(ROOT / 'Scripts/build_wibo_heapmodel.py')], check=True)
    heapnt = ROOT / 'build/wibo-heapmodel/wibo-heapnt'
    version = subprocess.run([str(heapnt), str(vc5_sp3 / 'bin/cl.exe')], capture_output=True, text=True,
                             env={**os.environ, 'WIBO_HEAP_MODEL': 'nt'})
    if '11.00.7022' not in version.stdout + version.stderr:
        parser.error('The heap-model wibo could not run MSVC 5.0 SP3.')
    print('Verified the heap-model wibo (build/wibo-heapmodel/wibo-heapnt).', flush=True)

    # Berkeley yacc 1.4 and flex 2.3.6, built natively from their pinned 1990 postings: the generators of
    # Libs/sith/Cog/sithCogYacc.c and sithCogFlex.c from cog.y/cog.l (Scripts/cog_generators.py). Optional: the
    # build compiles the checked-in output; without them `ninja regen-cog` is unavailable and the test that the
    # checked-in files are the grammar's output is skipped.
    if subprocess.run([sys.executable, str(ROOT / 'Scripts/cog_generators.py'), 'build']).returncode:
        print('Note: building byacc 1.4/flex 2.3.6 failed (needs a host C compiler); '
              'the Cog regeneration test will be skipped.', flush=True)

    objdiff_asset = ('macos' if system == 'Darwin' else 'linux') + '-' + arch
    if objdiff_asset not in OBJDIFF_SHA256:
        parser.error(f'No pinned objdiff binary for {system} {arch}.')
    objdiff = ROOT / 'build/tools/objdiff-cli'
    expected = OBJDIFF_SHA256[objdiff_asset]
    download(f'https://github.com/encounter/objdiff/releases/download/v3.5.1/objdiff-cli-{objdiff_asset}',
             objdiff, expected)
    objdiff.chmod(0o755)

    sdk = ROOT / 'Libs/external/DirectX61c'
    if not (sdk / 'include/d3d.h').exists() or not (sdk / 'lib/ddraw.lib').exists():
        sevenzip = shutil.which('7zz') or shutil.which('7z')
        if not sevenzip:
            parser.error('Install 7-Zip to extract the SDK (on macOS: brew install sevenzip).')
        archive = ROOT / 'tools/downloads/dx61sdkimage.exe'
        download(SDK_URL, archive, SDK_SHA256)
        with tempfile.TemporaryDirectory(prefix='infernal-dx61-') as temp:
            subprocess.run([sevenzip, 'x', '-y', '-bso0', '-bsp0', '-o' + temp,
                            str(archive), 'include/*', 'lib/*'], check=True)
            for directory in ('include', 'lib'):
                shutil.copytree(Path(temp) / directory, sdk / directory, dirs_exist_ok=True)

    for exe in args.exe:
        install_executable(exe.expanduser().resolve())
    for digest, (_, name) in EXECUTABLES.items():
        if (ROOT / name).exists():
            verify(ROOT / name, digest)

    python = ROOT / '.venv/bin/python'
    if not python.exists():
        venv.EnvBuilder(with_pip=True).create(ROOT / '.venv')
    subprocess.run([str(python), '-m', 'pip', 'install', '-r', 'requirements.txt'], check=True)
    subprocess.run([str(python), 'configure.py'], check=True)
    print('Setup complete. Run ninja for the 1.2 build (needs orig/Indy3D-1.2.exe), ninja v1.0 for the 1.0 build '
          '(needs orig/Indy3D.exe), or ninja converted without an original executable.')


if __name__ == '__main__':
    main()
