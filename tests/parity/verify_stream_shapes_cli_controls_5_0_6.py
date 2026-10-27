#!/usr/bin/env python3
"""Reject altered raw stream output, historical scope and current CTest commands."""

import argparse
import copy
import gzip
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
import stream_shapes_cli_source_binding_5_0_6 as binding


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
            raise AssertionError('Stream source accepted ' + label)

    original = binding.read(ROOT / registry['strict_receipt'])
    changes = [
        ('changed_raw_output', lambda row:
         row['actual'][0].update(error='changed')),
        ('short_raw_output', lambda row: row['actual'].pop()),
        ('hidden_source_difference', lambda row:
         row.update(strict_source_differences=[0])),
        ('missing_boundary_check', lambda row:
         row.update(boundaries_passed=False)),
    ]
    for label, mutate in changes:
        changed = copy.deepcopy(original)
        mutate(changed)
        reject(label, lambda changed=changed: validator.validate_source(changed))
    changed = copy.deepcopy(registry)
    changed['historical_adaptations'] += 1
    reject('historical_disposition_count',
           lambda: binding.Validator(changed).validate_source())

    tests = json.loads(subprocess.check_output(
        ['ctest', '--test-dir', str(args.build_dir), '--show-only=json-v1'],
        text=True))['tests']
    for label in ('foreign_python', 'wrong_binary', 'extra_argument',
                  'missing_source_fixture', 'duplicate_entry'):
        changed = copy.deepcopy(tests)
        chosen = next(row for row in changed if row['name'] == 'stream_shapes_rust_parity')
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
            duplicate['name'] = 'unqualified_stream_comparison'
            changed.append(duplicate)
        reject(label, lambda changed=changed:
               validator.validate_commands(changed, args.build_dir))

    mutations = [
        ('current_comparator_body', lambda row:
         row['support_files'].update({
             'tests/parity/compare_stream_shapes_5_0_6.py': '0' * 64})),
        ('immutable_source_corpus', lambda row:
         row.update(corpus_sha256='0' * 64)),
        ('retained_historical_pair', lambda row:
         row.update(historical_pair_sha256='0' * 64)),
        ('retained_strict_readback', lambda row:
         row.update(strict_receipt_sha256='0' * 64)),
        ('terminal_acceptance_identity', lambda row:
         row.update(terminal_sha256='0' * 64)),
    ]
    for label, mutate in mutations:
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
    print('Stream source controls:', len(controls),
          'substitutions rejected; strict source/CTest accepted')


if __name__ == '__main__':
    main()
