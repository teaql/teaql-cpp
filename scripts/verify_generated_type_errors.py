#!/usr/bin/env python3
"""Require real compiler rejection of invalid generated C++ application calls."""
import argparse
import json
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, required=True)
    args = parser.parse_args()
    build = args.build_dir.resolve()
    commands = json.loads((build / 'compile_commands.json').read_text())
    record = next(row for row in commands
                  if row['file'].endswith('/tests/generator/generated_api_tests.cpp'))
    original = record.get('arguments') or shlex.split(record['command'])
    command = []
    skip = False
    for index, part in enumerate(original):
        if skip:
            skip = False
            continue
        if part == '-o':
            skip = True
            continue
        if part == '-c' or part == record['file']:
            continue
        command.append(part)
    command.append('-fsyntax-only')
    cases = {
        'positive': ('Q::orders(user).comment("what").purpose("why").execute_for_list(user);', True, ''),
        'missing_purpose': ('Q::orders(user).comment("what").execute_for_list(user);', False, 'execute_for_list'),
        'missing_audit': ('user.make<Order>().save(user);', False, 'save'),
        'unknown_field': ('Q::orders(user).with_nmae_is("x");', False, 'with_nmae_is'),
        'wrong_scalar': ('Q::orders(user).with_id_is("not an ID");', False, 'with_id_is'),
        'wrong_relation': ('Q::orders(user).select_lines_with(Q::products(user));', False, 'select_lines_with'),
        'extra_runtime_argument': ('Q::orders(user).purpose("why").execute_for_list(user, 1);', False, 'execute_for_list'),
        'global_builder': ('GlobalContext global; Q::orders(global);', False, 'GlobalContext'),
    }
    with tempfile.TemporaryDirectory(prefix='teaql_cpp_type_errors_') as directory:
        source = Path(directory) / 'consumer.cpp'
        for name, (body, succeeds, diagnostic) in cases.items():
            source.write_text('#include "' + str(ROOT / 'examples/generated/include/cpp_examples.hpp') + '"\n'
                              'using namespace cpp_examples;\n'
                              'void contract(UserContext& user) { ' + body + ' }\n')
            result = subprocess.run([*command, str(source)], cwd=record['directory'],
                                    text=True, capture_output=True)
            if (result.returncode == 0) != succeeds:
                raise SystemExit(f'Unexpected compiler outcome for {name}:\n{result.stdout}{result.stderr}')
            if not succeeds and diagnostic not in result.stderr:
                raise SystemExit(f'Unrelated compiler failure for {name}:\n{result.stderr}')
            print(f'Verified {name}', flush=True)
    print('Verified one positive and seven negative generated API compiler contracts')


if __name__ == '__main__':
    main()
