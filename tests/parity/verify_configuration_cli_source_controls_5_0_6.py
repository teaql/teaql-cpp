#!/usr/bin/env python3
"""Reject configuration source, dependency, helper, matrix and CTest substitutions."""

import argparse
import copy
import gzip
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from configuration_cli_source_provenance_5_0_6 import REGISTRY, Validator, read
from compare_values import same
from custom_public_cli_source_provenance_5_0_6 import digest


def artifact_change(source, field, value, library=False):
    observer = source['observers'][0]
    step = source['steps'][observer['build_step']]
    rows = [json.loads(line) for line in step['stdout'].split('\n') if line]
    artifact = (next(v for v in rows if v.get('reason') == 'compiler-artifact'
                     and 'scalar_candidate_fixture_core' == v['target']['name']) if library
                else next(iter(observer['compiler_artifacts'].values())))
    identity, name = artifact['package_id'], artifact['target']['name']
    if field in artifact['profile']:
        artifact['profile'][field] = value
    else:
        artifact[field] = value
    for index, row in enumerate(rows):
        if (row.get('reason') == 'compiler-artifact' and row['package_id'] == identity
                and row['target']['name'] == name):
            rows[index] = copy.deepcopy(artifact)
    step['stdout'] = ''.join(json.dumps(row) + '\n' for row in rows)


