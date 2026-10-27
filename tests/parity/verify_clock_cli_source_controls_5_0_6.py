#!/usr/bin/env python3
"""Reject substituted clock source traces, partitions, modes and CTest bindings."""

import argparse
import copy
import gzip
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from clock_cli_source_provenance_5_0_6 import REGISTRY, Validator, read
from compare_values import same
from custom_public_cli_source_provenance_5_0_6 import digest


def artifact_change(source, field, value, library=False):
    rows = [json.loads(line) for line in source['steps'][2]['stdout'].split('\n') if line]
    artifact = (next(r for r in rows if r.get('reason') == 'compiler-artifact'
                     and r['target']['name'] == 'teaql_core') if library
                else source['compiler_artifacts']['checker_clock_composition'])
    identity, name = artifact['package_id'], artifact['target']['name']
    if field in artifact['profile']:
        artifact['profile'][field] = value
    else:
        artifact[field] = value
    for i, row in enumerate(rows):
        if (row.get('reason') == 'compiler-artifact' and row['package_id'] == identity
                and row['target']['name'] == name):
            rows[i] = copy.deepcopy(artifact)
    source['steps'][2]['stdout'] = ''.join(json.dumps(row) + '\n' for row in rows)


def raw_change(source, mode):
    step = source['steps'][source['results'][0]['runs'][0]['step']]
    rows = [json.loads(line) for line in step['stdout'].split('\n') if line]
    if mode == 'blank':
        step['stdout'] = '\n' + step['stdout']
        return
    if mode == 'missing':
        rows.pop()
    elif mode == 'extra':
        rows.append(rows[-1])
    else:
        rows[0]['wall_millis'] = float(rows[0]['wall_millis'])
    step['stdout'] = ''.join(json.dumps(row) + '\n' for row in rows)


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
    controls = [
        ('numeric_complete', lambda d: d.update(complete=1)),
        ('numeric_goal_scope', lambda d: d.update(whole_goal_complete=0)),
        ('missing_copied_input', lambda d: d['source_files'].pop(next(iter(d['source_files'])))),
        ('changed_source_epoch', lambda d: d['source_input_hashes_after'].update(invented='corruption')),
        ('foreign_reference', lambda d: d['references_after'].update({'teaql-rs': '0' * 40})),
        ('archive_member', lambda d: d['archive_files'].pop(next(iter(d['archive_files'])))),
        ('missing_fixture', lambda d: d['generated_fixtures'].pop()),
        ('fixture_hand_edit', lambda d: d['generated_fixtures'][0]['provenance'].update(hand_edited=True)),
        ('dataset_declaration', lambda d: d['dataset_contracts'].pop()),
        ('numeric_core_limit', lambda d: d.update(core_dump_limit=[False, False])),
        ('clock_binary', lambda d: d.update(clock_binary_sha256='0' * 64)),
        ('failed_clock_compile', lambda d: d['steps'][3].update(exit_code=1)),
        ('foreign_clock_compiler', lambda d: d['steps'][3]['command'].__setitem__(0, '/invented')),
        ('manifest', lambda d: d['derived_observer'].update(manifest=d['derived_observer']['manifest'] + '\n')),
        ('compiled_lock', lambda d: d['derived_observer'].update(compiled_lock=d['derived_observer']['compiled_lock'] + '\n')),
        ('dependency_delta', lambda d: d.update(registry_identity_differences=[['invented']])),
        ('missing_artifact', lambda d: d['compiler_artifacts'].pop('checker_clock_composition')),
        ('debug_assertions', lambda d: artifact_change(d, 'debug_assertions', False)),
        ('numeric_overflow', lambda d: artifact_change(d, 'overflow_checks', 1)),
        ('observer_test_profile', lambda d: artifact_change(d, 'test', True)),
        ('runtime_test_profile', lambda d: artifact_change(d, 'test', True, True)),
        ('runtime_numeric_test_profile', lambda d: artifact_change(d, 'test', 0, True)),
        ('runtime_release_profile', lambda d: artifact_change(d, 'opt_level', '3', True)),
        ('wrong_observer_package', lambda d: artifact_change(d, 'package_id', 'invented')),
        ('missing_dataset', lambda d: d['results'].pop()),
        ('missing_input', lambda d: d['results'][0]['inputs'].pop()),
        ('missing_expected', lambda d: d['results'][0]['expected'].pop()),
        ('missing_observed', lambda d: d['results'][0]['observed'].pop()),
        ('missing_partition', lambda d: d['results'][0]['runs'].pop()),
        ('duplicate_partition', lambda d: d['results'][0]['runs'].__setitem__(1, copy.deepcopy(d['results'][0]['runs'][0]))),
        ('wrong_wall', lambda d: d['results'][0]['runs'][0].update(wall_millis=0)),
        ('wrong_partition_indices', lambda d: d['results'][0]['runs'][0]['indices'].reverse()),
        ('numeric_partition_step', lambda d: d['results'][0]['runs'][0].update(step=True)),
        ('foreign_clock_preload', lambda d: d['steps'][4]['clock_environment'].update(LD_PRELOAD='/invented.so')),
        ('foreign_process_wall', lambda d: d['steps'][4]['clock_environment'].update(TEAQL_TEST_SAMPLE_WALL_MILLIS='0')),
        ('blank_raw_record', lambda d: raw_change(d, 'blank')),
        ('missing_raw_record', lambda d: raw_change(d, 'missing')),
        ('extra_raw_record', lambda d: raw_change(d, 'extra')),
        ('numeric_raw_wall', lambda d: raw_change(d, 'type')),
        ('missing_boundary', lambda d: d['boundary']['cases'].pop()),
        ('boundary_input', lambda d: d['boundary']['cases'][0]['input'].update(mode='negative_value')),
        ('boundary_classification', lambda d: d['boundary']['cases'][0].update(observed={'outcome': 'value'})),
        ('boundary_missing_failure', lambda d: d['steps'][d['boundary']['cases'][0]['step']].update(exit_code=0)),
        ('boundary_numeric_exit', lambda d: d['steps'][d['boundary']['cases'][2]['step']].update(exit_code=False)),
        ('boundary_wrong_mode', lambda d: d['steps'][d['boundary']['cases'][0]['step']]['command'].__setitem__(1, 'negative_value')),
        ('boundary_wrong_clock', lambda d: d['steps'][d['boundary']['cases'][0]['step']]['clock_environment'].update(TEAQL_TEST_SAMPLE_WALL_MILLIS='1')),
        ('boundary_missing_failure_text', lambda d: d['steps'][d['boundary']['cases'][0]['step']].update(stderr='invented')),
        ('cleanup', lambda d: d.update(owned_directory_removed=False)),
        ('numeric_cleanup_size', lambda d: d.update(owned_bytes_removed=True)),
    ]
    results = []
    errors = (RuntimeError, ValueError, KeyError, TypeError, IndexError, StopIteration)
    for name, mutate in controls:
        changed = copy.deepcopy(validator.source)
        mutate(changed)
        assert not same(changed, validator.source), name
        try:
            validator.validate_source(changed)
        except errors as error:
            results.append(dict(name=name, rejected=True, reason=str(error)))
        else:
            raise AssertionError('Clock source verifier accepted ' + name)
    tests = json.loads(subprocess.check_output(['ctest', '--test-dir', str(args.build_dir),
                                               '--show-only=json-v1'], text=True))['tests']
    for name, mutate in (
        ('wrong_native_clock', lambda t: t['command'].__setitem__(-1, '/invented.so')),
        ('extra_original_argument', lambda t: t['command'].append('--rust=unqualified')),
        ('source_fixture_removed', lambda t: t.update(properties=[])),
        ('foreign_python', lambda t: t['command'].__setitem__(0, '/invented/python')),
    ):
        changed = copy.deepcopy(tests)
        test = next(t for t in changed if t['name'] == 'checker_clock_composition_rust_parity')
        mutate(test)
        try:
            validator.validate_commands(changed, args.build_dir)
        except errors as error:
            results.append(dict(name=name, rejected=True, reason=str(error)))
        else:
            raise AssertionError('Clock CTest verifier accepted ' + name)
    if args.record is not None:
        args.record.write_bytes(gzip.compress((json.dumps(dict(
            complete=True, whole_goal_complete=False, controls=results,
            registry_sha256=digest(REGISTRY)), indent=2) + '\n').encode(), mtime=0))
    print('Clock source controls:', len(results), 'rejected substitutions; valid source and CTest accepted')


if __name__ == '__main__':
    main()
