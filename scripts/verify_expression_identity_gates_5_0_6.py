#!/usr/bin/env python3
"""Run focused canonical E-root gates without a full-suite or MIPS run."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path, required=True)
    parser.add_argument('--stage', choices=['kernel', 'installed'], required=True)
    args = parser.parse_args()
    if args.record.exists():
        parser.error('Preserve earlier receipts; choose a fresh path')
    names = ['runtime_entity_expression_contracts', 'expression_entry_contracts']
    if args.stage == 'installed':
        names += ['expression_root_rust_parity', 'generated_expression_diagnostics_rust_parity',
                  'expression_diagnostic_framing_rust_parity']
    profiles = [('native', names), ('sanitize', ['runtime_entity_expression_contracts']),
                ('modules_native', ['module_runtime_entity_expression_contracts'])]
    if args.stage == 'installed':
        profiles[-1][1].extend(['module_expression_entry_contracts', 'module_expression_root_rust_parity'])
    inputs = [ROOT / 'teaql_runtime/include/teaql/runtime/entity_expression.hpp',
              ROOT / 'teaql_runtime/tests/entity_expression_tests.cpp',
              ROOT / 'tests/generator/expression_entry_tests.cpp',
              ROOT / 'toolchains.lock.json', Path(__file__).resolve()]
    record = dict(complete=False, stage=args.stage, steps=[], profiles=[],
                  free_bytes_before=shutil.disk_usage(ROOT).free,
                  input_hashes_before={str(path): digest(path) for path in inputs})

    def run(label, command):
        result = subprocess.run(list(map(str, command)), cwd=ROOT, text=True, capture_output=True)
        record['steps'].append(dict(label=label, command=list(map(str, command)), exit_code=result.returncode,
                                    stdout=result.stdout, stderr=result.stderr))
        print(label, 'exit', result.returncode, flush=True)
        result.check_returncode()
        return result.stdout

    try:
        assert shutil.disk_usage(ROOT).free > 1024 ** 3
        for preset, selected_names in profiles:
            pattern = '^(' + '|'.join(selected_names) + ')$'
            tests = json.loads(run(preset + '/selection',
                ['ctest', '--preset', preset, '--show-only=json-v1', '-R', pattern]))['tests']
            assert {test['name'] for test in tests} == set(selected_names)
            # ctest JSON omits an unbuilt executable's command. Re-select after
            # building the exact known permanent targets; never consume outputs early.
            targets = {'runtime_entity_expression_contracts': 'teaql_runtime_expression_tests',
                       'expression_entry_contracts': 'teaql_expression_entry_tests',
                       'expression_root_rust_parity': 'teaql_expression_root_probe',
                       'generated_expression_diagnostics_rust_parity': 'teaql_generated_expression_diagnostics_probe',
                       'expression_diagnostic_framing_rust_parity': 'teaql_expression_diagnostic_text_probe'}
            requested = [targets[name.removeprefix('module_')] for name in selected_names]
            if preset == 'modules_native':
                requested = [name.replace('teaql_', 'teaql_module_', 1) for name in requested]
            run(preset + '/build', ['cmake', '--build', '--preset', preset, '--target', *requested, '-j', '4'])
            run(preset + '/test', ['ctest', '--preset', preset, '-R', pattern])
            record['profiles'].append(dict(preset=preset, tests=selected_names, targets=requested, terminal_exit=0))
        record['input_hashes_after'] = {str(path): digest(path) for path in inputs}
        assert record['input_hashes_before'] == record['input_hashes_after']
        record['complete'] = True
    finally:
        record['free_bytes_after'] = shutil.disk_usage(ROOT).free
        args.record.write_bytes(gzip.compress(json.dumps(record).encode(), mtime=0))


if __name__ == '__main__':
    main()
