#!/usr/bin/env python3
"""Reconcile complete composite-source evidence while preserving all remaining parity obligations."""
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


def read(path):
    data = path.read_bytes()
    return json.loads(gzip.decompress(data) if path.suffix == '.gz' else data)


def report(name):
    return ROOT / 'docs/parity' / ('composite_corpus_source_' + name + '_5_0_6.json.gz')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    assert not args.record.exists() and not report('cleanup').exists()
    names = ['initial', 'current_native', 'controls', 'registered_gates', 'dispatcher', 'inventory', 'guards']
    receipts = {name: read(report(name)) for name in names}
    assert all(r['complete'] is True for r in receipts.values())
    registry_paths = [ROOT / 'tests/parity' / (name + '_5_0_6.json') for name in (
        'corpus_source_contracts', 'resolved_corpus_source_contracts', 'composite_corpus_source_contracts')]
    registries = [read(p) for p in registry_paths]
    contracts = registries[-1]['contracts']
    assert len(contracts) == 9 and sum(c['count'] for c in contracts) == 16373
    assert sum(p['version'] == 'archived' for c in contracts for p in c['asset_versions'].values()) == 17
    for name in ['initial', 'current_native', 'controls', 'registered_gates']:
        receipt = receipts[name]
        assert receipt['input_hashes_before'] == receipt['input_hashes_after']
        assert all(digest(Path(p)) == sha for p, sha in receipt['input_hashes_after'].items())
    for name in ['initial', 'current_native']:
        observations = receipts[name]['contracts']
        assert [c['name'] for c in observations] == [c['name'] for c in contracts]
        for c, result in zip(contracts, observations):
            validation = result['validation']
            assert validation['observations'] == c['count'] and validation['original_source_files'] == 67
            assert validation['registry_packages'] == 119
            proof = validation['composite_assembly']
            assert proof['asset_versions'] == c['asset_versions']
            assert proof['assembly_inputs'] == sorted(c['render_inputs'])
            assert proof['compile_inputs'] == sorted(c['compile_inputs'])
            assert proof['renderer'] == c['renderer'] and proof['suite'] == c['suite']
            if name == 'initial':
                assert result['current_observations'] is None and result['current_binary_sha256'] is None
            else:
                assert same(result['current_observations'], [v['expected'] for v in read(ROOT / c['corpus'])['cases']])
                assert result['current_binary_sha256'] == digest(ROOT / 'build_native_current' / c['native_target'])
    controls = receipts['controls']
    assert controls['positives'] == [c['name'] for c in contracts]
    assert len(controls['controls']) == 24 and all(c['rejected'] for c in controls['controls'])
    gates = receipts['registered_gates']
    assert gates['registered_tests'] == 885
    assert [len(s['tests']) for s in gates['steps']] == [26, 3]
    assert all(s['exit_code'] == 0 for s in gates['steps'])
    assert set(gates['steps'][0]['tests']) == set(gates['expected_tests'])
    dispatcher = receipts['dispatcher']
    assert dispatcher['inputs_unchanged'] and dispatcher['accepted_mutations'] == 0
    assert len(dispatcher['cases']) == 1 and dispatcher['cases'][0]['invariant'] is True
    assert dispatcher['cases'][0]['test'] == 'parity_composite_corpus_source_controls'
    previous = read(ROOT / 'docs/parity/resolved_corpus_source_milestone_5_0_6.json.gz')
    archive_paths = [ROOT / 'docs/parity/corpus_source_before_composite_5_0_6.json.gz',
                     ROOT / 'docs/parity/corpus_document_before_composite_5_0_6.json.gz']
    for name, source in read(archive_paths[0])['sources'].items():
        assert hashlib.sha256(source['text'].encode()).hexdigest() == source['sha256']
        assert source['sha256'] == previous['input_hashes'][str(ROOT / name)]
    old_document = read(archive_paths[1])
    assert hashlib.sha256(old_document['text'].encode()).hexdigest() == old_document['sha256']
    assert old_document['sha256'] == previous['input_hashes'][str(ROOT / old_document['path'])]
    inventory = receipts['inventory']['inventory']
    assert inventory['registered_tests'] == 885
    assert inventory['counts'] == {'active_baseline': 305, 'custom_dispatcher_requires_review': 173}
    baseline_only = set()
    for row in inventory['tests']:
        if row['status'] == 'active_baseline':
            corpus = ROOT / row['corpus']
            assert digest(corpus) == row['sha256']
            if set(read(corpus)) == {'baseline', 'cases'}:
                baseline_only.add(row['corpus'])
    assert len(baseline_only) == 27
    all_contracts = [c for r in registries for c in r['contracts']]
    bound = {c['corpus'] for c in all_contracts}
    assert len(bound) == len(all_contracts) == 20 and bound <= baseline_only
    assert sum(c['count'] for c in all_contracts) == 52639
    ledger_path = ROOT / 'docs/cpp_generator_parity_ledger.json'
    ledger = read(ledger_path)
    before = read(report('ledger_before'))
    assert Counter(c['status'] for c in ledger['capabilities']) == {'qualified': 14, 'partial': 3}
    for old, current in zip(before['capabilities'], ledger['capabilities']):
        assert {k: v for k, v in old.items() if k != 'evidence'} == {
            k: v for k, v in current.items() if k != 'evidence'}
        assert current['evidence'][:len(old['evidence'])] == old['evidence']
    cleanup = []
    for name, receipt in receipts.items():
        if 'owned_directory' not in receipt:
            continue
        owned = Path(receipt['owned_directory'])
        assert owned.parent == Path('/tmp') and owned.name.startswith((
            'teaql_corpus_source_', 'teaql_composite_corpus_', 'teaql_dispatcher_result_controls_506_'))
        assert receipt['owned_directory_removed'] is True and not owned.exists()
        cleanup.append(dict(receipt=name, path=str(owned), removed=True, measured_bytes=receipt.get(
            'owned_bytes_removed', receipt.get('owned_bytes_before_cleanup'))))
    report('cleanup').write_bytes(gzip.compress(json.dumps(dict(
        complete=True, paths=cleanup, measured_bytes_removed=sum(c['measured_bytes'] for c in cleanup),
        retained='Stable preset outputs, original source fixtures and compact execution evidence')).encode(), mtime=0))
    inputs = [Path(__file__), ROOT / 'scripts/corpus_source_assets_5_0_6.py',
              ROOT / 'scripts/verify_corpus_source_contracts_5_0_6.py',
              ROOT / 'scripts/verify_dispatcher_result_controls_5_0_6.py', ROOT / 'tests/CMakeLists.txt',
              ROOT / 'tests/parity/verify_composite_corpus_source_controls_5_0_6.py',
              ROOT / 'docs/composite_corpus_source_provenance_5_0_6.md',
              ROOT / 'docs/resolved_corpus_source_provenance_5_0_6.md', ledger_path,
              *registry_paths, *archive_paths, *[report(n) for n in names], report('cleanup'), report('ledger_before')]
    args.record.write_bytes(gzip.compress(json.dumps(dict(
        complete=True, whole_goal_complete=False, whole_provenance_complete=False, scope=__doc__,
        runtime_commit=registries[-1]['runtime_commit'], generator_commit=registries[-1]['generator_commit'],
        new_bindings=9, new_observations=16373, cumulative_bindings=20, cumulative_observations=52639,
        archived_asset_references=17, effective_corruptions_rejected=24, native_checks=26,
        focused_fixture_checks=3, baseline_only_remaining=sorted(baseline_only - bound),
        custom_registrations_open=173, input_hashes={str(p): digest(p) for p in inputs})).encode(), mtime=0))
    print('Nine composite-source bindings / 16373 current native outputs reconcile; seven baseline-only corpora remain open')


if __name__ == '__main__':
    main()
