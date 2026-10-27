#!/usr/bin/env python3
"""Qualify graph owner mechanics before thin generated forward-owner metadata."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
TARGETS = {
    'entity_runtime_contracts': 'teaql_entity_runtime_tests',
    'generated_relation_contracts': 'teaql_generated_relation_tests',
    'generated_relations_rust_parity': 'teaql_loaded_snapshot_probe',
    'relation_identity_rust_parity': 'teaql_relation_identity_probe',
    'relation_identity_generated_contracts': 'teaql_relation_identity_tests',
    'relation_identity_kernel_contracts': 'teaql_relation_identity_kernel_tests',
    'runtime_entity_expression_contracts': 'teaql_runtime_expression_tests',
    'generated_graph_serialization_contracts': 'teaql_generated_graph_serialization_tests',
    'generated_runtime_state_bridge_rust_parity': 'teaql_generated_runtime_state_bridge_probe',
    'expression_root_rust_parity': 'teaql_expression_root_probe',
}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path, required=True)
    parser.add_argument('--stage', choices=['kernel', 'installed'], required=True)
    args = parser.parse_args()
    assert not args.record.exists(), 'Preserve previous receipts'
    names = ['entity_runtime_contracts'] if args.stage == 'kernel' else list(TARGETS)
    modules = ['module_entity_runtime_contracts']
    if args.stage == 'installed':
        modules += ['module_relation_identity_generated_contracts',
                    'module_relation_identity_kernel_contracts',
                    'module_relation_identity_rust_parity']
    inputs = [Path(__file__).resolve(), ROOT / 'teaql_runtime/tests/entity_runtime_tests.cpp',
              ROOT / 'teaql_runtime/include/teaql/runtime/entity_runtime.hpp',
              ROOT / 'teaql_runtime/src/entity_runtime.cpp', ROOT / 'toolchains.lock.json']
    record = dict(complete=False, stage=args.stage, steps=[], profiles=[],
                  free_bytes_before=shutil.disk_usage(ROOT).free,
                  input_hashes_before={str(p): digest(p) for p in inputs},
                  scope='Native and imported tests; production runtime/lifetime mechanics unchanged; no sanitizer/MIPS/full-suite rerun')
    def run(label, command):
        result = subprocess.run(list(map(str, command)), cwd=ROOT, text=True, capture_output=True)
        record['steps'].append(dict(label=label, command=list(map(str, command)), exit_code=result.returncode,
                                    stdout=result.stdout, stderr=result.stderr))
        print(label, 'exit', result.returncode, flush=True)
        result.check_returncode()
        return result.stdout
    try:
        assert shutil.disk_usage(ROOT).free > 1024 ** 3
        for preset, selected in [('native', names), ('modules_native', modules)]:
            pattern = '^(' + '|'.join(selected) + ')$'
            tests = json.loads(run(preset + '/selection', ['ctest', '--preset', preset,
                               '--show-only=json-v1', '-R', pattern]))['tests']
            assert {t['name'] for t in tests} == set(selected)
            targets = sorted({TARGETS[name.removeprefix('module_')] for name in selected})
            if preset == 'modules_native':
                targets = [t.replace('teaql_', 'teaql_module_', 1) for t in targets]
            run(preset + '/build', ['cmake', '--build', '--preset', preset, '--target', *targets, '-j', '4'])
            run(preset + '/test', ['ctest', '--preset', preset, '-R', pattern])
            record['profiles'].append(dict(preset=preset, tests=selected, targets=targets, terminal_exit=0))
        record['input_hashes_after'] = {str(p): digest(p) for p in inputs}
        assert record['input_hashes_before'] == record['input_hashes_after']
        record['complete'] = True
    finally:
        args.record.write_bytes(gzip.compress(json.dumps(record).encode(), mtime=0))


if __name__ == '__main__':
    main()
