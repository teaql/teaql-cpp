#!/usr/bin/env python3
"""Join all 27 baseline-only source bindings without promoting whole semantic qualification."""
import argparse
from collections import Counter
import gzip
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path):
    data = path.read_bytes()
    return json.loads(gzip.decompress(data) if path.suffix == '.gz' else data)


def report(name):
    return ROOT / 'docs/parity' / ('remaining_corpus_source_' + name + '_5_0_6.json.gz')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    assert not args.record.exists() and not report('cleanup').exists()
    names = ['execution', 'bindings', 'controls', 'registered_gates', 'dispatcher', 'inventory', 'guards']
    receipts = {n: read(report(n)) for n in names}
    assert all(r['complete'] is True for r in receipts.values())
    execution = receipts['execution']
    assert execution['input_hashes_before'] == execution['input_hashes_after']
    assert len(execution['input_hashes_after']) == 4372 and len(execution['reference_source_files']) == 466
    assert execution['owned_bytes_removed'] == 399189614
    snapshot_path = ROOT / 'docs/parity/corpus_source_before_remaining_5_0_6.json.gz'
    snapshot = read(snapshot_path)
    old_cmake = snapshot['sources']['tests/CMakeLists.txt']
    assert old_cmake['sha256'] == hashlib.sha256(old_cmake['text'].encode()).hexdigest()
    assert execution['input_hashes_after'][str(ROOT / 'tests/CMakeLists.txt')] == old_cmake['sha256']
    for path, sha in execution['input_hashes_after'].items():
        if path != str(ROOT / 'tests/CMakeLists.txt'):
            assert digest(Path(path)) == sha, path
    assert all(s['exit_code'] == 0 for s in execution['steps'])
    assert [c['count'] for c in execution['contracts']] == [822, 2200, 18, 32, 1373, 972, 864]
    for result in execution['contracts']:
        c = next(c for c in execution['registry']['contracts'] if c['name'] == result['name'])
        assert result['native_binary_sha256'] == digest(ROOT / 'build_native_current' / c['native_target'])
    bindings = receipts['bindings']
    assert bindings['execution_sha256'] == digest(report('execution'))
    assert sum(c['count'] for c in bindings['contracts']) == 6281
    assert bindings['source_validation']['original_reference_files'] == 466
    assert bindings['source_validation']['runtime_files'] == 67
    assert bindings['source_validation']['projects'] == 3
    controls = receipts['controls']
    assert controls['input_hashes_before'] == controls['input_hashes_after']
    assert all(digest(Path(p)) == sha for p, sha in controls['input_hashes_after'].items())
    assert len(controls['controls']) == 24 and all(c['rejected'] for c in controls['controls'])
    gates = receipts['registered_gates']
    assert gates['registered_tests'] == 887 and gates['input_hashes_before'] == gates['input_hashes_after']
    assert all(digest(Path(p)) == sha for p, sha in gates['input_hashes_after'].items())
    assert [len(s['tests']) for s in gates['steps']] == [35, 4]
    assert set(gates['steps'][0]['tests']) == set(gates['expected_tests'])
    dispatcher = receipts['dispatcher']
    assert dispatcher['inputs_unchanged'] is True and dispatcher['accepted_mutations'] == 0
    assert len(dispatcher['cases']) == 1 and dispatcher['cases'][0]['invariant'] is True
    assert dispatcher['cases'][0]['test'] == 'parity_remaining_corpus_source_controls'
    inventory = receipts['inventory']['inventory']
    assert inventory['registered_tests'] == 887
    assert inventory['counts'] == {'active_baseline': 305, 'custom_dispatcher_requires_review': 174}
    registry_paths = [ROOT / 'tests/parity' / (n + '_5_0_6.json') for n in (
        'corpus_source_contracts', 'resolved_corpus_source_contracts',
        'composite_corpus_source_contracts', 'remaining_corpus_source_contracts')]
    contracts = [c for p in registry_paths for c in read(p)['contracts']]
    assert len(contracts) == 27 and sum(c['count'] for c in contracts) == 58920
    baseline_only = set()
    for row in inventory['tests']:
        if row['status'] != 'active_baseline':
            continue
        path = ROOT / row['corpus']
        assert row['sha256'] == digest(path)
        if set(read(path)) == {'baseline', 'cases'}:
            baseline_only.add(row['corpus'])
    assert len(baseline_only) == 27 and baseline_only == {c['corpus'] for c in contracts}
    ledger_path = ROOT / 'docs/cpp_generator_parity_ledger.json'
    ledger = read(ledger_path)
    before = read(report('ledger_before'))
    assert Counter(c['status'] for c in ledger['capabilities']) == {'qualified': 14, 'partial': 3}
    for old, current in zip(before['capabilities'], ledger['capabilities']):
        assert {k: v for k, v in old.items() if k != 'evidence'} == {k: v for k, v in current.items() if k != 'evidence'}
        assert current['evidence'][:len(old['evidence'])] == old['evidence']
    cleanup = []
    for name, receipt in receipts.items():
        if 'owned_directory' not in receipt:
            continue
        owned = Path(receipt['owned_directory'])
        assert owned.parent == Path('/tmp') and owned.name.startswith((
            'teaql_remaining_corpus_', 'teaql_all_corpus_source_gates_506_', 'teaql_dispatcher_result_controls_506_'))
        assert receipt['owned_directory_removed'] is True and not owned.exists()
        cleanup.append(dict(receipt=name, path=str(owned), removed=True, measured_bytes=receipt.get(
            'owned_bytes_removed', receipt.get('owned_bytes_before_cleanup'))))
    report('cleanup').write_bytes(gzip.compress(json.dumps(dict(complete=True, paths=cleanup,
        measured_bytes_removed=sum(p['measured_bytes'] for p in cleanup),
        retained='Stable presets, original fixtures and complete compressed source/native execution evidence')).encode(), mtime=0))
    inputs = [Path(__file__), ROOT / 'scripts/replay_remaining_corpus_sources_5_0_6.py',
              ROOT / 'scripts/verify_remaining_corpus_source_contracts_5_0_6.py',
              ROOT / 'scripts/corpus_input_contracts_5_0_6.py', ROOT / 'scripts/run_corpus_source_gates_5_0_6.py',
              ROOT / 'tests/parity/verify_remaining_corpus_source_controls_5_0_6.py',
              ROOT / 'scripts/verify_dispatcher_result_controls_5_0_6.py', ROOT / 'tests/CMakeLists.txt',
              ROOT / 'docs/baseline_corpus_source_bindings_5_0_6.md',
              ROOT / 'docs/semantic_qualification_requirements_5_0_6.md', ledger_path,
              snapshot_path, *registry_paths, *[report(n) for n in names], report('cleanup'), report('ledger_before')]
    args.record.write_bytes(gzip.compress(json.dumps(dict(complete=True, whole_goal_complete=False,
        whole_provenance_complete=False, scope=__doc__, new_bindings=7, new_observations=6281,
        cumulative_bindings=27, cumulative_observations=58920, baseline_only_remaining=[],
        embedded_direct_corpora_open=278, custom_registrations_open=174, effective_corruptions_rejected=24,
        native_checks=35, focused_fixture_checks=4, input_hashes={str(p): digest(p) for p in inputs})).encode(), mtime=0))
    print('All 27 baseline-only source bindings reconcile; whole direct/custom provenance and P0-P7 remain open')


if __name__ == '__main__':
    main()
