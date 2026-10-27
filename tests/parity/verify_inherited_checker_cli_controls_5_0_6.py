#!/usr/bin/env python3
"""Reject altered inherited Checker source, clock and installed CTest commands."""

import argparse
import copy
import gzip
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
import inherited_checker_cli_source_binding_5_0_6 as binding


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
            raise AssertionError('Inherited Checker source accepted ' + label)

    corpus = binding.read(ROOT / registry['corpus'])
    for label, change in [
        ('changed_result', lambda row: row['cases'][0]['expected'].update(error='changed')),
        ('changed_input', lambda row: row['cases'][0]['input'].update(root=99)),
        ('short_corpus', lambda row: row['cases'].pop()),
        ('wrong_wall', lambda row:
         row['cases'][0]['expected'].update(wall_millis=99)),
        ('wrong_model', lambda row:
         row['cases'][0]['input'].update(model='invented')),
    ]:
        altered = copy.deepcopy(corpus)
        change(altered)
        reject(label, lambda altered=altered: validator.validate_source(altered))

    for label in ('foreign_python', 'wrong_model_argument', 'wrong_clock',
                  'missing_fixture', 'duplicate_entry'):
        altered = copy.deepcopy(tests)
        chosen = next(row for row in altered
                      if row['name'] == 'inherited_checker_helpers_document_rust_parity')
        if label == 'foreign_python':
            chosen['command'][0] = '/invented/python'
        elif label == 'wrong_model_argument':
            chosen['command'][5] = 'invoice'
        elif label == 'wrong_clock':
            chosen['command'][-1] = '/invented/clock'
        elif label == 'missing_fixture':
            chosen['properties'] = []
        else:
            duplicate = copy.deepcopy(chosen)
            duplicate['name'] = 'unqualified_inherited_checker'
            altered.append(duplicate)
        reject(label, lambda altered=altered:
               validator.validate_commands(altered, args.build_dir))

    for label, change in [
        ('current_comparator', lambda row: row['support_files'].update({
            'tests/parity/compare_inherited_checker_helpers_5_0_6.py': '0' * 64})),
        ('original_rust_driver', lambda row: row['support_files'].update({
            'tests/parity/inherited_checker_helpers_5_0_6.rs': '0' * 64})),
        ('fixed_clock_driver', lambda row: row['support_files'].update({
            'tests/parity/sample_clock_driver.py': '0' * 64})),
        ('retained_corpus', lambda row: row.update(corpus_sha256='0' * 64)),
        ('terminal', lambda row: row.update(terminal_sha256='0' * 64)),
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
    print('Inherited Checker controls:', len(controls), 'substitutions rejected')


if __name__ == '__main__':
    main()
