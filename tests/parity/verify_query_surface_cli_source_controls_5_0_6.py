#!/usr/bin/env python3
"""Reject query-surface source, profile, corpus and CTest substitutions."""

import argparse
import copy
import gzip
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from custom_public_cli_source_provenance_5_0_6 import digest
from query_surface_cli_source_provenance_5_0_6 import REGISTRY, Validator, read
from verify_expression_cli_source_controls_5_0_6 import artifact_change, raw_change


def library_test(source, value):
    rows = [json.loads(line) for line in source['steps'][2]['stdout'].split('\n') if line]
    target = next(row for row in rows if row.get('reason') == 'compiler-artifact'
                  and row['target']['name'] == 'teaql_core')
    target['profile']['test'] = value
    source['steps'][2]['stdout'] = ''.join(json.dumps(row) + '\n' for row in rows)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, default=ROOT / 'build_native_current')
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    if args.record is not None and args.record.exists():
        parser.error('Preserve historical evidence')
    validator = Validator(ROOT, read(REGISTRY)).load()
    validator.validate_files()
    validator.validate_source(validator.source)
    validator.validate_ctest(args.build_dir)
    mutations = [
        ('numeric_complete', lambda row: row.update(complete=1)),
        ('foreign_runtime', lambda row: row.update(runtime_commit='0' * 40)),
        ('missing_source_file', lambda row: row['source_files'].pop(next(iter(row['source_files'])))),
        ('source_epoch', lambda row: row['input_hashes_after'].update(invented='corruption')),
        ('missing_archive_member', lambda row: row['archive_files'].pop(next(iter(row['archive_files'])))),
        ('missing_fixture', lambda row: row['generated_fixtures'].pop()),
        ('fixture_hand_edit', lambda row: row['generated_fixtures'][0]['provenance'].update(hand_edited=True)),
        ('manifest', lambda row: row['derived_observer'].update(
            manifest=row['derived_observer']['manifest'] + '\n')),
        ('compiled_lock', lambda row: row['derived_observer'].update(
            compiled_lock=row['derived_observer']['compiled_lock'] + '\n')),
        ('dependency_delta', lambda row: row.update(registry_identity_differences=[['invented']])),
        ('failed_build', lambda row: row['steps'][2].update(exit_code=1)),
        ('unlocked_build', lambda row: row['steps'][2]['command'].remove('--locked')),
        ('debug_assertions', lambda row: artifact_change(row, 'debug_assertions', False)),
        ('numeric_overflow', lambda row: artifact_change(row, 'overflow_checks', 1)),
        ('observer_test_mode', lambda row: artifact_change(row, 'test', True)),
        ('library_test_mode', lambda row: library_test(row, True)),
        ('library_numeric_test_mode', lambda row: library_test(row, 0)),
        ('missing_dataset', lambda row: row['results'].pop()),
        ('missing_input', lambda row: row['results'][0]['inputs'].pop()),
        ('missing_expected', lambda row: row['results'][0]['expected'].pop()),
        ('missing_observed', lambda row: row['results'][0]['observed'].pop()),
        ('wrong_source_binary', lambda row: row['results'][0].update(binary_sha256='0' * 64)),
        ('blank_raw', lambda row: raw_change(row, 'blank')),
        ('missing_raw', lambda row: raw_change(row, 'missing')),
        ('extra_raw', lambda row: raw_change(row, 'extra')),
        ('changed_raw', lambda row: raw_change(row, 'value')),
        ('cleanup', lambda row: row.update(owned_directory_removed=False)),
        ('numeric_cleanup_size', lambda row: row.update(owned_bytes_removed=True)),
    ]
    results = []
    errors = (RuntimeError, ValueError, KeyError, TypeError, IndexError, StopIteration)
    for label, mutate in mutations:
        changed = copy.deepcopy(validator.source)
        mutate(changed)
        try:
            validator.validate_source(changed)
        except errors as error:
            results.append(dict(name=label, rejected=True, reason=str(error)))
        else:
            raise AssertionError('Query-surface source accepted ' + label)
    contract = validator.registry['contracts'][0]
    corpus = read(ROOT / contract['corpus'])
    for label in ('extra_case_field', 'missing_case', 'reordered_cases'):
        changed = copy.deepcopy(corpus)
        if label == 'extra_case_field':
            changed['cases'][0]['unreviewed'] = True
        elif label == 'missing_case':
            changed['cases'].pop()
        else:
            changed['cases'][:2] = reversed(changed['cases'][:2])
        try:
            validator.selected_rows(contract, changed)
        except errors as error:
            results.append(dict(name=label, rejected=True, reason=str(error)))
        else:
            raise AssertionError('Query-surface corpus admitted ' + label)
    tests = json.loads(subprocess.check_output(
        ['ctest', '--test-dir', str(args.build_dir), '--show-only=json-v1'],
        text=True))['tests']
    for label in ('foreign_python', 'wrong_binary', 'extra_argument',
                  'missing_source_prerequisite'):
        changed = copy.deepcopy(tests)
        chosen = 'query_alias_rust_parity'
        test = next(row for row in changed if row['name'] == chosen)
        if label == 'foreign_python':
            test['command'][0] = '/invented/python'
        elif label == 'wrong_binary':
            test['command'][-1] = '/invented'
        elif label == 'extra_argument':
            test['command'].append('--rust=unqualified')
        else:
            test['properties'] = []
        try:
            validator.validate_commands(changed, args.build_dir)
        except errors as error:
            results.append(dict(name=label, rejected=True, reason=str(error)))
        else:
            raise AssertionError('Query-surface CTest accepted ' + label)
    assert len(results) == 35 and all(result['rejected'] for result in results)
    if args.record:
        args.record.write_bytes(gzip.compress((json.dumps(dict(
            complete=True, whole_goal_complete=False, controls=results,
            registry_sha256=digest(REGISTRY)), indent=2) + '\n').encode(), mtime=0))
    print('Query-surface source controls:', len(results),
          'substitutions rejected; complete source/CTest accepted')


if __name__ == '__main__':
    main()
