"""Project-agnostic MSVC/PE build generator with the DTK project API shape.

Project-specific source lists, flags, versions and completion status belong in
configure.py. The binary backend remains Scripts.pebuild (i386 PE/COFF).
"""
from dataclasses import dataclass, field
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys

from Scripts.pe_project import (DEBUG_COMPILERS, DEBUG_ONLY_SOURCES, Project, ROOT, UNASSIGNED, debug_cflags,
                                version_file, write_changed, write_json)
from tools.objdiff_watch import check_watch_tree


def is_windows():
    return sys.platform == 'win32'


@dataclass
class ProgressCategory:
    id: str
    name: str


class Object:
    def __init__(self, completed, name, **options):
        if not isinstance(completed, bool):
            raise ValueError(f'{name}: completion status must be a boolean')
        if set(options) - {'cflags', 'extra_cflags', 'source', 'src_dir', 'progress_category', 'original_filename', 'compiler'}:
            raise ValueError(f'{name}: unsupported Object options: {sorted(options)}')
        self.name, self.completed, self.options = name, completed, options


@dataclass
class ProjectConfig:
    version: str = 'indy3d-v1.0'
    root: Path = ROOT
    build_dir: Path = Path('build')
    src_dir: Path = Path('.')
    compilers_path: Path = Path('tools/msvc500sp3-7303')
    compiler_path: Path | None = None
    compilers: dict = field(default_factory=dict)
    wrapper: Path = Path('build/tools/wibo')
    # The wibo every compile runs under, and its environment (configure.py; docs/crt.md).
    compile_wrapper: Path = Path('build/wibo-heapmodel/wibo-heapnt')
    compile_env: dict = field(default_factory=lambda: {'WIBO_HEAP_MODEL': 'nt'})
    objdiff_path: Path = Path('build/tools/objdiff-cli')
    ninja_path: Path = field(default_factory=lambda: Path(shutil.which('ninja') or 'ninja'))
    cflags: list = field(default_factory=list)
    libs: list = field(default_factory=list)
    progress_categories: list = field(default_factory=list)
    progress: bool = True
    progress_each_module: bool = False
    generate_map: bool = False
    generate_compile_commands: bool = True
    reconfig_deps: list = field(default_factory=list)
    split_deps: list = field(default_factory=list)
    custom_build_rules: list = field(default_factory=list)
    custom_build_steps: dict = field(default_factory=dict)
    configure_args: list = field(default_factory=list)
    # The public build export (tools/export_public.py) ships without tests/ and the research tools:
    # its graph has no test, parsecheck or audit targets.
    public: bool = False
    # A version's overlay tree (versions/<n>/, README.md "Versions"): a file there replaces the shared file of
    # the same relative path for this version's build. None for the base version.
    overlay_dir: Path | None = None
    # Further versions built from the same build.ninja (configure.py): each one's targets are named with
    # its `target_prefix` (`v1.0-all`, `v1.0-check`, ...) and its rules with `rule_suffix`.
    extra_versions: list = field(default_factory=list)
    target_prefix: str = ''
    rule_suffix: str = ''

    @property
    def config_path(self):
        return Path('config') / self.version / 'config.yml'

    @property
    def check_sha_path(self):
        return Path('config') / self.version / 'build.sha1'

    def out_path(self):
        return self.build_dir / self.version

    def objects(self):
        result, sources = {}, set()
        for lib in self.libs:
            if set(lib) - {'lib', 'cflags', 'extra_cflags', 'src_dir', 'progress_category', 'objects', 'compiler'}:
                raise ValueError(f'Unsupported library options: {lib}')
            for definition in lib['objects']:
                obj = Object(definition.completed, definition.name, **definition.options)
                opts = {**lib, **obj.options}
                source = Path(opts.get('src_dir', self.src_dir)) / opts.get('source', obj.name)
                if source.is_absolute() or '..' in source.parts or not (self.root / source).is_file():
                    raise ValueError(f'Invalid source path: {source}')
                obj.source = source.as_posix()
                # The file actually compiled: the version's overlay of the source when there is one.
                obj.input = obj.source
                if self.overlay_dir is not None and (self.root / self.overlay_dir / source).is_file():
                    obj.input = (Path(self.overlay_dir) / source).as_posix()
                obj.compiler = opts.get('compiler')
                if obj.compiler is not None and obj.compiler not in self.compilers:
                    raise ValueError(f'{obj.name}: unknown compiler {obj.compiler}')
                obj.cflags = list(opts.get('cflags', self.cflags)) + list(opts.get('extra_cflags', []))
                if 'original_filename' in opts:
                    filename = opts['original_filename']
                    if (not isinstance(filename, str) or not filename
                            or any(not 32 <= ord(c) < 127 for c in filename)):
                        raise ValueError(f'{obj.name}: original_filename must be a printable ASCII string')
                    # A compiler definition supplies diagnostic metadata while MSVC
                    # emits the literal and its relocations from the actual C source.
                    obj.cflags.append('/DJ3D_FILE=' + json.dumps(filename))
                obj.progress_category = opts.get('progress_category', lib['lib'])
                if obj.name in result or obj.source in sources:
                    raise ValueError(f'Duplicate object/source: {obj.name}')
                result[obj.name] = obj
                sources.add(obj.source)
        return result


