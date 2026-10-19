#!/usr/bin/env python3
"""Link the debug build's objects (`ninja debug`) into build/<version>-debug/Indy3D_debug.exe: `ninja debug-link`.

The debug build is not byte-matched (docs/public/DEBUG-BUILD.md). The link follows tools/linktrial.sh (docs/link-trial.md):
only objects compiled from this repository's source, the period linker 5.10.7303 and resource compiler of the pinned
VS97 SP3 package under wine (wibo's link.exe fails under /OPT:REF and its rc.exe lacks FormatMessageA), the release CRT
`libc.lib` of the same package (the objects are /ML and do not define _DEBUG, so they name no debug-CRT function), the
DirectX 6.1 import libraries and Intel's libm. The icons are the user-supplied assets the release build extracts from
the user's own executable. The wine prefix defaults to the link trial's (~/.cache/linktrial-wineprefix, overridable with
LINKTRIAL_WINEPREFIX); it is never inside the checkout.
"""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parent.parent
LIBS = ('libc.lib kernel32.lib user32.lib gdi32.lib comdlg32.lib comctl32.lib shell32.lib ddraw.lib dsound.lib '
        'dinput.lib winmm.lib advapi32.lib ole32.lib')


def winpath(path):
    return 'Z:' + str(Path(path).resolve()).replace('/', '\\')


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--version', required=True)
    ap.add_argument('--build-dir', type=Path, default=Path('build'))
    ap.add_argument('--icons', type=Path, required=True, help='the extracted icons directory')
    args = ap.parse_args()
    if shutil.which('wine') is None:
        sys.exit('debug-link needs wine (the period link.exe and rc.exe run under it); `ninja debug` needs only wibo')
    out = ROOT / args.build_dir / (args.version + '-debug')
    config = json.loads((out / 'config.json').read_text())
    objs = [out / 'obj' / Path(s).with_suffix('.obj') for s in config['sources']]
    tools = ROOT / 'tools/msvc500sp3-7303'
    dx = ROOT / 'Libs/external/DirectX61c/lib'
    libm = ROOT / 'tools/intel-cc-4.0/Compiler_Lib_files_(shared)'
    env = dict(os.environ, WINEARCH='win32', WINEDEBUG='-all',
               WINEPREFIX=os.environ.get('LINKTRIAL_WINEPREFIX', str(Path.home() / '.cache/linktrial-wineprefix')),
               LIB=winpath(tools / 'lib'), INCLUDE=winpath(tools / 'include'), WINEPATH=winpath(tools / 'redist'))
    res = out / 'res'
    if res.exists():
        shutil.rmtree(res)
    shutil.copytree(ROOT / args.icons, res)
    shutil.copy(ROOT / 'Jones3D/Indy3D.rc', res / 'Indy3D.rc')
    subprocess.run(['wine', str(tools / 'bin/rc.exe'), '-l', '409', '-i', winpath(tools / 'include'),
                    '-fo', 'Indy3D.res', 'Indy3D.rc'], cwd=res, env=env, check=True, stdout=subprocess.DEVNULL)
    subprocess.run(['wine', str(tools / 'bin/cvtres.exe'), '/nologo', '/machine:ix86', '/out:Indy3D_res.obj',
                    'Indy3D.res'], cwd=res, env=env, check=True, stdout=subprocess.DEVNULL)
    rsp = ['/nologo', '/OUT:Indy3D_debug.exe', '/SUBSYSTEM:WINDOWS,4.0', '/BASE:0x400000', '/MAP:Indy3D_debug.map',
           '/INCREMENTAL:NO', '/OPT:REF', *(winpath(o) for o in objs), winpath(res / 'Indy3D_res.obj'),
           '/LIBPATH:' + winpath(dx), '/LIBPATH:' + winpath(libm), LIBS]
    (out / 'link.rsp').write_text('\r\n'.join(rsp) + '\r\n')
    proc = subprocess.run(['wine', str(tools / 'bin/link.exe'), '@link.rsp'], cwd=out, env=env,
                          stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, errors='replace')
    print(proc.stdout, end='')
    if proc.returncode or not (out / 'Indy3D_debug.exe').is_file():
        (out / 'Indy3D_debug.exe').unlink(missing_ok=True)
        sys.exit(f'link.exe failed ({proc.returncode})')
    print(f'{out / "Indy3D_debug.exe"}: {(out / "Indy3D_debug.exe").stat().st_size} bytes')


if __name__ == '__main__':
    main()
