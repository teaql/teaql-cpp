#!/usr/bin/env python3
"""Reconcile G17 source owners, permanent module registrations and current execution."""
import argparse
from collections import Counter
import gzip
import hashlib
import json
from pathlib import Path
import re

from verify_generated_identity_save_5_0_6 import same

ROOT = Path(__file__).resolve().parents[1]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def report_path(name):
    return ROOT / 'docs/parity' / (name + '_5_0_6.json.gz')


def read(name):
    return json.loads(gzip.decompress(report_path(name).read_bytes()))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    assert not args.record.exists()
    matrix_path = ROOT / 'docs/governance_qualification_requirements_5_0_6.json'
    matrix = json.loads(matrix_path.read_text())
    old_matrix = read('governance_requirements_before_reconciliation')
    assert hashlib.sha256(old_matrix['source'].encode()).hexdigest() == old_matrix['sha256']
    previous = json.loads(old_matrix['source'])
    assert matrix['historical_question_mapping'] == previous['historical_question_mapping']
    assert len(matrix['historical_question_mapping']) == 22
    assert len(matrix['requirements']) == 16
    for old, current in zip(previous['requirements'], matrix['requirements']):
        for key in ('id', 'responsibility', 'tests', 'remaining'):
            assert current[key] == old[key], (current['id'], key)
        added = set(current['sources']) - set(old['sources'])
        assert added == ({'teaql-runtime/src/error.rs'} if current['id'] == 'GP08' else set())
        assert set(old['sources']) <= set(current['sources'])
    assert Counter(r['status'] for r in matrix['requirements']) == {
        'reconciled_runtime_contract': 13, 'open': 3}
    assert {r['id'] for r in matrix['requirements'] if r['status'] == 'open'} == {'GP11', 'GP13', 'GP16'}
    sources = json.loads((ROOT / 'docs/parity/v5_0_6/source_dispositions.json').read_text())
    sources = {r['source']: r for r in sources['sources']}
    owners = sorted({s for r in matrix['requirements'] for s in r['sources']})
    assert len(owners) == 35
    source_owners = []
    for owner in owners:
        accepted = sources[owner]
        assert accepted['status'] == 'accepted'
        path = ROOT / '.reference/teaql-rs' / owner
        assert digest(path) == accepted['sha256']
        source_owners.append(dict(source=owner, sha256=digest(path),
                                  cpp_target=accepted['cpp_target'], evidence=accepted['evidence'],
                                  acceptance_scope=accepted.get('acceptance_scope')))
    native = read('governance_join_native_verified')
    module = read('governance_join_modules_final')
    names = {t for r in matrix['requirements'] for t in r['tests']}
    assert len(names) == 40
    driver = ROOT / 'scripts/verify_governance_join_5_0_6.py'
    archived_driver = read('governance_join_driver_before_path_fix')
    assert hashlib.sha256(archived_driver['source'].encode()).hexdigest() == archived_driver['sha256']
    assert native['input_hashes_before'][str(driver)] == archived_driver['sha256']
    for result, expected in [(native, names), (module, {'module_' + n for n in names})]:
        assert result['complete'] is True and set(result['tests']) == expected
        assert result['requirements_sha256'] == old_matrix['sha256']
        assert result['input_hashes_before'] == result['input_hashes_after']
        assert result['executable_hashes_before'] == result['executable_hashes_after']
        for filename, sha in result['input_hashes_after'].items():
            if result is native and filename == str(driver):
                continue
            assert digest(Path(filename)) == sha, filename
        for filename, sha in result['executable_hashes_after'].items():
            assert digest(Path(filename)) == sha, filename
    original_registry = json.loads(next(s for s in native['steps'] if s['label'] == 'registry')['stdout'])
    registered = {t['name']: t for t in original_registry['tests']}
    assert all(registered[n]['command'] == native['registered_tests'][n]['command'] for n in names)
    save = read('g17_generated_save_current')
    controls = read('g17_generated_save_current_controls')
    assert save['complete'] is True and controls['complete'] is True
    assert not save.get('reused_source') and save['runtime_overlay'] is None and save['generator_overlay'] is None
    assert len(save['cases']) == 5 and len(save['inputs']) == 72
    for case in save['cases']:
        assert case['registry_identity_matches'] is True and not case['differences']
        assert len(case['original']) == len(case['cpp']) == 72 and same(case['original'], case['cpp'])
    assert len(controls['rejected_mutations']) == 10
    assert controls['coverage'] == dict(observations=360, successes=56, plans=168,
                                       events=100, logs=232, committed_audit_errors=44)
    for filename, sha in save['input_hashes_after'].items():
        assert digest(Path(filename)) == sha, filename
    before = read('governance_module_registration_before')['files']
    contracts = json.loads((ROOT / 'modules/contracts.json').read_text())
    old_contracts = json.loads(before['modules/contracts.json']['source'])
    new = [c for c in contracts['contracts'] if c['stem'] == 'commit_audit_error_probe']
    assert new == [dict(package='teaql_runtime', stem='commit_audit_error_probe',
                        source='tests/parity/commit_audit_error_probe.cpp', cases=[])]
    contracts['contracts'].remove(new[0])
    assert contracts == old_contracts
    presets = json.loads((ROOT / 'CMakePresets.json').read_text())
    preset = next(p for p in presets['buildPresets'] if p['name'] == 'modules_native')
    preset['targets'].remove('teaql_module_commit_audit_error_probe')
    assert presets == json.loads(before['CMakePresets.json']['source'])
    cmake = (ROOT / 'modules/CMakeLists.txt').read_text()
    start = cmake.index('    add_test(NAME module_commit_audit_source_rust_parity')
    end = cmake.index('    add_test(NAME module_dynamic_carrier_rust_parity', start)
    added = set(re.findall(r'add_test\(NAME (\w+)', cmake[start:end]))
    assert added == {'module_commit_audit_source_rust_parity', 'module_commit_audit_error_rust_parity',
                     'module_resolved_scope_privacy_rust_parity', 'module_resolved_cache_intent_rust_parity'}
    assert cmake[:start] + cmake[end:] == before['modules/CMakeLists.txt']['source']
    initial_guards = read('governance_join_final_guards')
    assert initial_guards['complete'] is False
    assert [s['exit_code'] for s in initial_guards['steps']] == [0, 0, 0, 2]
    assert '--ctest-json' in initial_guards['steps'][-1]['stderr']
    guards = read('governance_join_remaining_guards')
    assert guards['complete'] is True and all(s['exit_code'] == 0 for s in guards['steps'])
    ledger = json.loads((ROOT / 'docs/cpp_generator_parity_ledger.json').read_text())
    g17 = next(c for c in ledger['capabilities'] if c['id'] == 'G17')
    archived_g17 = read('governance_ledger_before_reconciliation')['G17']
    assert set(archived_g17['runtime_sources']) <= set(owners)
    assert g17['runtime_sources'] == owners
    assert g17['runtime_source_hashes'] == {s['source']: s['sha256'] for s in source_owners}
    assert Counter(c['status'] for c in ledger['capabilities']) == {'qualified': 14, 'partial': 3}
    cleanup = []
    for name in ['g17_generated_save_current', 'governance_join_native',
                 'governance_join_native_verified', 'governance_join_modules_final',
                 'governance_join_remaining_guards']:
        value = read(name)
        if 'owned_directory' not in value:
            continue
        owned = Path(value['owned_directory'])
        assert owned.parent == Path('/tmp') and owned.name.startswith((
            'teaql_generated_identity_save_', 'teaql_governance_join_', 'teaql_governance_root_registry_'))
        assert value['owned_directory_removed'] is True and not owned.exists()
        cleanup.append(dict(receipt=name, path=str(owned), removed=True,
                            measured_bytes=value.get('owned_bytes_removed')))
    cleanup_path = report_path('governance_join_cleanup')
    assert not cleanup_path.exists()
    cleanup_path.write_bytes(gzip.compress(json.dumps(dict(
        complete=True, scope='Exact task-owned disposable paths; stable profiles/shared caches retained',
        paths=cleanup, measured_bytes_removed=sum(c['measured_bytes'] or 0 for c in cleanup))).encode(), mtime=0))
    inputs = [Path(__file__), matrix_path, ROOT / 'docs/governance_qualification_requirements_5_0_6.md',
              ROOT / 'docs/cpp_generator_parity_ledger.json', cleanup_path,
              *[report_path(n) for n in ['governance_join_native_verified', 'governance_join_modules_final',
                 'g17_generated_save_current', 'g17_generated_save_current_controls',
                 'governance_join_final_guards', 'governance_join_remaining_guards',
                 'governance_module_registration_before',
                 'governance_requirements_before_reconciliation', 'governance_ledger_before_reconciliation',
                 'governance_join_driver_before_path_fix']]]
    args.record.write_bytes(gzip.compress(json.dumps(dict(
        complete=True, whole_goal_complete=False, scope=__doc__, source_owners=source_owners,
        reconciled=13, open_requirements=['GP11', 'GP13', 'GP16'], native_checks=40, imported_checks=40,
        actual_source_save_observations=360, strict_result_controls=10,
        source_adaptation='Native execution uses the archived driver before module-only path resolution; actual registered commands required no fallback',
        input_hashes={str(p): digest(p) for p in inputs})).encode(), mtime=0))
    print('G17 reconciles13 runtime contracts;35 owners/native40/imported40/source360; GP11/GP13/GP16 remain open')


if __name__ == '__main__':
    main()
