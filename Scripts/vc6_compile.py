"""Compile a checked-in C (or MASM) source directly with its selected compiler through the configured wibo.

Every compile runs under config.json's `wibo` with `compiler_env` added to the environment: the heap-model
wibo with WIBO_HEAP_MODEL=nt (docs/crt.md, "A principled 1999 heap model").
"""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parent.parent


def winpath(path):
    return 'Z:' + str(path).replace('/', '\\')


def intel_compile(config_unit, source, output, env):
    """Compile with Intel C/C++ 4.0 (docs/crt.md, "rdModel3K reconstruction") through wibo.

    The driver needs a writable temp directory (else "Can not create tmp file"), the SSE headers
    by explicit -I (it does not find its own), and emits one object; there is no /TC equivalent
    needed (a .c file compiles as C).
    """
    env = env.copy()
    tmp = ROOT / 'build/icl-tmp'
    tmp.mkdir(parents=True, exist_ok=True)
    env['TMP'] = env['TEMP'] = winpath(tmp)
    flags = config_unit.get('compiler_flags', []) + config_unit['cflags']
    includes = ['-I' + winpath(p) for p in config_unit['includes']]
    with tempfile.TemporaryDirectory(prefix=output.stem + '.', dir=output.parent) as temp:
        compiled = Path(temp) / output.name
        proc = subprocess.run([config_unit['wibo'], config_unit['compiler'], '-nologo', '-c'] + flags + includes +
                              ['-Fo' + winpath(compiled), str(source.relative_to(ROOT))],
                              cwd=ROOT, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        log = Path(temp) / output.with_suffix('.log').name
        log.write_bytes(proc.stdout)
        log.replace(output.with_suffix('.log'))
        if proc.returncode:
            output.unlink(missing_ok=True)
            sys.stdout.buffer.write(proc.stdout)
        else:
            compiled.replace(output)
        return proc.returncode


def masm_assemble(config_unit, source, output, env):
    """Assemble with MASM 6.13 (docs/crt.md, "MASM 6.13") through wibo: COFF output, no listing."""
    flags = config_unit.get('compiler_flags', []) + config_unit['cflags']
    with tempfile.TemporaryDirectory(prefix=output.stem + '.', dir=output.parent) as temp:
        compiled = Path(temp) / output.name
        proc = subprocess.run([config_unit['wibo'], config_unit['compiler'], '/nologo', '/c'] + flags +
                              ['/Fo' + winpath(compiled), str(source.relative_to(ROOT)).replace('/', '\\')],
                              cwd=ROOT, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        log = Path(temp) / output.with_suffix('.log').name
        log.write_bytes(proc.stdout)
        log.replace(output.with_suffix('.log'))
        if proc.returncode:
            output.unlink(missing_ok=True)
            sys.stdout.buffer.write(proc.stdout)
        else:
            compiled.replace(output)
        return proc.returncode


def unit_for(config, source):
    """The configured TU a compiled file belongs to: itself, or the TU whose version overlay it is
    (config.json `units[tu].source`, README.md "Versions")."""
    units = config.get('units', {})
    if source in units:
        return source
    return next((key for key, value in units.items() if value.get('source') == source), source)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('source')
    ap.add_argument('output')
    ap.add_argument('--config', type=Path, required=True)
    ap.add_argument('--unit-key', default=None,
                    help='look the per-unit compiler, flags and includes up under '
                         'this configured source path instead of under `source`. '
                         'Only for out-of-tree variants of a configured TU, such '
                         'as the cells tools/bulksweep.py compiles; the production '
                         'build never passes it.')
    args = ap.parse_args()
    config = json.loads((ROOT / args.config).read_text())
    source = ROOT / args.source
    output = ROOT / args.output
    output.parent.mkdir(parents=True, exist_ok=True)
    env = os.environ.copy()
    env.pop('CL', None)
    env.pop('_CL_', None)
    env.update(config.get('compiler_env', {}))
    unit_key = unit_for(config, args.unit_key or args.source)
    unit = config.get('units', {}).get(unit_key, {})
    if args.unit_key and not unit:
        sys.exit('--unit-key %s is not a configured source' % args.unit_key)
    if unit.get('compiler_kind') == 'icl':
        return intel_compile({**unit, 'wibo': config['wibo']}, source, output, env)
    if unit.get('compiler_kind') == 'masm':
        return masm_assemble({**unit, 'wibo': config['wibo']}, source, output, env)
    env['INCLUDE'] = ';'.join(winpath(p) for p in unit.get('includes', config['includes']))
    command = [config['wibo'], unit.get('compiler', config['compiler'])]
    # /TC is explicit: compiling as C++ changes ABI and code generation.
    flags = unit.get('compiler_flags', config.get('compiler_flags', [])) + unit.get('cflags', config['cflags'])
    # Objdiff may request a build while another build is finishing. Publish a
    # complete object atomically so readers never observe CL's partial output.
    with tempfile.TemporaryDirectory(prefix=output.stem + '.', dir=output.parent) as temp:
        compiled = Path(temp) / output.name
        proc = subprocess.run(command + ['/nologo', '/TC', '/c'] + flags +
                              ['/Fo' + winpath(compiled), str(source.relative_to(ROOT)).replace('/', '\\')],
                              cwd=ROOT, env=env,
                              stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        log = Path(temp) / output.with_suffix('.log').name
        log.write_bytes(proc.stdout)
        log.replace(output.with_suffix('.log'))
        if proc.returncode:
            output.unlink(missing_ok=True)
            sys.stdout.buffer.write(proc.stdout)
        else:
            compiled.replace(output)
        return proc.returncode


if __name__ == '__main__':
    sys.exit(main())
