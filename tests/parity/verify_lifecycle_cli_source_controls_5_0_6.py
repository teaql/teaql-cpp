#!/usr/bin/env python3
"""Reject lifecycle source substitutions and undeclared ownership/log projections."""

import argparse
import copy
import gzip
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from lifecycle_cli_source_provenance_5_0_6 import REGISTRY, Validator, read
import lifecycle_cli_protocol_5_0_6 as protocol
from compare_values import same
from custom_public_cli_source_provenance_5_0_6 import digest
from verify_expression_cli_source_controls_5_0_6 import artifact_change, raw_change


def library_test(source, value):
    rows = [json.loads(line) for line in source['steps'][2]['stdout'].split('\n') if line]
    target = next(v for v in rows if v.get('reason') == 'compiler-artifact'
                  and v['target']['name'] == 'teaql_core')
    target['profile']['test'] = value
    source['steps'][2]['stdout'] = ''.join(json.dumps(v) + '\n' for v in rows)


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
        ('foreign_runtime', lambda d: d.update(runtime_commit='0' * 40)),
        ('missing_source_file', lambda d: d['source_files'].pop(next(iter(d['source_files'])))),
        ('source_epoch', lambda d: d['input_hashes_after'].update(invented='corruption')),
        ('missing_archive_member', lambda d: d['archive_files'].pop(next(iter(d['archive_files'])))),
        ('missing_fixture', lambda d: d['generated_fixtures'].pop()),
        ('fixture_hand_edit', lambda d: d['generated_fixtures'][0]['provenance'].update(hand_edited=True)),
        ('manifest', lambda d: d['derived_observer'].update(manifest=d['derived_observer']['manifest'] + '\n')),
        ('compiled_lock', lambda d: d['derived_observer'].update(compiled_lock=d['derived_observer']['compiled_lock'] + '\n')),
        ('dependency_delta', lambda d: d.update(registry_identity_differences=[['invented']])),
        ('numeric_success', lambda d: d['steps'][0].update(exit_code=False)),
        ('unlocked_build', lambda d: d['steps'][2]['command'].remove('--locked')),
        ('debug_assertions', lambda d: artifact_change(d, 'debug_assertions', False)),
        ('numeric_overflow', lambda d: artifact_change(d, 'overflow_checks', 1)),
        ('observer_test_mode', lambda d: artifact_change(d, 'test', True)),
        ('runtime_test_mode', lambda d: library_test(d, True)),
        ('runtime_numeric_test_mode', lambda d: library_test(d, 0)),
        ('missing_dataset', lambda d: d['results'].pop()),
        ('missing_input', lambda d: d['results'][0]['inputs'].pop()),
        ('missing_expected', lambda d: d['results'][0]['expected'].pop()),
        ('missing_observed', lambda d: d['results'][0]['observed'].pop()),
        ('wrong_binary', lambda d: d['results'][0].update(binary_sha256='0' * 64)),
        ('numeric_step', lambda d: d['results'][0].update(step=True)),
        ('blank_raw', lambda d: raw_change(d, 'blank')),
        ('missing_raw', lambda d: raw_change(d, 'missing')),
        ('extra_raw', lambda d: raw_change(d, 'extra')),
        ('changed_raw', lambda d: raw_change(d, 'value')),
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
            raise AssertionError('Lifecycle source validator accepted ' + name)
    native = read(ROOT / 'docs/parity/lifecycle_cli_admission_installed_5_0_6.json.gz')
    actual = {r['name']: r for r in native['native']}
    original = {r['name']: r for r in validator.source['results']}
    for name in ('resource_bindings', 'sample_progress'):
        inputs, expected, candidate = original[name]['inputs'], original[name]['observed'], actual[name]['observed']
        assert protocol.compare(ROOT, name, inputs, expected, candidate)
        mutations = [('extra_source_root', True), ('extra_native_root', False)]
        mutations += [('extra_source_round', True), ('extra_native_round', False)] if name == 'resource_bindings' else [('extra_log_field', True)]
        if name == 'resource_bindings':
            mutations += [(field, False) for field in ('expired_rejected', 'expired_copy_rejected')]
        for label, source_side in mutations:
            left, right = copy.deepcopy(expected), copy.deepcopy(candidate)
            selected = left if source_side else right
            if label in ('extra_source_root', 'extra_native_root'):
                selected[0]['unexpected_field'] = 'corruption'
            elif label in ('extra_source_round', 'extra_native_round'):
                selected[0]['ownership'][0]['unexpected_field'] = 'corruption'
            elif label == 'extra_log_field':
                next(v for v in selected if v['records'])['records'][0]['unexpected_field'] = 'corruption'
            else:
                selected[0]['ownership'][0][label] = False
            try:
                accepted = protocol.compare(ROOT, name, inputs, left, right)
            except errors as error:
                results.append(dict(name=name + ':' + label, rejected=True, reason=str(error)))
            else:
                assert not accepted, 'Lifecycle adapter accepted ' + name + ':' + label
                results.append(dict(name=name + ':' + label, rejected=True, reason='comparison differs'))
    tests = json.loads(subprocess.check_output(['ctest', '--test-dir', str(args.build_dir),
                                               '--show-only=json-v1'], text=True))['tests']
    for label in ('foreign_python', 'wrong_binary', 'extra_argument', 'missing_source_prerequisite'):
        changed = copy.deepcopy(tests)
        test = next(t for t in changed if t['name'] == 'bootstrap_extensions_rust_parity')
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
            raise AssertionError('Lifecycle CTest validator accepted ' + label)
    if args.record is not None:
        args.record.write_bytes(gzip.compress((json.dumps(dict(
            complete=True, whole_goal_complete=False, controls=results,
            registry_sha256=digest(REGISTRY)), indent=2) + '\n').encode(), mtime=0))
    print('Lifecycle source/adapter controls:', len(results), 'substitutions rejected; positive source/native accepted')


if __name__ == '__main__':
    main()
