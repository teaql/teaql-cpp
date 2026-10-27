#!/usr/bin/env python3
"""Reject error-source receipt, corpus, current-file and CTest substitutions."""

import argparse
import copy
import gzip
import json
from pathlib import Path
import subprocess
import sys
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
import runtime_error_cli_source_binding_5_0_6 as binding


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, default=ROOT / 'build_native_current')
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    if args.record is not None and args.record.exists():
        parser.error('Preserve historical source-control evidence')
    validator = binding.Validator(binding.read(binding.REGISTRY))
    validator.validate_files()
    validator.validate_source()
    validator.validate_ctest(args.build_dir)
    controls = []

    def reject(label, action):
        try:
            action()
        except (RuntimeError, ValueError, KeyError, TypeError, IndexError) as error:
            controls.append(dict(name=label, rejected=True, reason=str(error)))
        else:
            raise AssertionError('Error-source binding accepted ' + label)

    mutations = [
        ('numeric_complete', lambda row: row.update(complete=1)),
        ('foreign_baseline', lambda row: row.update(baseline='0' * 40)),
        ('missing_original_step', lambda row: row['steps'].pop(4)),
        ('failed_original_step', lambda row: row['steps'][4].update(exit_code=1)),
        ('unlocked_build', lambda row: row['steps'][1]['command'].remove('--locked')),
        ('missing_original_row', lambda row: row['steps'][4].update(
            stdout=row['steps'][4]['stdout'].split('\n', 1)[1])),
        ('changed_original_row', lambda row: row['matrices'][0]['original_outputs'][0].update(
            invented='corruption')),
        ('missing_matrix', lambda row: row['matrices'].pop()),
        ('numeric_matrix_count', lambda row: row['matrices'][0].update(observations=True)),
        ('missing_trait_test', lambda row: row.update(original_trait_tests=2)),
        ('missing_compile_refusal', lambda row: row.update(original_compile_refusals=3)),
        ('module_scope', lambda row: row['profiles']['modules_native'].update(passed=5)),
    ]
    for label, mutate in mutations:
        changed = copy.deepcopy(validator.execution)
        mutate(changed)
        reject(label, lambda changed=changed: validator.validate_source(changed))

    corpus_path = ROOT / validator.registry['contracts'][0]['corpus']
    real_read = binding.read
    for label in ('extra_case_field', 'missing_case', 'changed_expected'):
        altered = copy.deepcopy(real_read(corpus_path))
        if label == 'extra_case_field':
            altered['cases'][0]['unreviewed'] = True
        elif label == 'missing_case':
            altered['cases'].pop()
        else:
            altered['cases'][0]['expected'] = {'unreviewed': True}

        def fake_read(path, altered=altered):
            return altered if Path(path) == corpus_path else real_read(path)

        with patch.object(binding, 'read', side_effect=fake_read):
            reject(label, validator.validate_source)

    tests = json.loads(subprocess.check_output(
        ['ctest', '--test-dir', str(args.build_dir), '--show-only=json-v1'],
        text=True))['tests']
    for label in ('foreign_python', 'wrong_binary', 'extra_argument',
                  'missing_source_fixture', 'duplicate_entry'):
        changed = copy.deepcopy(tests)
        chosen = next(row for row in changed if row['name'] == 'runtime_error_source_rust_parity')
        if label == 'foreign_python':
            chosen['command'][0] = '/invented/python'
        elif label == 'wrong_binary':
            chosen['command'][-2] = '/invented/native'
        elif label == 'extra_argument':
            chosen['command'].append('--rust=/unqualified')
        elif label == 'missing_source_fixture':
            chosen['properties'] = []
        else:
            duplicate = copy.deepcopy(chosen)
            duplicate['name'] = 'unqualified_error_comparison'
            changed.append(duplicate)
        reject(label, lambda changed=changed: validator.validate_commands(changed, args.build_dir))

    registry = copy.deepcopy(validator.registry)
    registry['original_files'][next(iter(registry['original_files']))] = '0' * 64
    reject('current_original_file', lambda: binding.Validator(registry).validate_files())
    assert len(controls) == 21 and all(item['rejected'] for item in controls)
    result = dict(complete=True, whole_goal_complete=False, controls=controls,
                  registry_sha256=binding.digest(binding.REGISTRY))
    if args.record:
        args.record.write_bytes(gzip.compress((json.dumps(result, indent=2) + '\n').encode(),
                                              mtime=0, compresslevel=3))
    print('Runtime-error source controls:', len(controls),
          'substitutions rejected; complete source/CTest accepted')


if __name__ == '__main__':
    main()
