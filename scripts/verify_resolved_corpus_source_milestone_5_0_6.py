#!/usr/bin/env python3
"""Reconcile the seven resolved-operation source bindings without promoting whole parity."""
import argparse
from collections import Counter
import gzip
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tests/parity'))
from compare_values import same


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def path(name):
    return ROOT / 'docs/parity' / ('resolved_corpus_source_' + name + '_5_0_6.json.gz')


def read(filename):
    return json.loads(gzip.decompress(filename.read_bytes()) if filename.suffix == '.gz' else filename.read_bytes())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    assert not args.record.exists() and not path('cleanup').exists()
    names = ['initial', 'current_native', 'controls', 'registered_gates', 'dispatcher', 'inventory', 'guards']
    receipts = {name: read(path(name)) for name in names}
    assert all(r['complete'] is True for r in receipts.values())
    registry_path = ROOT / 'tests/parity/resolved_corpus_source_contracts_5_0_6.json'
    registry = json.loads(registry_path.read_text())
    contracts = registry['contracts']
    assert len(contracts) == 7 and sum(c['count'] for c in contracts) == 30410
    archived = {c['name'] for c in contracts if c['observer_version'] == 'archived'}
    assert archived == {'enhanced_aggregate', 'query_entry_cursor', 'query_entry_protocol'}
    for name in ['initial', 'current_native', 'registered_gates']:
        receipt = receipts[name]
        assert receipt['input_hashes_before'] == receipt['input_hashes_after']
        assert all(digest(Path(p)) == sha for p, sha in receipt['input_hashes_after'].items())
    for name in ['initial', 'current_native']:
        observed = receipts[name]['contracts']
        assert [c['name'] for c in observed] == [c['name'] for c in contracts]
        for contract, result in zip(contracts, observed):
            corpus = read(ROOT / contract['corpus'])
            assert result['validation']['observations'] == contract['count']
            assert result['validation']['observer_version'] == contract['observer_version']
            assert result['validation']['observer_snapshot_sha256'] == contract['observer_snapshot_sha256']
            assert result['validation']['original_source_files'] == 67
            assert result['validation']['registry_packages'] == 119
            if name == 'initial':
                assert result['current_observations'] is None and result['current_binary_sha256'] is None
            else:
                assert same(result['current_observations'], [c['expected'] for c in corpus['cases']])
                assert result['current_binary_sha256'] == digest(ROOT / 'build_native_current' / contract['native_target'])
    controls = receipts['controls']
    assert len(controls['controls']) == 20 and all(c['rejected'] for c in controls['controls'])
    assert set(controls['positive_archived_contracts']) == archived
    gates = receipts['registered_gates']
    assert gates['registered_tests'] == 883
    assert [len(s['tests']) for s in gates['steps']] == [15, 2]
    assert all(s['exit_code'] == 0 for s in gates['steps'])
    assert set(gates['steps'][0]['tests']) == set(gates['expected_tests'])
    dispatcher = receipts['dispatcher']
    assert dispatcher['inputs_unchanged'] is True and dispatcher['accepted_mutations'] == 0
    assert len(dispatcher['cases']) == 1 and dispatcher['cases'][0]['invariant'] is True
    assert dispatcher['cases'][0]['test'] == 'parity_resolved_corpus_source_controls'
    inventory = receipts['inventory']['inventory']
    assert inventory['registered_tests'] == 883
    assert inventory['counts'] == {'active_baseline': 305, 'custom_dispatcher_requires_review': 172}
    baseline_only = []
    for row in inventory['tests']:
        if row['status'] == 'active_baseline':
            corpus = ROOT / row['corpus']
            assert digest(corpus) == row['sha256']
            if set(read(corpus)) == {'baseline', 'cases'}:
                baseline_only.append(row['corpus'])
    assert len(baseline_only) == len(set(baseline_only)) == 27
    context = json.loads((ROOT / 'tests/parity/corpus_source_contracts_5_0_6.json').read_text())['contracts']
    bound = {c['corpus'] for c in context + contracts}
    assert len(bound) == 11 and bound <= set(baseline_only)
    assert sum(c['count'] for c in context + contracts) == 36266
    ledger = json.loads((ROOT / 'docs/cpp_generator_parity_ledger.json').read_text())
    previous = read(path('ledger_before'))
    assert Counter(c['status'] for c in ledger['capabilities']) == {'qualified': 14, 'partial': 3}
    for old, current in zip(previous['capabilities'], ledger['capabilities']):
        assert {k: v for k, v in old.items() if k != 'evidence'} == {
            k: v for k, v in current.items() if k != 'evidence'}
        assert current['evidence'][:len(old['evidence'])] == old['evidence']
    cleanup = []
    for name, receipt in receipts.items():
        if 'owned_directory' not in receipt:
            continue
        owned = Path(receipt['owned_directory'])
        assert owned.parent == Path('/tmp') and owned.name.startswith((
            'teaql_corpus_source_', 'teaql_resolved_corpus_', 'teaql_dispatcher_result_controls_506_'))
        assert receipt['owned_directory_removed'] is True and not owned.exists()
        cleanup.append(dict(receipt=name, path=str(owned), removed=True, measured_bytes=receipt.get(
            'owned_bytes_removed', receipt.get('owned_bytes_before_cleanup'))))
    cleanup_record = dict(complete=True, paths=cleanup,
                          measured_bytes_removed=sum(c['measured_bytes'] for c in cleanup),
                          retained='Stable presets, source fixtures and compact original/current execution evidence')
    path('cleanup').write_bytes(gzip.compress(json.dumps(cleanup_record).encode(), mtime=0))
    inputs = [Path(__file__), registry_path, ROOT / 'scripts/verify_corpus_source_contracts_5_0_6.py',
              ROOT / 'tests/parity/verify_corpus_source_controls_5_0_6.py', ROOT / 'tests/CMakeLists.txt',
              ROOT / 'scripts/verify_dispatcher_result_controls_5_0_6.py',
              ROOT / 'docs/resolved_corpus_source_provenance_5_0_6.md',
              ROOT / 'docs/context_corpus_source_provenance_5_0_6.md',
              ROOT / 'docs/cpp_generator_parity_ledger.json', path('ledger_before'), path('cleanup'),
              *[path(name) for name in names]]
    record = dict(complete=True, whole_goal_complete=False, whole_provenance_complete=False,
                  scope=__doc__, runtime_commit=registry['runtime_commit'], generator_commit=registry['generator_commit'],
                  new_bindings=7, new_observations=30410, cumulative_bindings=11, cumulative_observations=36266,
                  archived_observer_versions=sorted(archived), effective_corruptions_rejected=20,
                  native_checks=15, focused_fixture_checks=2,
                  baseline_only_remaining=sorted(set(baseline_only) - bound), custom_registrations_open=172,
                  input_hashes={str(p): digest(p) for p in inputs})
    args.record.write_bytes(gzip.compress(json.dumps(record).encode(), mtime=0))
    print('Seven source bindings / 30410 current native outputs reconcile; 16 baseline-only corpora remain open')


if __name__ == '__main__':
    main()
