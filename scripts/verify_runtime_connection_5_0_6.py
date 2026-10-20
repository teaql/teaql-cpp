#!/usr/bin/env python3
"""Compare actual SQLite connection outcomes and files with original startup.

This deliberately observes the connection boundary only. The original successful
post-startup generated query_error remains in its source record; generated
metadata/executor/query assembly requires a separate test.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile

from verify_runtime_configuration_kernel_5_0_6 import verify_inputs, same

ROOT = Path(__file__).resolve().parents[1]


def observe(executable, arguments, environment):
    with tempfile.TemporaryDirectory(prefix='teaql_runtime_connection_') as directory:
        root = Path(directory)
        (root / 'blocked').write_text('A regular file, not a directory.\n')
        result = subprocess.run([str(executable.resolve()), *arguments], cwd=root,
                                env=environment, capture_output=True, check=True)
        files = sorted(str(path.relative_to(root)) for path in root.rglob('*')
                       if path.is_file() and path.name != 'blocked')
        return json.loads(result.stdout), files


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', type=Path, required=True)
    parser.add_argument('--rust', type=Path)
    parser.add_argument('--assembly', action='store_true',
                        help='Require the full original generated post-startup query outcome too')
    args = parser.parse_args()
    source = json.loads((ROOT / 'docs/parity/runtime_configuration_original_observations_5_0_6.json').read_text())
    verify_inputs(source)
    total = 0
    for case in source['cases']:
        if case['mode'] != 'connect':
            continue
        environment = dict(os.environb)
        environment.pop(b'CPP_DYNAMIC_JSON_CORE_DATABASE_URL', None)
        environment[b'TEAQL_LOG_ENDPOINT'] = b'off'
        if case['environment_hex'] is not None:
            environment[b'CPP_DYNAMIC_JSON_CORE_DATABASE_URL'] = bytes.fromhex(case['environment_hex'])
        if args.rust:
            result, files = observe(args.rust, ['connect'], environment)
            assert same(result, case['source_expected']) and files == case['created_files'], case['name'] + ': original changed'
        result, files = observe(args.cpp, [], environment)
        connection_outcome = (case['source_expected'] if args.assembly else
                              {key: value for key, value in case['source_expected'].items()
                               if key != 'query_error'})
        assert same(result, connection_outcome) and files == case['created_files'], (case['name'], result, connection_outcome, files)
        total += 1
    assert total == 13
    if args.assembly:
        print('Verified13 complete original/native generated runtime startup/query outcomes and files')
    else:
        print('Verified13 exact original/native connection outcomes and files; generated runtime/query assembly remains separate')


if __name__ == '__main__':
    main()
