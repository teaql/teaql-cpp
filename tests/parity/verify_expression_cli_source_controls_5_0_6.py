#!/usr/bin/env python3
"""Reject source, dependency, profile, raw-stream and CTest binding substitutions."""
import argparse
import copy
import gzip
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from expression_cli_source_provenance_5_0_6 import REGISTRY, Validator, read


def artifact_change(source, field, value):
    name = next(iter(source['compiler_artifacts']))
    artifact = source['compiler_artifacts'][name]
    if field in artifact['profile']:
        artifact['profile'][field] = value
    else:
        artifact[field] = value
    rows = [json.loads(line) for line in source['steps'][2]['stdout'].splitlines()]
    for index, row in enumerate(rows):
        if row.get('reason') == 'compiler-artifact' and row.get('executable') is not None \
                and row['target']['name'] == name:
            rows[index] = copy.deepcopy(artifact)
    source['steps'][2]['stdout'] = ''.join(json.dumps(row) + '\n' for row in rows)


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
    else:
        def change_text(value):
            if type(value) is dict:
                for key, child in value.items():
                    if type(child) is str:
                        value[key] = child + 'corruption'
                        return True
                    if change_text(child):
                        return True
            elif type(value) is list:
                for index, child in enumerate(value):
                    if type(child) is str:
                        value[index] = child + 'corruption'
                        return True
                    if change_text(child):
                        return True
            return False
        assert change_text(rows[0]), 'The corruption must change a real observed string'
    step['stdout'] = ''.join(json.dumps(row) + '\n' for row in rows)