def raw_change(source, mode):
    result = source['results'][0]
    step = source['steps'][result['step']]
    rows = [json.loads(line) for line in step['stdout'].split('\n') if line]
    if mode == 'missing':
        rows.pop()
    elif mode == 'extra':
        rows.append(rows[-1])
    elif mode == 'blank':
        step['stdout'] = '\n' + step['stdout']
        return
    elif mode == 'type':
        rows[0]['rows'][0]['id'] = float(rows[0]['rows'][0]['id'])
    else:
        rows[0]['filter'] = 'corruption'
    step['stdout'] = ''.join(json.dumps(row) + '\n' for row in rows)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--contracts', type=Path, default=REGISTRY)
    parser.add_argument('--build-dir', type=Path, default=ROOT / 'build_native_current')
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    if args.record is not None and args.record.exists():
        parser.error('Preserve historical evidence')
    validator = Validator(ROOT, read(args.contracts)).load()
    validator.validate_files()
    validator.validate_source(validator.source)
    validator.validate_ctest(args.build_dir)
    controls = [
        ('numeric_complete', lambda d: d.update(complete=1)),
        ('incomplete', lambda d: d.update(complete=False)),
        ('numeric_goal_scope', lambda d: d.update(whole_goal_complete=0)),
        ('foreign_baseline', lambda d: d.update(runtime_commit='0' * 40)),
        ('foreign_generator', lambda d: d.update(generator_commit='0' * 40)),
        ('missing_copied_input', lambda d: d['source_files'].pop(next(iter(d['source_files'])))),
        ('input_epoch', lambda d: d['input_hashes_after'].update(invented='0' * 64)),
        ('erased_epoch', lambda d: d.update(input_hashes_before={}, input_hashes_after={})),
        ('archive_hash', lambda d: d.update(archive_sha256='0' * 64)),
        ('missing_archive_file', lambda d: d['archive_files'].pop(next(iter(d['archive_files'])))),
        ('missing_generated_model', lambda d: d['generated_fixtures'].pop()),
        ('generated_hand_edit', lambda d: d['generated_fixtures'][0]['provenance'].update(hand_edited=True)),
        ('missing_project', lambda d: d['projects'].pop()),
        ('wrong_model_variant', lambda d: d['projects'][0].update(case='integer')),
        ('missing_observer', lambda d: d['observers'].pop()),
        ('wrong_observer', lambda d: d['observers'][0].update(name='invented')),
        ('seed_lock', lambda d: d.update(seed_lock_sha256='0' * 64)),
        ('manifest', lambda d: d['observers'][0].update(manifest=d['observers'][0]['manifest'] + '\n')),
        ('compiled_lock', lambda d: d['observers'][0].update(compiled_lock=d['observers'][0]['compiled_lock'] + '\n')),
        ('metadata_graph', lambda d: d['observers'][0].update(metadata=d['observers'][0]['metadata'] + ' ')),
        ('dependency_differences', lambda d: d['observers'][0].update(registry_identity_differences=[['invented']])),
        ('missing_call_helper', lambda d: d['observers'][0]['helpers'].pop('base/candidate_calls.rs')),
        ('changed_call_helper', lambda d: d['observers'][0]['helpers'].update({'base/candidate_calls.rs': 'corruption'})),
        ('changed_driver', lambda d: d['observers'][0]['driver_copies'].update({'base/main.rs': '0' * 64})),
        ('numeric_resolution_step', lambda d: d['observers'][0].update(resolve_step=False)),
        ('wrong_build_step', lambda d: d['observers'][0].update(build_step=3)),
        ('environment', lambda d: d['cargo_environment'].update(CARGO_INCREMENTAL='1')),
        ('ambient_override', lambda d: d.update(ambient_rust_overrides=['RUSTFLAGS'])),
        ('failed_build', lambda d: d['steps'][2].update(exit_code=1)),
        ('numeric_success', lambda d: d['steps'][0].update(exit_code=False)),
        ('unlocked_build', lambda d: d['steps'][2]['command'].remove('--locked')),
        ('wrong_metadata_command', lambda d: d['steps'][1]['command'].append('invented')),
        ('missing_binary_artifact', lambda d: d['observers'][0]['compiler_artifacts'].clear()),
        ('debug_assertions', lambda d: artifact_change(d, 'debug_assertions', False)),
        ('overflow_type', lambda d: artifact_change(d, 'overflow_checks', 1)),
        ('test_profile', lambda d: artifact_change(d, 'test', True)),
        ('release_profile', lambda d: artifact_change(d, 'opt_level', '3')),
        ('observer_package', lambda d: artifact_change(d, 'package_id', 'invented')),
        ('domain_debug_assertions', lambda d: artifact_change(d, 'debug_assertions', False, True)),
        ('domain_overflow_checks', lambda d: artifact_change(d, 'overflow_checks', False, True)),
        ('domain_test_profile', lambda d: artifact_change(d, 'test', True, True)),
        ('domain_numeric_test_profile', lambda d: artifact_change(d, 'test', 0, True)),
        ('missing_dataset', lambda d: d['results'].pop()),
        ('wrong_dataset', lambda d: d['results'][0].update(corpus='invented.json.gz')),
        ('wrong_project_binding', lambda d: d['results'][0].update(project_name='invented')),
        ('missing_input', lambda d: d['results'][0]['inputs'].pop()),
        ('missing_expected', lambda d: d['results'][0]['expected'].pop()),
        ('missing_observed', lambda d: d['results'][0]['observed'].pop()),
        ('foreign_observer', lambda d: d['results'][0].update(binary_sha256='0' * 64)),
        ('numeric_execution_step', lambda d: d['results'][0].update(step=True)),
        ('missing_raw_output', lambda d: raw_change(d, 'missing')),
        ('extra_raw_output', lambda d: raw_change(d, 'extra')),
        ('blank_raw_record', lambda d: raw_change(d, 'blank')),
        ('wrong_raw_value', lambda d: raw_change(d, 'value')),
        ('wrong_raw_number_type', lambda d: raw_change(d, 'type')),
        ('cleanup', lambda d: d.update(owned_directory_removed=False)),
        ('numeric_cleanup_size', lambda d: d.update(owned_bytes_removed=True)),
    ]
    results = []
    errors = (RuntimeError, ValueError, KeyError, TypeError, StopIteration, IndexError)
    for name, mutate in controls:
        altered = copy.deepcopy(validator.source)
        mutate(altered)
        assert not same(altered, validator.source), 'Control must mutate source: ' + name
        try:
            validator.validate_source(altered)
        except errors as error:
            results.append(dict(name=name, rejected=True, reason=str(error)))
        else:
            raise AssertionError('Source validator accepted ' + name)
    registry = json.loads(subprocess.check_output(
        ['ctest', '--test-dir', str(args.build_dir), '--show-only=json-v1'], text=True))
    first_name = validator.registry['contracts'][0]['test']
    command_controls = (
        ('missing_ctest', lambda d, t: d['tests'].remove(t)),
        ('wrong_case', lambda d, t: t['command'].__setitem__(t['command'].index('--case') + 1, 'integer')),
        ('wrong_binary', lambda d, t: t['command'].__setitem__(t['command'].index('--cpp') + 1, '/invented')),
        ('foreign_python', lambda d, t: t['command'].__setitem__(0, '/invented/python')),
        ('extra_argument', lambda d, t: t['command'].append('--rust=unqualified')),
        ('missing_source_prerequisite', lambda d, t: t.update(properties=[])),
    )
    for name, mutate in command_controls:
        altered = copy.deepcopy(registry)
        test = next(t for t in altered['tests'] if t['name'] == first_name)
        mutate(altered, test)
        try:
            validator.validate_commands(altered, args.build_dir)
        except errors as error:
            results.append(dict(name=name, rejected=True, reason=str(error)))
        else:
            raise AssertionError('Source validator accepted ' + name)
    if args.record is not None:
        args.record.write_bytes(gzip.compress((json.dumps(dict(
            complete=True, whole_goal_complete=False, controls=results,
            registry_sha256=digest(args.contracts)), indent=2) + '\n').encode(), mtime=0))
    print('Configuration source controls:', len(results), 'rejected substitutions; positive source and CTest accepted')


if __name__ == '__main__':
    main()
