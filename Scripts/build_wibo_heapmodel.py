#!/usr/bin/env python3
"""Build build/wibo-heapmodel/wibo-heapnt: wibo 1.1.0 with the NT heap model (tools/icl/wibo-heap-model.patch).

Every compile of the production build runs under this wibo with WIBO_HEAP_MODEL=nt
(configure.py --wibo-heapmodel, Scripts/vc6_compile.py): Intel C/C++ 4.0, VC5 SP3 and VC6 RTM alike.
mcpcom orders commutative operands by the addresses of its symbol records, so its output depends on the host heap,
and the NT heap model reproduces what the compiler gives on Windows (docs/crt.md, "A principled 1999 heap model").
MSVC output is byte-identical under it and under the release wibo (all 162 MSVC units).
Scripts/setup_toolchain.py runs this script. A worktree may symlink build/wibo-heapmodel from the main checkout.

Pinned inputs: the wibo source at its 1.1.0 commit (git), the patch in this repository, CMake 3.30.5 (Linux
x86_64 release archive; on other hosts a `cmake` >= 3.13 on PATH), libclang 18.1.1 from PyPI (wibo's
trampoline generator needs it) and the dependencies wibo's own CMake pins (mimalloc commit, Wine CRT DLL
hashes). Needs git, ninja, a C++20 compiler (gcc or clang) and network access. Linux x86_64 is the tested host.

  python3 Scripts/build_wibo_heapmodel.py [--force]
"""
import argparse
import hashlib
import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys
import tarfile
import urllib.request

ROOT = Path(__file__).resolve().parent.parent
WIBO_URL = 'https://github.com/decompals/wibo.git'
WIBO_COMMIT = 'b80feaae19c1b0b0759d9abf2b6afabc48957c3d'  # tag 1.1.0, the pinned release binary's source
PATCH = ROOT / 'tools/icl/wibo-heap-model.patch'
CMAKE_URL = 'https://github.com/Kitware/CMake/releases/download/v3.30.5/cmake-3.30.5-linux-x86_64.tar.gz'
CMAKE_SHA256 = 'f747d9b23e1a252a8beafb4ed2bc2ddf78cff7f04a8e4de19f4ff88e9b51dc9d'
LIBCLANG = 'libclang==18.1.1'
LIBCLANG_SHA256 = 'c533091d8a3bbf7460a00cb6c1a71da93bffe148f172c7d03b1c31fbf8aa2a0b'  # manylinux2010_x86_64 wheel
WORK = ROOT / 'build/wibo-heapmodel'
OUTPUT = WORK / 'wibo-heapnt'
STAMP = OUTPUT.with_name(OUTPUT.name + '.stamp')


def stamp_text():
    return WIBO_COMMIT + '\n' + hashlib.sha256(PATCH.read_bytes()).hexdigest() + '\n'


def up_to_date():
    return OUTPUT.exists() and STAMP.exists() and STAMP.read_text() == stamp_text()


def run(cmd, **kw):
    print('+', ' '.join(map(str, cmd)), flush=True)
    subprocess.run(cmd, check=True, **kw)


def cmake_binary():
    if platform.system() == 'Linux' and platform.machine() == 'x86_64':
        archive = ROOT / 'tools/downloads/cmake-3.30.5-linux-x86_64.tar.gz'
        if not archive.exists():
            archive.parent.mkdir(parents=True, exist_ok=True)
            partial = archive.with_name(archive.name + '.partial')
            print('Downloading', CMAKE_URL, flush=True)
            with urllib.request.urlopen(CMAKE_URL, timeout=120) as response, partial.open('wb') as out:
                shutil.copyfileobj(response, out)
            if hashlib.sha256(partial.read_bytes()).hexdigest() != CMAKE_SHA256:
                raise SystemExit('Unexpected SHA-256 for the CMake archive')
            partial.replace(archive)
        dest = WORK / 'cmake'
        if not (dest / 'bin/cmake').exists():
            with tarfile.open(archive) as tar:
                # The archive is SHA-256 pinned above. Where the interpreter has the extraction filters
                # (3.12, 3.11.4+, 3.10.12+) use them; older ones extract the verified archive as is.
                if hasattr(tarfile, 'data_filter'):
                    tar.extractall(WORK, filter='data')
                else:
                    tar.extractall(WORK)
            (WORK / 'cmake-3.30.5-linux-x86_64').rename(dest)
        return dest / 'bin/cmake'
    found = shutil.which('cmake')
    if not found:
        raise SystemExit('Install CMake >= 3.13 (on macOS: brew install cmake).')
    return Path(found)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--force', action='store_true', help='rebuild even if the stamp matches')
    args = ap.parse_args()
    if up_to_date() and not args.force:
        print(f'{OUTPUT.relative_to(ROOT)} is up to date.')
        return 0
    if not shutil.which('ninja') and not (ROOT / 'build/bin/ninja').exists():
        raise SystemExit('Install Ninja first.')
    WORK.mkdir(parents=True, exist_ok=True)
    src = WORK / 'src'
    if not src.exists():
        run(['git', 'clone', '--no-checkout', WIBO_URL, str(src)])
    run(['git', '-C', str(src), 'checkout', '--detach', '--force', WIBO_COMMIT])
    run(['git', '-C', str(src), 'clean', '-fdx', '-e', 'build/'])
    run(['git', '-C', str(src), 'apply', str(PATCH)])

    pylib = WORK / 'pylibclang'
    if not (pylib / 'clang/cindex.py').exists():
        req = WORK / 'libclang-requirements.txt'
        hashes = f' --hash=sha256:{LIBCLANG_SHA256}' if platform.system() == 'Linux' else ''
        req.write_text(LIBCLANG + hashes + '\n')
        run([sys.executable, '-m', 'pip', 'install', '--quiet', '--no-deps', '--only-binary=:all:',
             *(['--require-hashes'] if hashes else []), '--target', str(pylib), '-r', str(req)])
    libclang = next(pylib.glob('clang/native/libclang*'), None)
    if libclang is None:
        raise SystemExit('libclang wheel has no native library for this host')

    cmake = cmake_binary()
    preset = 'release-macos' if platform.system() == 'Darwin' else 'release64'
    env = dict(os.environ, PYTHONPATH=str(pylib) + os.pathsep + os.environ.get('PYTHONPATH', ''))
    if (ROOT / 'build/bin/ninja').exists():
        env['PATH'] = str(ROOT / 'build/bin') + os.pathsep + env['PATH']
    env['PATH'] = str(cmake.parent) + os.pathsep + env['PATH']
    run([str(cmake), '--preset', preset, '-DWIBO_ENABLE_FIXTURE_TESTS=OFF', '-DWIBO_ENABLE_LIBURING=OFF',
         '-DWIBO_VERSION=1.1.0-heapmodel', f'-DLIBCLANG_LIBRARY={libclang}'], cwd=src, env=env)
    run([str(cmake), '--build', '--preset', preset, '-j2'], cwd=src, env=env)
    built = src / 'build' / preset / 'wibo'
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    tmp = OUTPUT.with_name(OUTPUT.name + '.partial')
    shutil.copy2(built, tmp)
    tmp.chmod(0o755)
    tmp.replace(OUTPUT)
    STAMP.write_text(stamp_text())
    print(f'Built {OUTPUT.relative_to(ROOT)} (wibo 1.1.0 + NT heap model).')
    return 0


if __name__ == '__main__':
    sys.exit(main())
