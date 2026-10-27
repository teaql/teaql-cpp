#!/usr/bin/env python3
"""Reject substituted query-key source, pointer-width and CTest evidence."""

import argparse
import copy
import gzip
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
import query_keys_cli_source_binding_5_0_6 as binding


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
    tests = binding.read_ctest(args.build_dir)
    validator.validate_commands(tests, args.build_dir)
    controls = []

    def reject(label, action):
        try:
            action()
        except (RuntimeError, ValueError, KeyError, TypeError,
                IndexError, AssertionError) as error:
            controls.append(dict(name=label, rejected=True, reason=str(error)))
        else:
            raise AssertionError('Query-key source accepted ' + label)

    corpus_64 = binding.read(ROOT / registry['corpus_64'])
    corpus_32 = binding.read(ROOT / registry['corpus_32'])
    source_mutations = [
        ('changed_64_result', '64', lambda row:
         row['cases'][0]['expected'].update(canonical='changed')),
        ('changed_32_result', '32', lambda row:
         row['cases'][0]['expected'].update(canonical='changed')),
        ('changed_64_input', '64', lambda row:
         row['cases'][0]['input'].update(user='changed')),
        ('short_32_result', '32', lambda row: row['cases'].pop()),
        ('wrong_32_observer', '32', lambda row:
         row.update(observer_sha256='0' * 64)),
    ]
    for label, width, change in source_mutations:
        altered = copy.deepcopy(corpus_64 if width == '64' else corpus_32)
        change(altered)
        reject(label, lambda altered=altered, width=width:
               validator.validate_source(override_64=altered if width == '64' else None,
                                         override_32=altered if width == '32' else None))

    for label in ('foreign_python', 'wrong_binary', 'wrong_pointer_width',
                  'missing_fixture', 'duplicate_entry'):
        altered = copy.deepcopy(tests)
        chosen = next(row for row in altered if row['name'] == registry['test'])
        if label == 'foreign_python':
            chosen['command'][0] = '/invented/python'
        elif label == 'wrong_binary':
            chosen['command'][3] = '/invented/native'
        elif label == 'wrong_pointer_width':
            chosen['command'][-1] = '--pointer-bytes=4'
        elif label == 'missing_fixture':
            chosen['properties'] = []
        else:
            duplicate = copy.deepcopy(chosen)
            duplicate['name'] = 'unqualified_query_key_comparison'
            altered.append(duplicate)
        reject(label, lambda altered=altered:
               validator.validate_commands(altered, args.build_dir))

    for label, change in [
        ('current_comparator', lambda row: row['support_files'].update({
            'tests/parity/compare_query_keys.py': '0' * 64})),
        ('current_64_corpus', lambda row: row.update(corpus_64_sha256='0' * 64)),
        ('current_32_corpus', lambda row: row.update(corpus_32_sha256='0' * 64)),
        ('width_prepared_source', lambda row: row.update(source_32_sha256='0' * 64)),
        ('retained_mips_receipt', lambda row: row.update(mips_receipt_sha256='0' * 64)),
    ]:
        altered = copy.deepcopy(registry)
        change(altered)
        reject(label, lambda altered=altered:
               binding.Validator(altered).validate_files())

    assert len(controls) == 15 and all(item['rejected'] for item in controls)
    if args.record:
        args.record.write_bytes(gzip.compress(
            (json.dumps(dict(complete=True, whole_goal_complete=False,
                             controls=controls,
                             registry_sha256=binding.digest(binding.REGISTRY)),
                        indent=2) + '\n').encode(), mtime=0, compresslevel=3))
    print('Query-key source controls:', len(controls), 'substitutions rejected')


if __name__ == '__main__':
    main()
