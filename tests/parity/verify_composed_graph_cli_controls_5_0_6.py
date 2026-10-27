#!/usr/bin/env python3
"""Reject composed-query and graph-save source or CTest substitutions."""

import argparse
import copy
import gzip
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
import composed_graph_cli_source_binding_5_0_6 as binding


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path,
                        default=ROOT / 'build_native_current')
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
        except (RuntimeError, ValueError, KeyError, TypeError,
                IndexError, AssertionError) as error:
            controls.append(dict(name=label, rejected=True, reason=str(error)))
        else:
            raise AssertionError('Composed/graph source accepted ' + label)

    originals = {item['name']: binding.read(ROOT / item['paired'])
                 for item in registry['contracts']}
    mutations = [
        ('composed_changed_original', 'composed_general',
         lambda row: row['original'][0].update(error='changed')),
        ('composed_short_original', 'composed_general',
         lambda row: row['original'].pop()),
        ('composed_changed_cpp', 'composed_general',
         lambda row: row['cpp'][0].update(error='changed')),
        ('graph_changed_input', 'relation_graph_save',
         lambda row: row['inputs'][0].update(target='invented')),
        ('graph_changed_original', 'relation_graph_save',
         lambda row: row['original'][0].update(error='changed')),
        ('graph_false_success', 'relation_graph_save',
         lambda row: row.update(failures=[0])),
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
        chosen = next(row for row in changed
                      if row['name'] == 'relation_graph_save_rust_parity')
        if label == 'foreign_python':
            chosen['command'][0] = '/invented/python'
        elif label == 'wrong_binary':
            chosen['command'][-1] = '/invented/native'
        elif label == 'extra_argument':
            chosen['command'].append('--rust=/unqualified')
        elif label == 'missing_source_fixture':
            chosen['properties'] = []
        else:
            duplicate = copy.deepcopy(chosen)
            duplicate['name'] = 'unqualified_graph_comparison'
            changed.append(duplicate)
        reject(label, lambda changed=changed:
               validator.validate_commands(changed, args.build_dir))

    changes = [
        ('current_comparator_body', lambda row:
         row['support_files'].update({
             'tests/parity/compare_relation_graph_save_5_0_6.py': '0' * 64})),
        ('retained_corpus_identity', lambda row:
         row['contracts'][0].update(corpus_sha256='0' * 64)),
        ('retained_paired_identity', lambda row:
         row['contracts'][1].update(paired_sha256='0' * 64)),
        ('terminal_receipt_identity', lambda row:
         row['contracts'][1].update(terminal_sha256='0' * 64)),
    ]
    for label, mutate in changes:
        changed = copy.deepcopy(registry)
        mutate(changed)
        reject(label, lambda changed=changed:
               binding.Validator(changed).validate_files())

    assert len(controls) == 15 and all(item['rejected'] for item in controls)
    result = dict(complete=True, whole_goal_complete=False, controls=controls,
                  registry_sha256=binding.digest(binding.REGISTRY))
    if args.record:
        args.record.write_bytes(gzip.compress(
            (json.dumps(result, indent=2) + '\n').encode(),
            mtime=0, compresslevel=3))
    print('Composed/graph source controls:', len(controls),
          'substitutions rejected; complete source/CTest accepted')


if __name__ == '__main__':
    main()
