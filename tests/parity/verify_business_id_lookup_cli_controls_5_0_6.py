#!/usr/bin/env python3
"""Reject substituted typed Business-ID Q evidence and CTest commands."""

import argparse
import copy
import gzip
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
import business_id_lookup_cli_source_binding_5_0_6 as binding


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
            raise AssertionError('Q source accepted ' + label)

    source = binding.read(ROOT / registry['generated_pair'])
    for label, change in [
        ('changed_original', lambda row: row['original'][0].update(error='changed')),
        ('changed_native', lambda row: row['cpp'][0].update(error='changed')),
        ('short_native', lambda row: row['cpp'].pop()),
        ('hidden_failure', lambda row: row.update(failures=[0])),
        ('runtime_mode_substitution', lambda row: row.update(runtime_composition=True)),
    ]:
        altered = copy.deepcopy(source)
        change(altered)
        reject(label, lambda altered=altered: validator.validate_source(altered))

    for label in ('foreign_python', 'wrong_binary', 'extra_argument',
                  'missing_fixture', 'duplicate_entry'):
        altered = copy.deepcopy(tests)
        chosen = next(row for row in altered if row['name'] == registry['test'])
        if label == 'foreign_python':
            chosen['command'][0] = '/invented/python'
        elif label == 'wrong_binary':
            chosen['command'][-1] = '/invented/native'
        elif label == 'extra_argument':
            chosen['command'].append('--rust=/unqualified')
        elif label == 'missing_fixture':
            chosen['properties'] = []
        else:
            duplicate = copy.deepcopy(chosen)
            duplicate['name'] = 'unqualified_business_id_lookup'
            altered.append(duplicate)
        reject(label, lambda altered=altered:
               validator.validate_commands(altered, args.build_dir))

    for label, change in [
        ('current_comparator', lambda row: row['support_files'].update({
            'tests/parity/compare_business_id_lookups_5_0_6.py': '0' * 64})),
        ('immutable_corpus', lambda row: row.update(corpus_sha256='0' * 64)),
        ('generated_pair', lambda row: row.update(generated_pair_sha256='0' * 64)),
        ('runtime_pair', lambda row: row.update(runtime_pair_sha256='0' * 64)),
        ('terminal', lambda row: row.update(terminal_sha256='0' * 64)),
    ]:
        altered = copy.deepcopy(registry)
        change(altered)
        reject(label, lambda altered=altered: binding.Validator(altered).validate_files())

    assert len(controls) == 15 and all(item['rejected'] for item in controls)
    if args.record:
        args.record.write_bytes(gzip.compress(
            (json.dumps(dict(complete=True, whole_goal_complete=False,
                             controls=controls,
                             registry_sha256=binding.digest(binding.REGISTRY)),
                        indent=2) + '\n').encode(), mtime=0, compresslevel=3))
    print('Typed Business-ID Q controls:', len(controls), 'substitutions rejected')


if __name__ == '__main__':
    main()