def ninja_path(path):
    return str(path).replace('$', '$$').replace(' ', '$ ').replace(':', '$:')


def command(args):
    return shlex.join(map(str, args)).replace('$', '$$')


def custom_build(config):
    """The same rule/step dictionaries used by DTK's project generator."""
    lines = []
    P, R = config.target_prefix, config.rule_suffix
    phases = {'pre-compile', 'post-compile', 'post-link', 'post-build'}
    if set(config.custom_build_steps) - phases:
        raise ValueError('Unknown custom build phase')
    rule_options = {'command', 'description', 'depfile', 'deps', 'generator', 'restat', 'rspfile', 'rspfile_content', 'pool'}
    for rule in config.custom_build_rules:
        if set(rule) - rule_options - {'name'} or not rule.get('command'):
            raise ValueError('Invalid custom build rule')
        lines.append('rule ' + rule['name'] + R)
        for key, value in rule.items():
            if key != 'name' and value is not None:
                lines.append(f'  {key} = {int(value) if isinstance(value, bool) else value}')

    def paths(value):
        if value is None:
            return []
        return value if isinstance(value, (list, tuple)) else [value]

    phase_outputs = {}
    for phase, steps in config.custom_build_steps.items():
        phase_outputs[phase] = []
        for step in steps:
            if set(step) - {'outputs', 'rule', 'inputs', 'implicit', 'order_only', 'variables', 'implicit_outputs', 'pool', 'dyndep'}:
                raise ValueError('Invalid custom build step')
            outputs = paths(step['outputs'])
            if not outputs:
                raise ValueError('Custom build step needs outputs')
            phase_outputs[phase] += outputs
            edge = 'build ' + ' '.join(map(ninja_path, outputs))
            if step.get('implicit_outputs'):
                edge += ' | ' + ' '.join(map(ninja_path, paths(step['implicit_outputs'])))
            edge += ': ' + step['rule'] + R + ' ' + ' '.join(map(ninja_path, paths(step.get('inputs'))))
            if step.get('implicit'):
                edge += ' | ' + ' '.join(map(ninja_path, paths(step['implicit'])))
            order_only = list(paths(step.get('order_only')))
            previous = {'post-compile': P + 'converted', 'post-link': config.out_path() / 'Indy3D.exe',
                        'post-build': P + 'check'}.get(phase)
            if previous:
                order_only.append(previous)
            if order_only:
                edge += ' || ' + ' '.join(map(ninja_path, order_only))
            lines.append(edge)
            variables = dict(step.get('variables', {}))
            variables.update({k: step[k] for k in ('pool', 'dyndep') if k in step})
            lines += [f'  {k} = {v}' for k, v in variables.items()]
    for phase in ('pre-compile', 'post-compile', 'post-link', 'post-build'):
        lines.append(f'build {P}{phase}: phony ' + ' '.join(map(ninja_path, phase_outputs.get(phase, []))))
    return lines


def write_compile_commands(config, objects, includes):
    if not config.generate_compile_commands:
        return
    entries = []
    for obj in objects.values():
        if not obj.source.endswith('.c'):
            continue  # MASM sources (.asm) have no Clang view
        # Editor parsing only; MSVC/wibo remains the compiler used by Ninja.
        flags = ['clang', '-x', 'c', '-std=c89', '-m32', '-fms-extensions', '-fms-compatibility']
        flags += ['-I' + p for p in getattr(obj, 'includes', includes)]
        flags += ['-' + f[1:] for f in obj.cflags if f.startswith(('/D', '/U'))]
        output = config.out_path() / 'obj' / Path(obj.source).with_suffix('.obj')
        entries.append({'directory': str(config.root.resolve()), 'file': obj.source,
                        'output': str(output), 'arguments': flags + ['-c', obj.source, '-o', str(output)]})
    write_json(config.root / 'compile_commands.json', entries)


def calculate_progress(config):
    # Reconfigure first so selection, version and options cannot report stale progress.
    generate_build(config)
    subprocess.run([str(config.ninja_path), 'progress'], cwd=config.root, check=True)
    if config.progress_each_module:
        report = json.loads((config.root / config.out_path() / 'progress.json').read_text())
        completed = {u['source'] for u in report['source_units']}
        objects = config.objects().values()
        for category in config.progress_categories:
            units = [o for o in objects if o.progress_category == category.id]
            print(f'{category.name}: {sum(o.source in completed for o in units)}/{len(units)} complete TUs')


