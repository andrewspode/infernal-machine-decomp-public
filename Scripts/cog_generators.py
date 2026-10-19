#!/usr/bin/env python3
"""Build the period parser generators and regenerate the Cog script parser from its grammar sources.

Libs/sith/Cog/sithCogYacc.c (+ the generated block of sithCogYacc.h) and Libs/sith/Cog/sithCogFlex.c are
generator output, kept checked in so the compile path does not depend on these tools. Their sources are
Libs/sith/Cog/cog.y (Berkeley yacc 1.4, `yacc -dl`), Libs/sith/Cog/cog.l and the project's scanner skeleton
Libs/sith/Cog/flex.skel (flex 2.3, `flex -Li -Sflex.skel`). Retail evidence for the versions: the
`@(#)yaccpar 1.4 (Berkeley) 02/25/90` banner string in .data, and the parser and DFA tables, which these
generators reproduce exactly (config/indy3d-v1.0/sithCogYacc.md, sithCogFlex.md).

Pinned inputs: the comp.sources.unix postings of both generators, by SHA-256 of each shar part:
  Berkeley yacc 1.4 (Robert Corbett), volume 21 issues 78-82, April 1990;
  flex 2.3 at patchlevel 6 (Vern Paxson), volume 23 issues 37-46, whose flex.skel is
  `$Header: /usr/fsys/odin/a/vern/flex/RCS/flex.skel,v 2.16 90/08/03 14:09:36 vern Exp $`.
The parts are unpacked without running them as shell scripts and built natively with the host C compiler
(gcc or clang; Linux x86_64 is the tested host). The 1990 K&R sources are compiled unmodified: a forced-include
shim only renames the sources' own K&R declarations of C library functions onto thin wrappers so they cannot
conflict with the host's prototypes. Before use, each generator must reproduce its own distribution's shipped
output: byacc its test/ftp and test/error parsers, flex its initscan.c bootstrap scanner from scan.l.

  python3 Scripts/cog_generators.py build [--downloads DIR] [--force]   # -> build/cog-generators/{yacc,flex}
  python3 Scripts/cog_generators.py regen                               # rewrite the generated sources
  python3 Scripts/cog_generators.py check                               # exit 1 if they differ
"""
import argparse
import hashlib
import os
from pathlib import Path
import platform
import re
import shutil
import subprocess
import sys
import tempfile
import urllib.request

ROOT = Path(__file__).resolve().parent.parent
WORK = ROOT / 'build/cog-generators'
YACC = WORK / 'yacc'
FLEX = WORK / 'flex'
STAMP = WORK / 'stamp'
COG = ROOT / 'Libs/sith/Cog'

ARCHIVE = 'https://sources.vsta.org/comp.sources.unix'
BYACC_PARTS = {
    'part01': '61e9c976e47074d02b3308319eef5af824ad32cffc71d2cba70856d76941d676',
    'part02': '3eb7f4639c349dea867c7e56a4aa1b250d6bc1ecf67ff37be41233b3bd659745',
    'part03': 'fda9a83553ce2675b3af381b46bf32e6e066469c1012c8c7b6f89101587f1df9',
    'part04': '4bff1eaa1aa541a720f2d290238cef301de075c9c74b5c2938883ff867b2e285',
    'part05': '8f84ab6f0d92a75d22f7dfbf494b73784363bfc78466a47e9c8f1bdb1b49c957',
}
FLEX_PARTS = {
    'part01': 'bcd8c504aa24e319476f07da158ed2ccc3cf703ecd5d21b6ab6558e6b18a23b9',
    'part02': '2c4234d871fe50b3aab816bc463bd1b9036c7dbac376e785df5c0e0c6b0513f3',
    'part03': '9542f437c7f1dc223cc2f6daddc8e43b3fb92d70a1f0c58516537e21d0bc446a',
    'part04': '0baeb324f6050acfe7e189e105f10e94d2dfc5696a5c6d9b26f3451ea581f77b',
    'part05': '1ad9f7856ac0d90a23af5461c507416203d8b9776ca1fc6d4b80e266d6ea7774',
    'part06': '57fc3084cc6eb2b3900911c85b41383f67f50d84654a1d285a01157b650ed4f3',
    'part07': 'ebbdbb097c095dd4ca1391a415b1ff5bd85d0d8e7236fea2b52d54ca4015dc45',
    'part08': '9d536b35f9b45c8832dfe78f389488babc345bffce8d2fb3335c3b3667711989',
    'part09': 'bfa9365fe1f3f56ebe7fc7b9d307195ce5bfbd51ebc68920439b9cd417229993',
    'part10': '9941005d216b0f4438313c8d2512d70239e2e8efb1fbb39a5b89fffde1c8c99e',
}
PACKAGES = (('byacc-1.4', 'volume21/berkeley_yacc', BYACC_PARTS), ('flex-2.3.6', 'volume23/flex2.3', FLEX_PARTS))

