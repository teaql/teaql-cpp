#!/usr/bin/env python3
"""Rebuild and run the focused generated-creation integration gates."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
NATIVE = (
    'runtime_entity_query_contracts', 'generated_api_contracts',
    'generated_return_type_contracts', 'generated_return_type_rust_parity',
    'generated_inheritance_save_rust_parity', 'entity_helper_entries_context_contracts',
    'entity_helper_entries_rust_parity', 'query_result_marker_runtime_rust_parity',
    'generated_sample_save_async_contracts', 'generated_api_type_errors',
)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    if args.record.exists():
        parser.error('Choose a fresh receipt; preserve earlier evidence')
    inputs = [Path(__file__).resolve(), ROOT / 'teaql_runtime/include/teaql/runtime/entity_query.hpp',
              ROOT / 'teaql_runtime/tests/entity_query_tests.cpp',
              ROOT / 'scripts/regenerate_creation_identity_5_0_6.py']
    update_path = ROOT / 'docs/parity/generated_creation_id_sdk_update_5_0_6.json.gz'
    update = json.loads(gzip.decompress(update_path.read_bytes()))
    assert update['complete'] and len(update['sdks']) == 80
    inputs += [ROOT / row['path'] / name for row in update['sdks'] for name in row['after']]
    record = dict(complete=False, scope=__doc__, steps=[], profiles=[],
                  sdk_update_sha256=digest(update_path), free_bytes_before=shutil.disk_usage(ROOT).free,
                  input_hashes_before={str(path): digest(path) for path in inputs})

    def save():
        args.record.write_bytes(gzip.compress(json.dumps(record).encode(), mtime=0))

    def run(label, command):
        result = subprocess.run(list(map(str, command)), cwd=ROOT, text=True, capture_output=True)
        record['steps'].append(dict(label=label, command=list(map(str, command)),
                                    exit_code=result.returncode, stdout=result.stdout, stderr=result.stderr))
        save()
        print(label, 'exit', result.returncode, flush=True)
        result.check_returncode()
        return result.stdout

    for preset, names in [('native', NATIVE),
                          ('sanitize', ('runtime_entity_query_contracts',)),
                          ('modules_native', ('module_runtime_entity_query_contracts',))]:
        pattern = '^(' + '|'.join(names) + ')$'
        selected = json.loads(run(preset + '/selection',
            ['ctest', '--preset', preset, '--show-only=json-v1', '-R', pattern]))['tests']
        assert {test['name'] for test in selected} == set(names)
        targets = sorted({Path(arg).name for test in selected for arg in test['command']
                          if str(ROOT / 'build_') in arg and Path(arg).name.startswith('teaql_')})
        assert targets
        run(preset + '/build', ['cmake', '--build', '--preset', preset, '--target', *targets, '-j', '4'])
        run(preset + '/test', ['ctest', '--preset', preset, '-R', pattern])
        record['profiles'].append(dict(preset=preset, tests=list(names), targets=targets,
                                       test_count=len(names), terminal_exit=0))
    record['input_hashes_after'] = {str(path): digest(path) for path in inputs}
    assert record['input_hashes_before'] == record['input_hashes_after']
    record['complete'] = True
    record['free_bytes_after'] = shutil.disk_usage(ROOT).free
    save()


if __name__ == '__main__':
    main()
