#!/usr/bin/env python3
"""Reject in-memory source, retained-corpus, current-file and CTest substitutions."""

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
import inmemory_cli_source_binding_5_0_6 as binding


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
            raise AssertionError('In-memory source binding accepted ' + label)

    mutations = [
        ('numeric_complete', lambda row: row.update(complete=1)),
        ('missing_original_step', lambda row: row['steps'].pop(3)),
        ('failed_original_step', lambda row: row['steps'][3].update(exit_code=1)),
        ('unlocked_build', lambda row: row['steps'][2]['command'].remove('--locked')),
        ('missing_raw_row', lambda row: row['steps'][3].update(
            stdout=row['steps'][3]['stdout'].split('\n', 1)[1])),
        ('changed_matrix_input', lambda row: row['matrices'][0]['inputs'][0].update(
            unreviewed=True)),
        ('wrong_matrix_count', lambda row: row['matrices'][0].update(observations=6531)),
        ('missing_matrix', lambda row: row['matrices'].pop()),
        ('wrong_original_step', lambda row: row['matrices'][0]['outputs']['original'].update(step=4)),
        ('unreviewed_difference', lambda row: row['matrices'][0]['differences'].append(0)),
        ('missing_original_test', lambda row: row['original_tests'].pop()),
    ]
    for label, mutate in mutations:
        changed = copy.deepcopy(validator.source)
        mutate(changed)
        reject(label, lambda changed=changed: validator.validate_source(changed))

    path = ROOT / validator.registry['corpus']
    real_read = binding.read
    changed = copy.deepcopy(real_read(path))
    changed['cases'][0]['unreviewed'] = True
    with patch.object(binding, 'read', side_effect=lambda value:
                      changed if Path(value) == path else real_read(value)):
        reject('extra_retained_case_field', validator.validate_source)

    tests = json.loads(subprocess.check_output(
        ['ctest', '--test-dir', str(args.build_dir), '--show-only=json-v1'],
        text=True))['tests']
    for label in ('foreign_python', 'wrong_binary', 'extra_argument',
                  'missing_source_fixture', 'duplicate_entry'):
        changed = copy.deepcopy(tests)
        chosen = next(row for row in changed if row['name'] == 'inmemory_rust_parity')
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
            duplicate['name'] = 'unqualified_inmemory_comparison'
            changed.append(duplicate)
        reject(label, lambda changed=changed: validator.validate_commands(changed, args.build_dir))

    registry = copy.deepcopy(validator.registry)
    registry['original_files'][next(iter(registry['original_files']))] = '0' * 64
    reject('changed_current_source_file', lambda: binding.Validator(registry).validate_files())
    assert len(controls) == 18 and all(item['rejected'] for item in controls)
    result = dict(complete=True, whole_goal_complete=False, controls=controls,
                  registry_sha256=binding.digest(binding.REGISTRY))
    if args.record:
        args.record.write_bytes(gzip.compress((json.dumps(result, indent=2) + '\n').encode(),
                                              mtime=0, compresslevel=3))
    print('In-memory source controls:', len(controls),
          'substitutions rejected; complete source/CTest accepted')


if __name__ == '__main__':
    main()
