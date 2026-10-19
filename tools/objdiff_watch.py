"""Detect ancestor symlinks that make objdiff's file watcher scan outside a checkout.

Objdiff 3.5.1 and 3.7.3 initialize notify's recursive file-ID cache on the
GUI thread. That cache follows directory symlinks before watch/ignore patterns
filter change events. In particular, a Wine prefix's dosdevices/z: -> / can
send it through the whole filesystem, even inside ignored build directories.
"""
import argparse
import os
from pathlib import Path
import sys


def check_watch_tree(root):
    """Check physical directories, including ignored ones, without following links.

    This targets ancestor links and unresolvable symlink loops. Ordinary file
    links and links to separate dependency directories remain supported; their
    external contents are not inspected by this check.
    """
    root = Path(root).resolve(strict=True)
    if not root.is_dir():
        raise ValueError(f'Not a project directory: {root}')

    def scan_error(error):
        raise error

    problems = []
    for directory, dirs, files in os.walk(root, followlinks=False, onerror=scan_error):
        for name in dirs + files:
            path = Path(directory) / name
            if not path.is_symlink():
                continue
            try:
                target = path.resolve(strict=True)
            except FileNotFoundError:
                continue
            except (OSError, RuntimeError) as error:
                problems.append(f'{path.relative_to(root)}: cannot resolve symlink ({error})')
                continue
            if target in path.parents:
                problems.append(f'{path.relative_to(root)} -> {target}')
    if problems:
        raise ValueError(
            'Unsafe objdiff watcher paths:\n  ' + '\n  '.join(sorted(problems)) + '\n'
            'Objdiff follows ancestor directory symlinks while initializing its watcher, '
            'before applying ignore patterns, which can freeze the GUI. '
            'Move Wine prefixes outside the checkout without leaving a directory symlink '
            'back to them; remove or relocate other ancestor links before opening objdiff. '
            'No files have been changed.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('project', nargs='?', type=Path, default=Path.cwd())
    args = parser.parse_args()
    try:
        check_watch_tree(args.project)
    except (ValueError, OSError) as error:
        print(f'error: {error}', file=sys.stderr)
        return 1
    print('Objdiff watcher check passed (no ancestor directory symlinks).')
    return 0


if __name__ == '__main__':
    sys.exit(main())