# Host shims, force-included ahead of each 1990 source file. They change no generator logic.
BYACC_SHIM = '''#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>
#define malloc compat_malloc
#define calloc compat_calloc
#define realloc compat_realloc
#define free compat_free
#define strcpy compat_strcpy
#define mktemp compat_mktemp
#define getenv compat_getenv
'''
BYACC_WRAPPERS = '''#include <stdlib.h>
#include <string.h>
#include <unistd.h>
char *compat_malloc(unsigned n) { return malloc(n ? n : 1); }
char *compat_calloc(unsigned n, unsigned s) { return calloc(n ? n : 1, s ? s : 1); }
char *compat_realloc(char *p, unsigned n) { return realloc(p, n ? n : 1); }
void compat_free(char *p) { free(p); }
char *compat_strcpy(char *d, const char *s) { return strcpy(d, s); }
char *compat_getenv(const char *n) { return getenv(n); }
char *compat_mktemp(char *t) { int fd = mkstemp(t); if (fd >= 0) close(fd); return t; }
'''
# flex 2.3 is configured as the System V build its Makefile documents (-DSYS_V: <string.h>, no K&R memset).
FLEX_SHIM = '''#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#define read compat_read
#define write compat_write
#define unlink compat_unlink
'''
FLEX_WRAPPERS = '''#include <unistd.h>
int compat_read(int fd, char *b, int n) { return (int) read(fd, b, n); }
int compat_write(int fd, char *b, int n) { return (int) write(fd, b, n); }
int compat_unlink(char *p) { return unlink(p); }
'''
FLEX_OBJECTS = ('ccl', 'dfa', 'ecs', 'gen', 'main', 'misc', 'nfa', 'parse', 'scan', 'sym', 'tblcmp', 'yylex')

# The generated sources and the generator command lines that produce them.
YACC_FLAGS = ['-dl']          # -d: y.tab.h; -l: no #line directives (retail's __FILE__ is sithCogYacc.c)
FLEX_FLAGS = ['-Li']          # -L: no #line directives; -i: case-insensitive (retail's yy_ec folds case)


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def stamp_text():
    pins = ''.join(f'{n} {p} {d}\n' for n, _, parts in PACKAGES for p, d in sorted(parts.items()))
    shims = hashlib.sha256((BYACC_SHIM + BYACC_WRAPPERS + FLEX_SHIM + FLEX_WRAPPERS).encode()).hexdigest()
    return pins + 'shim ' + shims + '\n'


def built():
    return YACC.exists() and FLEX.exists() and STAMP.exists() and STAMP.read_text() == stamp_text()


def fetch(url, path, digest):
    if not path.exists():
        path.parent.mkdir(parents=True, exist_ok=True)
        partial = path.with_name(path.name + '.partial')
        print(f'Downloading {url}', flush=True)
        with urllib.request.urlopen(url, timeout=60) as response, partial.open('wb') as output:
            shutil.copyfileobj(response, output)
        if sha256(partial) != digest:
            raise SystemExit(f'Unexpected SHA-256 for {url}: {sha256(partial)}')
        partial.replace(path)
    if sha256(path) != digest:
        raise SystemExit(f'Unexpected SHA-256 for {path}: {sha256(path)}')


