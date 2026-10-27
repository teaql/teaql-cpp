#!/usr/bin/env python3
"""Reject state/entity source, typed-row and current CTest substitutions."""

import argparse
import copy
import gzip
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
import state_source_cli_binding_5_0_6 as binding


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, default=ROOT / 'build_native_current')
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    if args.record is not None and args.record.exists():
        parser.error('Preserve historical controls')
    registry = binding.read(binding.REGISTRY)
    validator = binding.Validator(registry)
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
            raise AssertionError('State source accepted ' + label)

    originals = {item['name']: binding.read(ROOT / item['source_receipt']['path'])
                 for item in registry['contracts']}
    mutations = [
        ('runtime_numeric_complete', 'runtime_state_bridge',
         lambda row: row.update(complete=1)),
        ('runtime_missing_original', 'runtime_state_bridge',
         lambda row: row['steps'].pop(0)),
        ('runtime_original_failure', 'runtime_state_bridge',
         lambda row: row['steps'][0].update(exit_code=1)),
        ('runtime_short_raw', 'runtime_state_bridge',
         lambda row: row['steps'][0].update(stdout=row['steps'][0]['stdout'].split('\n', 1)[1])),
        ('generated_missing_original', 'generated_runtime_state_bridge',
         lambda row: row.update(actual_original_replayed=False)),
        ('generated_changed_raw', 'generated_runtime_state_bridge',
         lambda row: row['steps'][0].update(stdout='{}\n')),
        ('bulk_wrong_corpus', 'generated_load_state_bulk',
         lambda row: row.update(corpus_sha256='0' * 64)),
        ('bulk_short_raw', 'generated_load_state_bulk',
         lambda row: row.update(stdout=row['stdout'].split('\n', 1)[1])),
        ('entity_missing_raw', 'entity_source',
         lambda row: row['steps'][0].update(stdout='')),
        ('entity_wrong_historical_corpus', 'entity_source',
         lambda row: row.update(corpus_sha256='0' * 64)),
    ]
    for label, name, mutate in mutations:
        changed = copy.deepcopy(originals[name])
        mutate(changed)
        reject(label, lambda changed=changed, name=name:
               validator.validate_source({name: changed}))

    tests = json.loads(subprocess.check_output(
        ['ctest', '--test-dir', str(args.build_dir), '--show-only=json-v1'],
        text=True))['tests']
    for label in ('foreign_python', 'wrong_binary', 'extra_argument',
                  'missing_source_fixture', 'duplicate_entry'):
        changed = copy.deepcopy(tests)
        chosen = next(row for row in changed if row['name'] == 'runtime_state_bridge_rust_parity')
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
            duplicate['name'] = 'unqualified_state_comparison'
            changed.append(duplicate)
        reject(label, lambda changed=changed:
               validator.validate_commands(changed, args.build_dir))

    changed = copy.deepcopy(registry)
    changed['contracts'][0]['source_hashes'][next(iter(changed['contracts'][0]['source_hashes']))] = '0' * 64
    reject('current_source_body', lambda: binding.Validator(changed).validate_files())
    changed = copy.deepcopy(registry)
    changed['contracts'][0]['corpus_sha256'] = '0' * 64
    reject('retained_corpus_identity', lambda: binding.Validator(changed).validate_files())
    changed = copy.deepcopy(registry)
    changed['contracts'][-1]['historical_corpus']['previous_source_sha256'] = '0' * 64
    reject('historical_metadata_transition',
           lambda: binding.Validator(changed).validate_source())
    assert len(controls) == 18 and all(item['rejected'] for item in controls)
    result = dict(complete=True, whole_goal_complete=False, controls=controls,
                  registry_sha256=binding.digest(binding.REGISTRY))
    if args.record:
        args.record.write_bytes(gzip.compress((json.dumps(result, indent=2) + '\n').encode(),
                                              mtime=0, compresslevel=3))
    print('State/entity source controls:', len(controls),
          'substitutions rejected; complete source/CTest accepted')


if __name__ == '__main__':
    main()
