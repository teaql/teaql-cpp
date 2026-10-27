#!/usr/bin/env python3
"""Reject quoted-FK scope, frozen-source and actual CTest substitutions."""

import argparse
import copy
import gzip
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
import relation_analysis_cli_source_binding_5_0_6 as binding
from quoted_fk_debug_adapter_5_0_6 import RUST_COLUMN, RUST_DESCRIPTOR, normalize_quoted_fk_descriptor


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
            raise AssertionError('Relationship source accepted ' + label)

    cases = binding.read(ROOT / registry['corpus'])['cases']
    exception_index = next(index for index, case in enumerate(cases)
                           if case['qualification'] == 'pending_source_failure_decision')
    changes = [
        ('missing_source_case', lambda rows: rows.pop()),
        ('changed_source_result', lambda rows:
         rows[0]['expected'].update(error='changed')),
        ('unapproved_exception_label', lambda rows:
         rows[exception_index].update(qualification='matched')),
        ('changed_approved_cpp_result', lambda rows:
         rows[exception_index]['cpp_observation_at_review'].update(error='changed')),
    ]
    for label, mutate in changes:
        changed = copy.deepcopy(cases)
        mutate(changed)
        reject(label, lambda changed=changed: validator.validate_source(changed))

    tests = json.loads(subprocess.check_output(
        ['ctest', '--test-dir', str(args.build_dir), '--show-only=json-v1'],
        text=True))['tests']
    for label in ('foreign_python', 'wrong_binary', 'extra_argument',
                  'missing_source_fixture', 'duplicate_entry'):
        changed = copy.deepcopy(tests)
        chosen = next(row for row in changed
                      if row['name'] == 'generated_relation_analysis_rust_parity')
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
            duplicate['name'] = 'unqualified_relation_comparison'
            changed.append(duplicate)
        reject(label, lambda changed=changed:
               validator.validate_commands(changed, args.build_dir))

    mutations = [
        ('current_adapter_body', lambda row:
         row['support_files'].update({
             'tests/parity/compare_generated_relation_analysis.py': '0' * 64})),
        ('immutable_source_corpus', lambda row:
         row.update(corpus_sha256='0' * 64)),
        ('approval_manifest_identity', lambda row:
         row.update(exception_manifest_sha256='0' * 64)),
        ('failure_witness_identity', lambda row:
         row.update(source_failure_sha256='0' * 64)),
    ]
    for label, mutate in mutations:
        changed = copy.deepcopy(registry)
        mutate(changed)
        reject(label, lambda changed=changed:
               binding.Validator(changed).validate_files())
    for label, key in [('changed_exception_count', 'exception_count'),
                       ('changed_exact_count', 'exact_count')]:
        changed = copy.deepcopy(registry)
        changed[key] += 1
        reject(label, lambda changed=changed:
               binding.Validator(changed).validate_source())

    assert len(controls) == 15 and all(item['rejected'] for item in controls)
    descriptor = {'query': RUST_DESCRIPTOR}
    adapted_descriptor = binding.cpp_observation(descriptor)
    assert adapted_descriptor['query'] == normalize_quoted_fk_descriptor(RUST_DESCRIPTOR)
    assert adapted_descriptor != descriptor
    ordinary = {'query': 'ordinary filter text: ' + RUST_COLUMN}
    assert binding.cpp_observation(ordinary) == ordinary
    result = dict(complete=True, whole_goal_complete=False, controls=controls,
                  registry_sha256=binding.digest(binding.REGISTRY),
                  adapter_scope=dict(descriptor_corrected=True, ordinary_text_preserved=True))
    if args.record:
        args.record.write_bytes(gzip.compress(
            (json.dumps(result, indent=2) + '\n').encode(),
            mtime=0, compresslevel=3))
    print('Relation-analysis source controls:', len(controls),
          'substitutions rejected; approved descriptor only and CTest accepted')


if __name__ == '__main__':
    main()