def unshar(parts, out):
    """Extract every `sed "s/^X//" >'name' <<'END'` member, checking the character count the shar declares."""
    for part in parts:
        lines = part.read_bytes().split(b'\n')
        declared = {}
        i = 0
        while i < len(lines):
            count = re.search(rb"""Extracting \\"'([^']+)'\\" \\\((\d+) characters\\\)""", lines[i])
            if count:
                declared[count.group(1).decode()] = int(count.group(2))
            member = re.match(rb"""\s*sed "s/\^X//" >'([^']+)' <<'([^']+)'$""", lines[i])
            if member:
                name, end = member.group(1).decode(), member.group(2)
                body = []
                i += 1
                while lines[i] != end:
                    body.append(lines[i][1:] if lines[i].startswith(b'X') else lines[i])
                    i += 1
                data = b'\n'.join(body) + b'\n' if body else b''
                if name in declared and declared[name] != len(data):
                    raise SystemExit(f'{part}: {name} is {len(data)} characters, the shar declares {declared[name]}')
                target = out / name
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(data)
            i += 1
    # flex 2.3's PACKNOTES: large files were split into numbered pieces.
    for first in sorted(out.glob('*.01')):
        stem = first.name[:-3]
        pieces = sorted(out.glob(stem + '.0[1-9]'))
        (out / stem).write_bytes(b''.join(p.read_bytes() for p in pieces))
        for p in pieces:
            p.unlink()


def run(cmd, **kw):
    print('+', ' '.join(map(str, cmd)), flush=True)
    subprocess.run(list(map(str, cmd)), check=True, **kw)


def host_cc(parser):
    cc = os.environ.get('CC') or shutil.which('gcc') or shutil.which('cc') or shutil.which('clang')
    if not cc:
        parser.error('Building the parser generators needs a host C compiler (gcc or clang).')
    return cc


def build(parser, downloads, force):
    if built() and not force:
        print(f'{YACC} and {FLEX} are up to date.')
        return
    cc = host_cc(parser)
    if WORK.exists():
        shutil.rmtree(WORK)
    src = WORK / 'src'
    for name, path, parts in PACKAGES:
        files = []
        for part, digest in sorted(parts.items()):
            local = downloads / f'{name}-{part}'
            fetch(f'{ARCHIVE}/{path}/{part}', local, digest)
            files.append(local)
        unshar(files, src / name)
    old = ['-std=gnu89', '-O1', '-w', '-fcommon']
    link = ['-no-pie'] if platform.system() == 'Linux' else []

    byacc, obj = src / 'byacc-1.4', WORK / 'obj/byacc'
    obj.mkdir(parents=True)
    (obj / 'shim.h').write_text(BYACC_SHIM)
    (obj / 'wrappers.c').write_text(BYACC_WRAPPERS)
    objects = []
    for source in sorted(byacc.glob('*.c')):
        # lr0.c declares `static` file-scope tables that defs.h has already declared `extern`, which 1990
        # compilers accepted and current ones reject; the qualifier is dropped for that file only.
        extra = ['-Dstatic='] if source.name == 'lr0.c' else []
        out = obj / (source.stem + '.o')
        run([cc, *old, *extra, '-include', obj / 'shim.h', '-c', source, '-o', out])
        objects.append(out)
    run([cc, '-O1', '-c', obj / 'wrappers.c', '-o', obj / 'wrappers.o'])
    run([cc, *link, *objects, obj / 'wrappers.o', '-o', YACC])
    for test, flags in (('ftp', ['-dv']), ('error', ['-dv'])):
        with tempfile.TemporaryDirectory(dir=WORK) as temp:
            shutil.copy(byacc / 'test' / f'{test}.y', temp)
            run([YACC, *flags, '-b', test, f'{test}.y'], cwd=temp)
            for suffix in ('.tab.c', '.tab.h', '.output'):
                if (byacc / 'test' / (test + suffix)).exists() and \
                        (Path(temp) / (test + suffix)).read_bytes() != (byacc / 'test' / (test + suffix)).read_bytes():
                    raise SystemExit(f'byacc 1.4 does not reproduce its own test/{test}{suffix}')

    flex, obj = src / 'flex-2.3.6', WORK / 'obj/flex'
    obj.mkdir(parents=True)
    (obj / 'shim.h').write_text(FLEX_SHIM)
    (obj / 'wrappers.c').write_text(FLEX_WRAPPERS)
    run([YACC, '-d', 'parse.y'], cwd=flex)
    (flex / 'y.tab.c').replace(flex / 'parse.c')
    (flex / 'y.tab.h').replace(flex / 'parse.h')
    shutil.copy(flex / 'initscan.c', flex / 'scan.c')
    objects = []
    for stem in FLEX_OBJECTS:
        out = obj / (stem + '.o')
        run([cc, *old, '-DSYS_V', f'-DDEFAULT_SKELETON_FILE="{flex / "flex.skel"}"', '-include', obj / 'shim.h',
             '-I', flex, '-c', flex / (stem + '.c'), '-o', out])
        objects.append(out)
    run([cc, '-O1', '-c', obj / 'wrappers.c', '-o', obj / 'wrappers.o'])
    run([cc, *link, *objects, obj / 'wrappers.o', '-o', FLEX])
    with tempfile.TemporaryDirectory(dir=WORK) as temp:
        shutil.copy(flex / 'scan.l', temp)
        shutil.copy(flex / 'flex.skel', temp)
        scan = subprocess.run([str(FLEX), '-ist8', '-Sflex.skel', 'scan.l'], cwd=temp, check=True,
                              capture_output=True).stdout
        if scan != (flex / 'initscan.c').read_bytes():
            raise SystemExit('flex 2.3.6 does not reproduce its own initscan.c from scan.l')
    STAMP.write_text(stamp_text())
    print(f'Built and self-tested {YACC} (Berkeley yacc 1.4) and {FLEX} (flex 2.3.6).')