def dataset_change(source):
    selection = source['dataset_contracts'][0]
    if type(selection) is dict:
        selection['invented'] = 'corruption'
    else:
        selection.append('invented')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, default=ROOT / 'build_native_current')
    parser.add_argument('--record', type=Path)
    parser.add_argument('--contracts', type=Path, default=REGISTRY)
    args = parser.parse_args()
    if args.record and args.record.exists():
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
        ('missing_source_file', lambda d: d['source_files'].pop(next(iter(d['source_files'])))),
        ('source_input_epoch', lambda d: d['input_hashes_after'].update(invented='0' * 64)),
        ('missing_archive_file', lambda d: d['archive_files'].pop(next(iter(d['archive_files'])))),
        ('changed_driver', lambda d: d['drivers'].update({next(iter(d['drivers'])): 'invented.rs'})),
        ('dataset_selection', dataset_change),
        ('missing_fixture', lambda d: d['generated_fixtures'].pop()),
        ('fixture_hand_edit', lambda d: d['generated_fixtures'][0]['provenance'].update(hand_edited=True)),
        ('manifest', lambda d: d['derived_observer'].update(manifest=d['derived_observer']['manifest'] + '\n')),
        ('lock', lambda d: d['derived_observer'].update(compiled_lock=d['derived_observer']['compiled_lock'] + '\n')),
        ('seed_lock', lambda d: d['derived_observer'].update(seed_lock_sha256='0' * 64)),
        ('dependency_differences', lambda d: d.update(registry_identity_differences=[['invented']])),
        ('source_environment', lambda d: d['cargo_environment'].update(CARGO_INCREMENTAL='1')),
        ('numeric_success', lambda d: d['steps'][0].update(exit_code=False)),
        ('failed_build', lambda d: d['steps'][2].update(exit_code=1)),
        ('unlocked_build', lambda d: d['steps'][2]['command'].remove('--locked')),
        ('missing_artifact', lambda d: d['compiler_artifacts'].pop(next(iter(d['compiler_artifacts'])))),
        ('debug_assertions', lambda d: artifact_change(d, 'debug_assertions', False)),
        ('numeric_overflow', lambda d: artifact_change(d, 'overflow_checks', 1)),
        ('test_profile', lambda d: artifact_change(d, 'test', True)),
        ('release_profile', lambda d: artifact_change(d, 'opt_level', '3')),
        ('package_identity', lambda d: artifact_change(d, 'package_id', 'invented')),
        ('missing_dataset', lambda d: d['results'].pop()),
        ('missing_input', lambda d: d['results'][0]['inputs'].pop()),
        ('missing_expected', lambda d: d['results'][0]['expected'].pop()),
        ('missing_observed', lambda d: d['results'][0]['observed'].pop()),
        ('wrong_dataset', lambda d: d['results'][0].update(corpus='invented.json.gz')),
        ('observer_binary', lambda d: d['results'][0].update(binary_sha256='0' * 64)),
        ('numeric_step', lambda d: d['results'][0].update(step=True)),
        ('missing_raw_output', lambda d: raw_change(d, 'missing')),
        ('extra_raw_output', lambda d: raw_change(d, 'extra')),
        ('blank_raw_record', lambda d: raw_change(d, 'blank')),
        ('corrupt_raw_output', lambda d: raw_change(d, 'value')),
        ('cleanup', lambda d: d.update(owned_directory_removed=False)),
        ('numeric_removed_bytes', lambda d: d.update(owned_bytes_removed=True)),
    ]
    results = []
    for name, mutate in controls:
        corrupted = copy.deepcopy(validator.source)
        mutate(corrupted)
        assert not validator.same(corrupted, validator.source), name
        try:
            validator.validate_source(corrupted)
        except (RuntimeError, ValueError, KeyError, TypeError, StopIteration) as error:
            results.append(dict(name=name, rejected=True, reason=str(error)))
        else:
            raise AssertionError('Source validator accepted ' + name)
    if validator.kind == 'query':
        contract = next(c for c in validator.registry['contracts'] if c.get('selection'))
        corpus = read(ROOT / contract['corpus'])
        mutations = [
            ('erased_partition', lambda c, d: c.update(selection=None)),
            ('numeric_partition', lambda c, d: c.update(selection=True)),
            ('wrong_partition_field', lambda c, d: c['selection'].update(field='entity')),
            ('unknown_partition', lambda c, d: c['selection'].update(value='invented')),
            ('reversed_domain_inventory', lambda c, d: c['selection']['complete_values'].reverse()),
            ('unknown_input_domain', lambda c, d: d['cases'][0]['input'].update(domain='invented')),
            ('numeric_input_domain', lambda c, d: d['cases'][0]['input'].update(domain=1)),
        ]
        for name, mutate in mutations:
            altered_contract, altered_corpus = copy.deepcopy(contract), copy.deepcopy(corpus)
            mutate(altered_contract, altered_corpus)
            try:
                validator.selected_rows(altered_contract, altered_corpus)
            except (RuntimeError, ValueError, KeyError, TypeError) as error:
                results.append(dict(name=name, rejected=True, reason=str(error)))
            else:
                raise AssertionError('Domain partition accepted ' + name)
        altered_corpus = copy.deepcopy(corpus)
        rows = [r for r in altered_corpus['cases'] if r['input']['domain'] == 'dynamic']
        altered_corpus['cases'] = [copy.deepcopy(rows[i % len(rows)])
                                   for i in range(len(corpus['cases']))]
        try:
            validator.selected_rows(contract, altered_corpus)
        except (RuntimeError, ValueError, KeyError, TypeError) as error:
            results.append(dict(name='same_length_missing_domain', rejected=True, reason=str(error)))
        else:
            raise AssertionError('Domain partition accepted an incomplete same-length corpus')
    if validator.kind == 'dynamic_json':
        for name, mutate in [
                ('changed_public_fixture_key', lambda d: d['cargo_environment'].update(
                    TEAQL_BUSINESS_ID_KEY_V1_HEX='11' * 32)),
                ('changed_public_fixture_version', lambda d: d['cargo_environment'].update(
                    TEAQL_BUSINESS_ID_KEY_VERSION='2')),
                ('missing_public_fixture_key', lambda d: d['cargo_environment'].pop(
                    'TEAQL_BUSINESS_ID_KEY_V1_HEX'))]:
            altered = copy.deepcopy(validator.source)
            mutate(altered)
            try:
                validator.validate_source(altered)
            except (RuntimeError, ValueError, KeyError, TypeError) as error:
                results.append(dict(name=name, rejected=True, reason=str(error)))
            else:
                raise AssertionError('Dynamic JSON source accepted ' + name)
        registry = copy.deepcopy(validator.registry)
        registry['contracts'][0]['adapter'] = 'identity'
        probe = Validator(ROOT, registry).load()
        try:
            probe.validate_source(probe.source)
        except (RuntimeError, ValueError, KeyError, TypeError) as error:
            results.append(dict(name='changed_declared_adapter', rejected=True, reason=str(error)))
        else:
            raise AssertionError('Dynamic JSON source accepted a different adapter')
        registry = copy.deepcopy(validator.registry)
        registry['kind'] = 'unknown_protocol'
        try:
            Validator(ROOT, registry)
        except RuntimeError as error:
            results.append(dict(name='unknown_source_protocol', rejected=True, reason=str(error)))
        else:
            raise AssertionError('Source validator accepted an unknown protocol')
    for name, mutate in [
            ('wrong_registered_binary', lambda c: c.update(binary='invented')),
            ('wrong_registered_comparator', lambda c: c.update(comparator='invented.py')),
            ('wrong_registered_test', lambda c: c.update(test='invented'))]:
        registry = copy.deepcopy(validator.registry)
        mutate(registry['contracts'][0])
        probe = Validator(ROOT, registry)
        try:
            probe.validate_ctest(args.build_dir)
        except (RuntimeError, ValueError, KeyError) as error:
            results.append(dict(name=name, rejected=True, reason=str(error)))
        else:
            raise AssertionError('CTest binding accepted ' + name)
    if args.record:
        args.record.write_bytes(gzip.compress((json.dumps(dict(
            complete=True,whole_goal_complete=False,controls=results,
            registry_sha256=hashlib.sha256(args.contracts.read_bytes()).hexdigest(),
            verifier_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()),
            indent=2)+'\n').encode(),mtime=0))
    print('Rejected', len(results), 'source/profile/raw-stream/CTest substitutions')


if __name__ == '__main__':
    main()

