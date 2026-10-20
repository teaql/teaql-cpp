#!/usr/bin/env python3
"""Build the independent composition observer from terminal native archives."""
import argparse
import json
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, default=ROOT / 'build_native_current')
    parser.add_argument('--output', type=Path, default=Path('/tmp/teaql_derived_query_composition_native'))
    parser.add_argument('--modules', action='store_true',
                        help='Reuse the terminal canonical module consumer and module maps')
    args = parser.parse_args()
    build = args.build_dir.resolve()
    entries = json.loads((build / 'compile_commands.json').read_text())
    suffix = ('/consumers/derived_query_diagnostics_probe.cpp' if args.modules
              else '/tests/parity/derived_query_diagnostics_probe.cpp')
    entry = next(x for x in entries if x['file'].endswith(suffix))
    command = list(entry.get('arguments') or shlex.split(entry['command']))
    for flag in ('-MD', '-MT', '-MF', '-o'):
        while flag in command:
            index = command.index(flag)
            del command[index:index + (1 if flag == '-MD' else 2)]
    command.remove('-c')
    command.remove(entry['file'])
    archives = [build / package / f'lib{package}.a'
                for package in ('teaql_provider_sqlite', 'teaql_runtime', 'teaql_sql',
                                'teaql_data_service', 'teaql_core', 'teaql_memory')]
    for archive in archives:
        if not archive.is_file():
            raise SystemExit(f'Build the base diagnostic probe first; missing archive: {archive}')
    if args.modules:
        module_archives = [build / 'modules' / f'lib{package}_module.a'
                           for package in ('teaql_provider_sqlite', 'teaql_runtime', 'teaql_sql',
                                           'teaql_data_service', 'teaql_core', 'teaql_async', 'teaql_memory')]
        for archive in module_archives:
            if not archive.is_file():
                raise SystemExit(f'Build the canonical module diagnostic probe first: {archive}')
        archives = module_archives + archives
    source = ROOT / 'tests/parity/derived_query_composition_probe.cpp'
    command += [str(source),
                '-o', str(args.output.resolve()), '-Wl,--start-group',
                *(str(path) for path in archives), '-Wl,--end-group',
                str(ROOT / '.tools/toolchains/sqlite-3.53.2/lib_native/libsqlite3.a'),
                '-lpthread' if args.modules else '-pthread', '-ldl', '-lm']
    if args.modules:
        with tempfile.TemporaryDirectory(prefix='teaql_composition_module_') as directory:
            consumer = Path(directory) / 'composition.cpp'
            text = source.read_text()
            old_include = '#include "derived_query_diagnostics_probe.cpp"'
            if text.count(old_include) != 1:
                raise SystemExit('Requalify the canonical observer include adaptation')
            consumer.write_text(text.replace(old_include, f'#include "{entry["file"]}"'))
            command[command.index(str(source))] = str(consumer)
            subprocess.run(command, cwd=entry['directory'], check=True)
    else:
        subprocess.run(command, cwd=entry['directory'], check=True)
    print(f'Compiled and linked {args.output.resolve()}')


if __name__ == '__main__':
    main()
