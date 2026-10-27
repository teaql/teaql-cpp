#!/usr/bin/env python3
"""Reject corrupt provenance and result data through the custom source validator."""
import argparse
import copy
import gzip
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
from unittest.mock import patch


def mutate_type(value):
    if isinstance(value, dict):
        for key, child in value.items():
            if type(child) is bool:
                value[key] = int(child)
                return True
            if type(child) is int:
                value[key] = float(child)
                return True
            if mutate_type(child):
                return True
    elif isinstance(value, list):
        for index, child in enumerate(value):
            if type(child) is bool:
                value[index] = int(child)
                return True
            if type(child) is int:
                value[index] = float(child)
                return True
            if mutate_type(child):
                return True
    return False


def check_ctest_fixture(build_dir):
    registry = json.loads(subprocess.check_output(
        ['ctest', '--test-dir', str(build_dir), '--show-only=json-v1'], text=True))
    tests = {item['name']: item for item in registry['tests']}
    required = ('transaction_scope_intent_rust_parity',
                'initialized_snapshot_cache_rust_parity',
                'deferred_facet_page_rust_parity',
                'deferred_resource_replacement_contracts',
                'custom_public_cli_source_rust_parity')
    for name in ('parity_custom_public_cli_source_contracts', *required):
        assert name in tests, f'missing custom source test: {name}'
        properties = {item['name']: item['value'] for item in tests[name]['properties']}
        property_name = ('FIXTURES_SETUP' if name == 'parity_custom_public_cli_source_contracts'
                         else 'FIXTURES_REQUIRED')
        assert 'custom_public_cli_source' in properties.get(property_name, []), (
            f'missing custom source fixture on {name}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument('--registry', type=Path)
    parser.add_argument('--verifier', type=Path)
    parser.add_argument('--record', type=Path)
    parser.add_argument('--ctest-build-dir', type=Path)
    args = parser.parse_args()
    if args.ctest_build_dir:
        check_ctest_fixture(args.ctest_build_dir)
    root = args.root.resolve()
    registry_path = args.registry or root / 'tests/parity/custom_public_cli_source_contracts_5_0_6.json'
    verifier = args.verifier or root / 'scripts/custom_public_cli_source_provenance_5_0_6.py'
    registry = json.loads(registry_path.read_text())
    assert hashlib.sha256(verifier.read_bytes()).hexdigest() == registry['verifier_sha256']
    if args.record:
        assert not args.record.exists()
    spec = importlib.util.spec_from_file_location('custom_source_validator', verifier)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    validator = module.Validator(root, registry).load().validate()
    record = dict(complete=False, passed=False, native_execution=False, fresh_rust_execution=False,
                  whole_goal_complete=False,
                  scope='Controls execute the actual validator on explicit copies of qualified artifacts and results.',
                  results=[])

    def run(name, action, error=None):
        observed = None
        try:
            action()
            accepted = True
        except (module.QualificationError, json.JSONDecodeError) as exception:
            accepted = False
            observed = type(exception).__name__ + ': ' + str(exception)
        passed = accepted if error is None else not accepted and error in observed
        record['results'].append(dict(name=name, expected_error=error, accepted=accepted,
                                      error=observed, passed=passed))

    def changed(name, side, path, value, error):
        data = copy.deepcopy(validator.source if side == 'source' else validator.binding)
        target = data
        for key in path[:-1]:
            target = target[key]
        target[path[-1]] = value
        method = validator.validate_source if side == 'source' else validator.validate_binding
        run(name, lambda: method(data), error)

    run('valid_retained_evidence', lambda: validator.validate())
    changed('source_complete_numeric', 'source', ['complete'], 1, 'original execution scope')
    changed('source_native_flag_numeric', 'source', ['native_execution'], 0, 'original execution scope')
    changed('source_wrong_baseline', 'source', ['runtime_commit'], '0' * 40, 'source baseline')
    changed('source_wrong_contract', 'source', ['contracts', 0, 'count'], 1, 'source contracts')
    driver = registry['contracts'][0]['driver']
    changed('source_wrong_driver_hash', 'source', ['source_files', driver], '0' * 64, 'source input identities')
    epoch_key = next(iter(validator.source['input_hashes_before']))
    changed('source_changed_epoch', 'source', ['input_hashes_after', epoch_key], '0' * 64,
            'original execution input epoch')
    changed('source_manifest_drift', 'source', ['generated_manifest'],
            validator.source['generated_manifest'] + '\n# undeclared change\n', 'original generated manifest')
    changed('source_lock_drift', 'source', ['compiled_lock'],
            validator.source['compiled_lock'] + '\n# undeclared change\n', 'original compiled lock')
    first = registry['contracts'][0]
    target = first['target']
    changed('source_numeric_debug_flag', 'source',
            ['compiler_artifacts', target, 'profile', 'debug_assertions'], 1, 'original Debug/overflow profile')
    changed('source_unchecked_arithmetic', 'source',
            ['compiler_artifacts', target, 'profile', 'overflow_checks'], False, 'original Debug/overflow profile')
    changed('source_external_observer_path', 'source',
            ['compiler_artifacts', target, 'target', 'src_path'], '/outside/observer.rs', 'original observer path')
    source_result = next(v for v in validator.source['results'] if v['name'] == first['name'])
    step = source_result['source_step']
    changed('source_numeric_exit_flag', 'source', ['steps', step, 'exit_code'], False, 'original observer exit')
    raw = validator.source['steps'][step]['stdout']
    rows = validator.lines(raw)
    changed('source_missing_output', 'source', ['steps', step, 'stdout'],
            ''.join(json.dumps(v) + '\n' for v in rows[:-1]), 'original output cardinality')
    changed('source_wrong_stdin', 'source', ['steps', step, 'stdin_sha256'], '0' * 64,
            'original serialized input identity')
    for name, mutation, expected in [
        ('metadata_external_local_package', 'path', 'local package path'),
        ('metadata_duplicate_local_package', 'duplicate', 'duplicate local package identity'),
        ('metadata_unlocked_registry_package', 'registry', 'metadata registry package identities'),
    ]:
        data = copy.deepcopy(validator.source)
        metadata = json.loads(data['project_metadata'])
        local = next(p for p in metadata['packages'] if p['source'] is None)
        if mutation == 'path':
            local['manifest_path'] = '/outside/Cargo.toml'
        elif mutation == 'duplicate':
            metadata['packages'].append(copy.deepcopy(local))
        else:
            metadata['packages'].append(dict(name='unlocked-observer-dependency', version='99.0.0',
                source='registry+https://github.com/rust-lang/crates.io-index'))
        data['project_metadata'] = json.dumps(metadata)
        run(name, lambda: validator.validate_source(data), expected)
    changed('binding_numeric_native_flag', 'binding', ['native_execution'], 1, 'native binding scope')
    changed('binding_false_fresh_rust_claim', 'binding', ['fresh_rust_execution'], True, 'native binding scope')
    changed('binding_wrong_source_identity', 'binding', ['source_execution_sha256'], '0' * 64,
            'native original-execution identity')
    changed('binding_float_count', 'binding', ['observations'], 2093.0, 'native binding counts')
    changed('binding_float_control_count', 'binding', ['controls_passed'], 31.0, 'retained boundary controls')
    changed('binding_numeric_control_flag', 'binding', ['controls', 0, 'passed'], 1, 'retained boundary controls')
    result = validator.binding['results'][0]
    native_step = result['native_step']
    changed('binding_numeric_exit', 'binding', ['steps', native_step, 'exit_code'], False, 'native observer exit')
    changed('binding_executable_drift', 'binding', ['results', 0, 'native_binary_sha256'], '0' * 64,
            'native executable identity')
    altered = copy.deepcopy(result['native'][0])
    assert mutate_type(altered)
    changed('binding_raw_stream_drift', 'binding', ['results', 0, 'native', 0], altered,
            'native raw stream identity')
    altered = copy.deepcopy(result['original'][0])
    assert mutate_type(altered)
    changed('binding_retained_source_drift', 'binding', ['results', 0, 'original', 0], altered,
            'retained original stream identity')
    for name in ['source_execution', 'native_binding']:
        wrong = copy.deepcopy(registry)
        wrong[name]['sha256'] = '0' * 64
        run(name + '_artifact_corruption', lambda: module.Validator(root, wrong).load(), name + ' artifact hash')
    for contract in registry['contracts']:
        stored = next(v for v in validator.binding['results'] if v['name'] == contract['name'])
        current = copy.deepcopy(stored['native'])
        run(contract['name'] + '/missing_native', lambda: validator.compare(contract, current[:-1]),
            'complete observer cardinality')
        run(contract['name'] + '/extra_native', lambda: validator.compare(contract, current + [current[0]]),
            'complete observer cardinality')
        for side in ['source', 'native']:
            source_copy = copy.deepcopy(validator.source)
            native_copy = copy.deepcopy(current)
            if side == 'native':
                rows = native_copy
            else:
                index = next(v['source_step'] for v in source_copy['results'] if v['name'] == contract['name'])
                rows = validator.lines(source_copy['steps'][index]['stdout'])
            field = rows[0]['common'] if contract['projection'] == 'stable_common_and_weak_ownership' else rows[0]
            assert mutate_type(field)
            if side == 'source':
                source_copy['steps'][index]['stdout'] = ''.join(json.dumps(v) + '\n' for v in rows)
            expected = ('cache' if contract['projection'] == 'stable_common_and_weak_ownership' else
                        'encoded JSON' if contract['projection'] == 'exact_encoded' else 'complete JSON')
            with patch.object(validator, 'source', source_copy):
                run(contract['name'] + '/' + side + '_type',
                    lambda: validator.compare(contract, native_copy), expected)
        bad = copy.deepcopy(current)
        bad[0].pop('common' if contract['projection'] == 'stable_common_and_weak_ownership'
                   else next(iter(bad[0])))
        run(contract['name'] + '/missing_field', lambda: validator.compare(contract, bad),
            'cache' if contract['projection'] == 'stable_common_and_weak_ownership' else
            'encoded JSON' if contract['projection'] == 'exact_encoded' else 'complete JSON')
        bad_corpus = copy.deepcopy(validator.corpus(contract))
        bad_corpus['cases'] = bad_corpus['cases'][:-1]
        with patch.object(validator, 'corpus', return_value=bad_corpus):
            run(contract['name'] + '/missing_input', lambda: validator.inputs(contract),
                'complete input builder matrix')
        raw = validator.binding['steps'][stored['native_step']]['stdout']
        first_lf = raw.index('\n')
        raw = raw[:first_lf + 1] + '\n' + raw[first_lf + 1:]
        run(contract['name'] + '/extra_blank_record', lambda: validator.lines(raw), 'JSONDecodeError')
        if contract['projection'] == 'stable_common_and_weak_ownership':
            for side in ['source', 'native']:
                source_copy = copy.deepcopy(validator.source)
                native_copy = copy.deepcopy(current)
                if side == 'source':
                    index = next(v['source_step'] for v in source_copy['results'] if v['name'] == contract['name'])
                    values = validator.lines(source_copy['steps'][index]['stdout'])
                else:
                    values = native_copy
                values[-1]['ownership']['within_row'][0] = int(values[-1]['ownership']['within_row'][0])
                if side == 'source':
                    source_copy['steps'][index]['stdout'] = ''.join(json.dumps(v) + '\n' for v in values)
                with patch.object(validator, 'source', source_copy):
                    run('cache/' + side + '_numeric_ownership', lambda: validator.compare(contract, native_copy),
                        'cache')
            source_copy = copy.deepcopy(validator.source)
            native_copy = copy.deepcopy(current)
            index = next(v['source_step'] for v in source_copy['results'] if v['name'] == contract['name'])
            values = validator.lines(source_copy['steps'][index]['stdout'])
            for old, new in zip(values[:-1], native_copy[:-1]):
                old['diagnostics'] = 'Independent original Debug'
                new['diagnostics'] = 'Independent native Debug'
            source_copy['steps'][index]['stdout'] = ''.join(json.dumps(v) + '\n' for v in values)
            with patch.object(validator, 'source', source_copy):
                run('cache/independent_diagnostics', lambda: validator.compare(contract, native_copy))
    for name, transition in registry['script_transitions'].items():
        original = (root / name).read_text()
        wanted = registry['evidence_source_files'][name]
        if hashlib.sha256(original.encode()).hexdigest() != wanted:
            for item in reversed(transition['replacements']):
                original = original.replace(item['new'], item['old'])
        candidate = original
        for item in transition['replacements']:
            candidate = candidate.replace(item['old'], item['new'])
        run('transition/original', lambda: module.check_transition(original, wanted, transition))
        run('transition/candidate', lambda: module.check_transition(candidate, wanted, transition))
        run('transition/undeclared_source_change',
            lambda: module.check_transition(candidate + '\n# extra change\n', wanted, transition),
            'declared comparator transition')
        wrong = copy.deepcopy(transition)
        wrong['replacements'][0]['new'] = 'nonexistent candidate fragment'
        run('transition/missing_inverse_fragment', lambda: module.check_transition(candidate, wanted, wrong),
            'inverse comparator patch')
        wrong = copy.deepcopy(transition)
        wrong['replacements'][0]['old'] += '\n# incorrect reconstruction'
        run('transition/wrong_original_reconstruction', lambda: module.check_transition(candidate, wanted, wrong),
            'unmodified original comparator outside repair')
    record['complete'] = True
    record['passed'] = all(row['passed'] for row in record['results'])
    if args.record:
        args.record.write_bytes(gzip.compress(json.dumps(record).encode(), mtime=0))
    failed = [row['name'] for row in record['results'] if not row['passed']]
    print(f'Custom source controls: {len(record["results"]) - len(failed)}/{len(record["results"])} pass')
    if failed:
        print('Unexpected controls:', ', '.join(failed))
    return 0 if record['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