def generate_build(config):
    """Write build.ninja (and the editor/objdiff views of the primary version). Every version in
    `config.extra_versions` is built from the same graph under its own target prefix."""
    root = config.root.resolve()
    check_watch_tree(root)
    os.chdir(root)
    graphs = [version_graph(config, root, primary=True)]
    for extra in config.extra_versions:
        graphs.append(version_graph(extra, root, primary=False))
    python = sys.executable
    generated = ['build.ninja', 'objdiff.json', 'Libs/j3dcore/j3d.h']
    generated += [g['config_path'] for g in graphs]
    if config.generate_compile_commands:
        generated.append('compile_commands.json')
    inputs = [i for g in graphs for i in g['config_inputs']]
    lines = ['ninja_required_version = 1.10', 'rule configure',
             '  command = ' + command([python, 'configure.py', *config.configure_args]),
             '  generator = 1', '  restat = 1',
             'build ' + ' '.join(map(ninja_path, generated)) +
             ': configure configure.py tools/project.py tools/config.py tools/objdiff_watch.py Scripts/pe_project.py Libs/j3dcore/j3d.h.in ' +
             ' '.join(map(ninja_path, [*dict.fromkeys(inputs), *config.reconfig_deps]))]
    for g in graphs:
        lines += g['lines']
    write_changed(root / 'build.ninja', '\n'.join(lines) + '\n')
    for g in graphs:
        print(g['summary'])
    print('Run ninja to compile, split, rebuild and verify; ninja report generates objdiff metrics.')
    for extra in config.extra_versions:
        print(f'{extra.version}: ninja {extra.target_prefix}all (see README.md, "Versions").')


def write_debug_config(config, root, objects, overrides, runtime):
    """The configuration of the version's debug build (`ninja debug`, docs/public/DEBUG-BUILD.md): every unit of the
    Jones3D project (DEBUG_COMPILERS) with its release flags made unoptimised and asserts-on (debug_cflags: no
    /DNDEBUG, so J3D_DEBUG is defined), the separately built libraries with their release flags, and the debug-only
    sources (DEBUG_ONLY_SOURCES) with their sibling unit's configuration. Not byte-matched: no original is compared.
    Returns the debug build directory, {source: compiled file} and {source: unit configuration}."""
    debug_build = config.build_dir / (config.version + '-debug')
    units, sources = {}, {}
    for obj in objects.values():
        unit = dict(overrides[obj.source])
        if unit.get('compiler_name', 'VC5_SP3') in DEBUG_COMPILERS:
            unit['cflags'] = debug_cflags(unit['cflags'])
        units[obj.source] = unit
        sources[obj.source] = obj.input
    for source, (sibling, original) in DEBUG_ONLY_SOURCES.items():
        if sibling not in units or not (root / source).is_file():
            continue  # a project without the sibling unit (a test fixture) has no debug-only source either
        unit = {k: v for k, v in units[sibling].items() if k != 'source'}
        unit['cflags'] = [('/DJ3D_FILE=' + json.dumps(original)) if f.startswith('/DJ3D_FILE=') else f
                          for f in unit['cflags']]
        sources[source] = version_file(config.version, source, root)
        if sources[source] != source:
            unit['source'] = sources[source]
        units[source] = unit
    write_json(root / debug_build / 'config.json', {**runtime, 'units': units, 'sources': list(sources),
                                                   'converted': list(sources), 'matching_units': [], 'libraries': []})
    return debug_build, sources, units


def debug_graph(config, project, debug_build, sources, units, headers, wibo_dep):
    """`ninja debug`: the debug build's objects (write_debug_config), never part of `all`; `ninja debug-link` links
    them with the period linker under wine into Indy3D_debug.exe (tools/debuglink.py)."""
    P, R = config.target_prefix, config.rule_suffix
    debug_config = debug_build / 'config.json'
    python = sys.executable
    lines = ['rule cc_debug' + R,
             '  command = ' + command([python, 'Scripts/vc6_compile.py', '--config', debug_config]) + ' $in $out',
             '  description = DEBUG $compiler_name $in']
    deps = headers + [debug_config.as_posix(), 'Scripts/vc6_compile.py', wibo_dep.as_posix()]
    objs = []
    for source, compiled in sources.items():
        obj = debug_build / 'obj' / Path(source).with_suffix('.obj')
        objs.append(obj)
        lines.append(f'build {ninja_path(obj)}: cc_debug{R} {ninja_path(compiled)} | ' + ' '.join(map(ninja_path, deps)))
        lines.append('  compiler_name = ' + units[source].get('compiler_name', 'VC5_SP3'))
    exe = debug_build / 'Indy3D_debug.exe'
    icons = project.build / 'assets'
    lines += ['rule debug_link' + R,
              '  command = ' + command([python, 'tools/debuglink.py', '--version', config.version,
                                        '--build-dir', config.build_dir, '--icons', icons / 'icons']),
              '  description = LINK (wine) ' + exe.as_posix(),
              f'build {P}debug: phony ' + ' '.join(map(ninja_path, objs)),
              f'build {ninja_path(exe)}: debug_link{R} ' + ' '.join(map(ninja_path, objs)) + ' | ' +
              ' '.join(map(ninja_path, ['tools/debuglink.py', 'Jones3D/Indy3D.rc', icons / 'icons.stamp'])),
              f'build {P}debug-link: phony ' + ninja_path(exe)]
    return lines