def generate():
    """{repository path: generated bytes} for every generated Cog parser file."""
    if not built():
        raise SystemExit(f'Build the generators first: python3 {Path(__file__).relative_to(ROOT)} build')
    with tempfile.TemporaryDirectory() as temp:
        for name in ('cog.y', 'cog.l', 'flex.skel'):
            shutil.copy(COG / name, temp)
        subprocess.run([str(YACC), *YACC_FLAGS, 'cog.y'], cwd=temp, check=True)
        subprocess.run([str(FLEX), *FLEX_FLAGS, '-Sflex.skel', 'cog.l'], cwd=temp, check=True)
        out = Path(temp)
        ytab_h = (out / 'y.tab.h').read_bytes()
        files = {'sithCogYacc.c': (out / 'y.tab.c').read_bytes(), 'sithCogFlex.c': (out / 'lex.yy.c').read_bytes()}
    # sithCogYacc.h is y.tab.h inside the project's header wrapper (include guard, the type headers the union
    # needs, and the scanner/parser entry points); the wrapper is hand-written, the y.tab.h block is generated.
    header = (COG / 'sithCogYacc.h').read_bytes()
    start = header.index(b'#define ' + ytab_h.split(b'\n', 1)[0].split()[1] + b' ')
    end = header.index(b'extern YYSTYPE yylval;\n', start) + len(b'extern YYSTYPE yylval;\n')
    files['sithCogYacc.h'] = header[:start] + ytab_h + header[end:]
    return {f'Libs/sith/Cog/{name}': data for name, data in files.items()}


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('action', choices=('build', 'regen', 'check'))
    parser.add_argument('--downloads', type=Path, default=ROOT / 'tools/downloads/cog-generators',
                        help='where the pinned shar parts are cached (default: tools/downloads/cog-generators)')
    parser.add_argument('--force', action='store_true', help='rebuild the generators even when up to date')
    args = parser.parse_args()
    if args.action == 'build':
        build(parser, args.downloads.resolve(), args.force)
        return
    stale = []
    for path, data in generate().items():
        if (ROOT / path).read_bytes() != data:
            stale.append(path)
            if args.action == 'regen':
                (ROOT / path).write_bytes(data)
    if args.action == 'regen':
        print('Regenerated: ' + (', '.join(stale) if stale else 'nothing changed'))
    elif stale:
        raise SystemExit('Differ from cog.y/cog.l/flex.skel output: ' + ', '.join(stale))
    else:
        print('sithCogYacc.c, sithCogYacc.h and sithCogFlex.c are exactly the generators\' output.')


if __name__ == '__main__':
    main()