def version_graph(config, root, primary):
    """The rules and edges of one version. The primary version keeps the plain target names; another
    version's phony targets carry its `target_prefix` and its rules its `rule_suffix`."""
    P, R = config.target_prefix, config.rule_suffix
    objects = config.objects()
    project = Project(config.version, root, config.build_dir,
                      [obj.source for obj in objects.values() if obj.completed])
    release_wibo = (root / config.wrapper).resolve()
    compiler = config.compiler_path or config.compilers_path / 'bin/cl.exe'
    compiler = (root / compiler).resolve()
    objdiff = (root / config.objdiff_path).resolve()
    if not release_wibo.exists() or not compiler.exists() or not objdiff.exists():
        raise ValueError('Missing toolchain. Run python3 Scripts/setup_toolchain.py first.')
    # Every compile runs under the heap-model wibo (docs/crt.md, "A principled 1999 heap model").
    wibo_dep = config.compile_wrapper
    if wibo_dep.is_absolute() and wibo_dep.resolve().is_relative_to(root):
        wibo_dep = wibo_dep.resolve().relative_to(root)
    wibo = (root / config.compile_wrapper).resolve()
    if not wibo.exists():
        raise ValueError(f'Missing the heap-model wibo {config.compile_wrapper}, which every compile runs under. '
                         'Build it with python3 Scripts/build_wibo_heapmodel.py (Scripts/setup_toolchain.py runs it), '
                         'or pass configure.py --wibo-heapmodel <path> to use another checkout\'s build.')
    compile_env = {**os.environ, **config.compile_env}
    banner = subprocess.run([str(wibo), str(compiler)], capture_output=True, text=True, env=compile_env)
    if banner.returncode or '11.00.7022' not in banner.stdout + banner.stderr:
        raise ValueError('wibo could not run MSVC 11.00.7022: ' + banner.stdout + banner.stderr)
    for name in {obj.compiler for obj in objects.values()} - {None}:
        tool = config.compilers[name]
        path = (root / tool['path']).resolve()
        if not path.exists():
            raise ValueError(f'Missing {name}. Run python3 Scripts/setup_toolchain.py first.')
        banner = subprocess.run([str(wibo), str(path)], capture_output=True, text=True, env=compile_env)
        # ICL prints its banner then exits 1 ("no files specified"); CL exits 0.
        if (banner.returncode and tool.get('kind') != 'icl') or tool['version'] not in banner.stdout + banner.stderr:
            raise ValueError(f'wibo could not run {name} {tool["version"]}: ' + banner.stdout + banner.stderr)
    sdk = root / 'Libs/external/DirectX61c/include'
    if not (sdk / 'd3d.h').exists():
        raise ValueError('Missing DirectX 6.1 SDK. Run python3 Scripts/setup_toolchain.py first.')
    sources = [obj.source for obj in objects.values()]
    converted = sources
    if set(project.units) - set(sources) - set(project.libraries) - {UNASSIGNED}:
        raise ValueError('splits.txt assigns ranges to sources missing from configure.py')
    overrides = {obj.source: {'cflags': obj.cflags, 'progress_category': obj.progress_category}
                 for obj in objects.values()}
    categories = {obj.source: obj.progress_category for obj in objects.values()}
    build = project.build
    config_path = build / 'config.json'
    runtime = {
        # `wibo` and `compiler_env` are what a compile runs under; `release_wibo` is the pinned release
        # binary, for tools that model the heap themselves (tools/icl/icl_heapenv.py, tools/probes/c2heapenv.py).
        'wibo': str(wibo), 'compiler_env': dict(config.compile_env), 'release_wibo': str(release_wibo),
        'compiler': str(compiler),
        'compiler_flags': ['/B1c1.dll', '/B2c2.exe'],
        'cflags': config.cflags,
        'units': overrides, 'sources': sources, 'converted': converted,
        # Library units (libraries.yml) are selected by the backend itself;
        # they are listed apart so they never count as compiled game TUs.
        'matching_units': [u for u in project.matching_units if u not in project.libraries],
        'libraries': sorted(project.libraries),
        'includes': [str(root), str(root / 'Libs'), str(sdk), str(compiler.parent.parent / 'include')],
    }
    for obj in objects.values():
        if obj.compiler is not None:
            tool = config.compilers[obj.compiler]
            path = (root / tool['path']).resolve()
            if tool.get('kind') == 'icl':
                # ICL's own SSE headers first, then the project, DirectX 6.1 and MSVC 5 headers the
                # game types need (ICL parses them as VC5 does).
                obj.includes = [str(path.parent.parent / tool['include_dir'])] + runtime['includes']
            else:
                obj.includes = runtime['includes'][:-1] + [str(path.parent.parent / tool.get('include_dir', 'include'))]
            overrides[obj.source].update(compiler=str(path), compiler_name=obj.compiler,
                                         compiler_flags=tool.get('flags', []), includes=obj.includes,
                                         compiler_kind=tool.get('kind', 'msvc'))
    if config.overlay_dir is not None:
        # A version's header overlays shadow the shared headers found through the include roots (the
        # mirrored root and Libs directories come first); an overlay source still finds the headers of
        # its shared directory, which the compiler would otherwise search first for a quoted include.
        roots = [root / config.overlay_dir / sub for sub in ('', 'Libs')]
        roots = [str(r) for r in roots if r.is_dir() and any(r.glob('**/*.h'))]
        for obj in objects.values():
            unit = overrides[obj.source]
            includes = roots + list(unit.get('includes', runtime['includes']))
            if obj.input != obj.source:
                unit['source'] = obj.input
                includes.append(str(root / Path(obj.source).parent))
            if includes != unit.get('includes', runtime['includes']):
                unit['includes'] = includes
    write_json(root / config_path, runtime)
    debug_build, debug_sources, debug_units = write_debug_config(config, root, objects, overrides, runtime)
    if primary:
        version_header = (root / 'Libs/j3dcore/j3d.h.in').read_text()
        for name, value in {'PROJECT_VERSION_MAJOR': '0', 'PROJECT_VERSION_MINOR': '3',
                            'PROJECT_VERSION_PATCH': '1', 'PROJECT_NAME': 'InfernalMachine'}.items():
            version_header = version_header.replace('@' + name + '@', value)
        write_changed(root / 'Libs/j3dcore/j3d.h', version_header)
    python = sys.executable
    backend = [python, '-m', 'Scripts.pebuild', '--version', config.version,
               '--build-dir', config.build_dir]
    # The objdiff report of the primary version reads objdiff.json; another version's reads the project
    # file written into its build directory (objdiff takes one project per directory).
    report_command = [objdiff, 'report', 'generate', '-p', '.' if primary else build, '-o', build / 'report.json',
                      '-f', 'json']
    lines = ['rule cc' + R,
             '  command = ' + command([python, 'Scripts/vc6_compile.py', '--config', config_path]) + ' $in $out',
             '  description = $compiler_name $in',
             'rule split' + R, '  command = ' + command([*backend, 'split']),
             f'  description = SPLIT {config.version} Indy3D.exe' if not primary else '  description = SPLIT Indy3D.exe',
             '  restat = 1',
             'rule target' + R, '  command = ' + command([*backend, 'target']) + ' --unit $unit',
             '  description = OBJDIFF $unit' + ('' if primary else f' ({config.version})'), '  restat = 1',
             'rule match' + R, '  command = ' + command([*backend, 'match']) + ' --unit $unit',
             '  description = MATCH TU $unit' + ('' if primary else f' ({config.version})'), '  restat = 1',
             'rule libextract' + R, '  command = ' + command([*backend, 'libextract']) + ' --unit $unit',
             '  description = LIBRARY $unit' + ('' if primary else f' ({config.version})'), '  restat = 1',
             'rule link' + R, '  command = ' + command([*backend, 'link']),
             '  description = REBUILD Indy3D.exe' + ('' if primary else f' ({config.version})'), '  restat = 1',
             'rule source_only' + R, '  command = ' + command([*backend, 'link', '--source-only']),
             '  description = REBUILD without original fallback' + ('' if primary else f' ({config.version})'),
             '  restat = 1',
             'rule verify' + R, '  command = ' + command([*backend, 'verify']),
             '  description = VERIFY Indy3D.exe' + ('' if primary else f' ({config.version})'),
             'rule freeze_layout' + R, '  command = ' + command([*backend, 'freeze-layout']),
             '  description = FREEZE layout.json' + ('' if primary else f' ({config.version})'),
             'rule progress' + R, '  command = ' + command([*backend, 'progress']),
             '  description = PROGRESS' + ('' if primary else f' ({config.version})'), '  restat = 1',
             'rule report' + R, '  command = ' + command(report_command),
             '  description = OBJDIFF REPORT' + ('' if primary else f' ({config.version})')]
    if primary:
        lines += ['rule objdiff_watch_check', '  command = ' + command([python, '-m', 'tools.objdiff_watch']),
                  '  description = CHECK objdiff watcher paths']
    if primary and not config.public:
        lines += ['rule test', '  command = ' + command([python, '-m', 'unittest', 'discover', '-s', 'tests', '-v']),
                  '  description = TEST PE/COFF backend',
                  # a paren-only source edit must not change an expression's parse (tools/parsediff.py);
                  # compares the working tree with its merge-base on origin/main, so run it before a push
                  'rule parsecheck', '  command = ' + command([python, 'tools/parsediff.py', '--base', 'origin/main']),
                  '  description = PARSECHECK paren-only edits against origin/main']
    if primary:
        # the Cog parser's generated sources from cog.y/cog.l/flex.skel (Scripts/cog_generators.py), in the public
        # graph too (optional: the generated sources are checked in): `ninja cog-generators` builds the period
        # byacc/flex, `ninja regen-cog` rewrites the sources
        lines += ['rule cog_generators', '  command = ' + command([python, 'Scripts/cog_generators.py', 'build']),
                  '  description = BUILD byacc 1.4 and flex 2.3.6',
                  'rule regen_cog', '  command = ' + command([python, 'Scripts/cog_generators.py', 'regen']),
                  '  description = REGENERATE sithCogYacc.c/.h and sithCogFlex.c']
    headers = sorted(p.relative_to(root).as_posix()
                     for pattern in ('Libs/**/*.h', 'Jones3D/**/*.h') for p in root.glob(pattern)
                     if 'external' not in p.parts)
    if config.overlay_dir is not None:
        headers += sorted(p.relative_to(root).as_posix() for p in (root / config.overlay_dir).glob('**/*.h'))
    inputs = {obj.source: obj.input for obj in objects.values()}
    modules = {}
    for source in sources:
        obj = project.obj('obj', source)
        deps = headers + [config_path.as_posix(), 'Scripts/vc6_compile.py', wibo_dep.as_posix()]
        lines.append(f'build {ninja_path(obj)}: cc{R} {ninja_path(inputs[source])} | ' + ' '.join(map(ninja_path, deps))
                     + f' || {P}pre-compile')
        lines.append('  compiler_name = ' + overrides[source].get('compiler_name', 'VC5_SP3'))
        module = categories[source]
        modules.setdefault(module, []).append(obj)
    for module, objs in modules.items():
        lines.append(f'build {P}{module}: phony ' + ' '.join(map(ninja_path, objs)))
    scripts = ['Scripts/pebuild.py', 'Scripts/pe_project.py', 'Scripts/coff.py', 'Scripts/pe_library.py',
               'Scripts/pe_link.py', 'Scripts/pe_image.py', 'Scripts/pe_objdiff.py', 'tools/config.py']
    backend_deps = scripts + project.config_inputs + [config_path.as_posix()] + list(map(str, config.split_deps))
    # A library unit's native object is its archive member, extracted unmodified; a vendor unit's
    # is its split object from the retail executable (Scripts/pe_library.py, image: true).
    from Scripts.pe_library import is_vendor
    library_objects = []
    for unit, entry in sorted(project.libraries.items()):
        obj = project.obj('obj', unit)
        library_objects.append(obj)
        source = project.obj('orig', unit) if is_vendor(entry) else entry['path']
        lines.append(f'build {ninja_path(obj)}: libextract{R} {ninja_path(source)} | ' +
                     ' '.join(map(ninja_path, backend_deps)))
        lines.append('  unit = ' + command([unit]))
    layout = build / 'layout.json'
    link_layout = Path(project.relative(project.link_layout))
    original_objects = project.original_objects
    lines.append('build ' + ' '.join(map(ninja_path, [layout, *original_objects])) + f': split{R} ' +
                 ninja_path(project.config['original']) + ' | ' + ' '.join(map(ninja_path, backend_deps)))
    targets = []
    bases = []
    units = []
    matches = []
    for unit in sorted(project.units):
        target = project.obj('target', unit)
        targets.append(target)
        deps = [layout, project.obj('orig', unit), config_path, *backend_deps]
        compiled = unit in converted or unit in project.libraries
        outputs = [target]
        if compiled:
            deps.append(project.obj('obj', unit))
            # Shared expected strings can live in any original TU. These are
            # outputs of the same split edge, so this adds no extra split work.
            deps += [p for p in original_objects if p != project.obj('orig', unit)]
            outputs += [project.obj('compare', unit), project.obj('compare', unit).with_suffix('.json')]
            bases.append(project.obj('compare', unit))
        if unit in project.matching_units:
            matched = project.obj('matched', unit)
            matches.append(matched)
            lines.append(f'build {ninja_path(matched)}: match{R} ' +
                         ' '.join(map(ninja_path, [link_layout, project.obj('obj', unit), *backend_deps])))
            lines.append('  unit = ' + command([unit]))
        # Comparisons must rebuild even when a selected TU regresses. Matching
        # and final image checks remain separate gates, never stale-view gates.
        lines.append('build ' + ' '.join(map(ninja_path, outputs)) + f': target{R} ' + ' '.join(map(ninja_path, deps)))
        lines.append('  unit = ' + command([unit]))
        module = (('vendor' if is_vendor(project.libraries[unit]) else 'library') if unit in project.libraries
                  else categories.get(unit, 'unassigned'))
        entry = {'name': unit, 'target_path': target.as_posix(),
                 'metadata': {'complete': unit in project.matching_units,
                              'progress_categories': [module]}}
        if unit != UNASSIGNED and unit not in project.libraries:
            entry['metadata']['source_path'] = unit
        if compiled:
            entry['base_path'] = project.obj('compare', unit).as_posix()
        units.append(entry)
    selected_objects = [project.obj('obj', unit) for unit in project.matching_units]
    fallback_objects = [project.obj('orig', unit) for unit in project.units if unit not in project.matching_units]
    fallback_objects.append(build / 'orig/__preserved__.obj')
    if project.config.get('layout'):
        # A frozen, byte-free layout allows reconstruction independently of the
        # extractor and original executable. Only actual fallback is an input.
        from Scripts.pe_image import load_layout
        frozen = load_layout(project)
        fallback_objects = sorted({Path(f['object']) for f in frozen['fragments']
                                   if f.get('source') not in project.matching_units
                                   and (f['file_offset'], f['size']) not in project.image_sources} |
                                  {Path(r['object']) for r in frozen['ranges']
                                   if r['source'] not in project.matching_units})
    image_inputs = sorted(set(project.image_sources.values()) | set(project.image_assets.values()))
    source_inputs = [link_layout, *selected_objects, *image_inputs, *backend_deps]
    # The source-only link reads the frozen layout (pe.source_layout) instead of the extracted one.
    source_only_inputs = [Path(project.relative(project.source_only_layout)), *source_inputs[1:]]
    objdiff_project = Path('objdiff.json') if primary else build / 'objdiff.json'
    lines += [
        'build ' + ninja_path(build / 'Indy3D.exe') + ' ' + ninja_path(build / 'link.json') + f': link{R} ' +
        ' '.join(map(ninja_path, [*source_inputs, *fallback_objects])) + f' || {P}post-compile',
        'build ' + ' '.join(map(ninja_path, [build / 'source-only/Indy3D.exe', build / 'source-only/link.json',
                                            build / 'source-only/verify.ok'])) + f': source_only{R} ' +
        ' '.join(map(ninja_path, source_only_inputs)) + f' || {P}post-compile',
        'build ' + ninja_path(build / 'verify.ok') + f': verify{R} ' +
        ' '.join(map(ninja_path, [build / 'Indy3D.exe', *backend_deps])) + f' || {P}post-link',
        'build ' + ninja_path(build / 'progress.json') + f': progress{R} ' +
        ' '.join(map(ninja_path, [build / 'verify.ok', build / 'link.json', config_path, *backend_deps])),
        'build ' + ninja_path(build / 'report.json') + f': report{R} {ninja_path(objdiff_project)} ' +
        # Both objdiff views: the target edge rewrites only the files whose bytes changed (restat), so
        # a presentation change that touches only the base view must still regenerate the report.
        ' '.join(map(ninja_path, [*targets, *bases, *(project.obj('obj', p) for p in converted), *library_objects])),
        f'build {P}split: phony ' + ninja_path(layout),
        f'build {P}freeze-layout: freeze_layout{R} ' + ninja_path(layout),
        f'build {P}converted: phony ' + ' '.join(ninja_path(p) for p in
                                                [*(project.obj('obj', u) for u in converted), *library_objects]),
        f'build {P}all-sources: phony ' + ' '.join(ninja_path(project.obj('obj', p)) for p in sources),
        *(['build objdiff-watch-check: objdiff_watch_check'] if primary else []),
        f'build {P}objdiff: phony objdiff-watch-check ' + ' '.join(map(ninja_path, targets)),
        f'build {P}check: phony ' + ninja_path(build / 'verify.ok'),
        f'build {P}match: phony ' + ' '.join(map(ninja_path, matches)),
        f'build {P}source-only: phony ' + ninja_path(build / 'source-only/verify.ok'),
        f'build {P}progress: phony ' + ninja_path(build / 'progress.json'),
        f'build {P}report: phony ' + ninja_path(build / 'report.json'),
        *([] if config.public or not primary else ['build test: test', 'build parsecheck: parsecheck']),
        *(['build cog-generators: cog_generators', 'build regen-cog: regen_cog'] if primary else []),
        f'build {P}all: phony {P}converted {P}objdiff {P}check {P}post-build' + (f' {P}progress' if config.progress else '')
        + (f' {P}map' if config.generate_map else ''),
        *(['default all'] if primary else [f'build {P.rstrip("-")}: phony {P}all']),
    ]
    project_file = {
        '$schema': 'https://raw.githubusercontent.com/encounter/objdiff/v3.5.1/config.schema.json',
        'min_version': '3.5.1', 'custom_make': str(config.ninja_path),
        'build_target': True, 'build_base': True,
        # Comparison copies use a common string order; raw MSVC objects feed the linker.
        'options': {'combineTextSections': True, 'combineDataSections': True},
        'watch_patterns': ['Libs/**/*.c', 'Libs/**/*.h', 'Jones3D/**/*.c', 'Jones3D/**/*.h',
                           'config/**/*', 'Scripts/**/*.py', 'tools/**/*.py', 'configure.py'],
        'units': units,
        'progress_categories': [{'id': m, 'name': next((c.name for c in config.progress_categories if c.id == m), m)}
                                for m in sorted({c for u in units for c in u['metadata']['progress_categories']})],
    }
    if not primary:
        # objdiff resolves a project's paths from its own directory: this version's report reads a
        # project file in its build directory (the GUI keeps the primary version's objdiff.json).
        def local(path):
            return os.path.relpath(root / path, root / build)
        project_file = {**project_file, 'build_target': False, 'build_base': False, 'watch_patterns': [],
                        'units': [{**u, 'target_path': local(u['target_path']),
                                   **({'base_path': local(u['base_path'])} if 'base_path' in u else {})}
                                  for u in units]}
        del project_file['custom_make']
    write_json(root / objdiff_project, project_file)
    lines += ['rule map' + R, '  command = ' + command([*backend, 'map']),
              '  description = MAP Indy3D.exe' + ('' if primary else f' ({config.version})'), '  restat = 1',
              'build ' + ninja_path(build / 'Indy3D.map') + f': map{R} ' +
              ' '.join(map(ninja_path, [link_layout, build / 'link.json', build / 'verify.ok', *backend_deps])),
              f'build {P}map: phony ' + ninja_path(build / 'Indy3D.map')]
    ownership_review = project.directory / 'ownership.yml'
    if ownership_review.exists() and not config.public:
        audit_outputs = [build / 'ownership.json', build / 'ownership.md']
        audit_inputs = [project.config['original'], project.relative(ownership_review),
                        'Scripts/ownership.py', *backend_deps,
                        *(project.obj('obj', p) for p in sources), *library_objects]
        lines += ['rule audit' + R, '  command = ' + command([
                      python, '-m', 'Scripts.ownership', '--version', config.version,
                      '--build-dir', config.build_dir]),
                  '  description = AUDIT TU ownership', '  restat = 1',
                  'build ' + ' '.join(map(ninja_path, audit_outputs)) + f': audit{R} ' +
                  ' '.join(map(ninja_path, audit_inputs)),
                  f'build {P}audit: phony ' + ' '.join(map(ninja_path, audit_outputs))]
    lines += debug_graph(config, project, debug_build, debug_sources, debug_units, headers, wibo_dep)
    lines += custom_build(config)
    if primary:
        write_compile_commands(config, objects, runtime['includes'])
    summary = (f'Configured {config.version}: {len(converted)}/{len(sources)} compiled sources, '
               f'{len(project.functions)} reference functions, {len(project.data_symbols)} data symbols, '
               f'{len(project.matching_units) - len(project.libraries)} selected matching TUs, '
               f'{len(project.libraries)} library members.')
    if not (root / project.config['original']).exists():
        summary += (f"\nOriginal executable {project.config['original']} missing: ninja {config.target_prefix}converted "
                    f"is available; pass it with --exe to setup_toolchain.py for the {config.version} build.")
    config_inputs = list(project.config_inputs)
    if config.overlay_dir is not None and (root / config.overlay_dir / 'overlays.yml').is_file():
        # the overlay registry lists every overlay, so adding or removing one regenerates the graph
        config_inputs.append((Path(config.overlay_dir) / 'overlays.yml').as_posix())
    return {'lines': lines, 'config_path': config_path.as_posix(), 'config_inputs': config_inputs,
            'summary': summary}
