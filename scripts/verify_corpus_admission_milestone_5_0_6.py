#!/usr/bin/env python3
"""Bind actual corpus admission repairs and current locator evidence without parity promotion."""
import argparse
from collections import Counter
import gzip
import hashlib
import json
from pathlib import Path

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
    names = ['corpus_admission_initial', 'corpus_admission_installed', 'corpus_inventory_before',
             'corpus_inventory_controls', 'corpus_inventory_current',
             'corpus_admission_dispatcher_controls', 'corpus_admission_final_guards']
    evidence = {n: read(n) for n in names}
    initial = evidence['corpus_admission_initial']
    installed = evidence['corpus_admission_installed']
    assert initial['complete'] is False and installed['complete'] is True
    assert {c['name'] for c in initial['cases'] if c['accepted'] and not c['valid']} == {
        'empty_list', 'empty_object', 'empty_string'}
    assert len(initial['cases']) == len(installed['cases']) == 11
    before = initial['comparator_source']
    after = before.replace('    cases = corpus["cases"]\n',
                           '    cases = corpus["cases"]\n'
                           '    if type(cases) is not list or not cases:\n'
                           '        raise RuntimeError(f"{label} corpus cases must be a nonempty list")\n')
    comparator = ROOT / 'tests/parity/compare_values.py'
    assert comparator.read_text() == installed['comparator_source'] == after
    for name in ['corpus_admission_installed', 'corpus_inventory_controls', 'corpus_admission_dispatcher_controls']:
        value = evidence[name]
        assert value['complete'] is True
        assert value['input_hashes_before'] == value['input_hashes_after']
    controls = evidence['corpus_inventory_controls']
    assert len(controls['binding_controls']) == 8 and len(controls['shape_controls']) == 4
    assert sum(c['actual'] != c['previous'] for c in controls['binding_controls']) == 3
    for case in controls['shape_controls']:
        assert case['previous']['exit_code'] == 0
        assert case['current']['exit_code'] == (0 if case['name'] == 'valid' else 1)
    inventory = evidence['corpus_inventory_current']['inventory']
    assert inventory['registered_tests'] == 879
    assert inventory['counts'] == {'active_baseline': 305, 'custom_dispatcher_requires_review': 170}
    corpus_hashes, comparator_hashes = {}, {}
    for row in inventory['tests']:
        path = ROOT / row['comparator']
        comparator_hashes[row['comparator']] = digest(path)
        if row['status'] == 'active_baseline':
            assert row['valid_cases'] is True and row['observations'] > 0
            path = ROOT / row['corpus']
            assert digest(path) == row['sha256']
            corpus_hashes[row['corpus']] = row['sha256']
    dispatcher = evidence['corpus_admission_dispatcher_controls']
    changed = [t for c in dispatcher['cases'] for t in c['trials'] if t['changed']]
    assert len(changed) == 5 and all(t['exit_code'] != 0 for t in changed)
    assert not dispatcher['accepted_mutations']
    for filename in ['corpus_admission_native_gate', 'corpus_admission_module_gate']:
        gate = json.loads((ROOT / 'docs/parity' / (filename + '_5_0_6.json')).read_text())
        assert gate['complete'] is True and gate['terminal']['exit_code'] == 0
    assert evidence['corpus_admission_final_guards']['complete'] is True
    cleanup = []
    for name, value in evidence.items():
        if 'owned_directory' not in value:
            continue
        owned = Path(value['owned_directory'])
        assert owned.parent == Path('/tmp') and owned.name.startswith((
            'teaql_corpus_', 'teaql_dispatcher_result_controls_506_'))
        assert value['owned_directory_removed'] is True and not owned.exists()
        cleanup.append(dict(receipt=name, path=str(owned), removed=True,
                            measured_bytes=value.get('owned_bytes_removed', value.get('owned_bytes_before_cleanup'))))
    cleanup_path = report_path('corpus_admission_cleanup')
    assert not cleanup_path.exists()
    cleanup_path.write_bytes(gzip.compress(json.dumps(dict(
        complete=True, scope='Exact owned control directories; production corpora and stable build outputs retained',
        paths=cleanup, measured_bytes_removed=sum(c['measured_bytes'] or 0 for c in cleanup))).encode(), mtime=0))
    ledger = json.loads((ROOT / 'docs/cpp_generator_parity_ledger.json').read_text())
    assert Counter(c['status'] for c in ledger['capabilities']) == {'qualified': 14, 'partial': 3}
    inputs = [Path(__file__), comparator, ROOT / 'scripts/inventory_parity_baselines.py',
              ROOT / 'tests/parity/verify_corpus_admission_5_0_6.py',
              ROOT / 'tests/parity/verify_corpus_inventory_5_0_6.py',
              ROOT / 'docs/corpus_admission_and_inventory_5_0_6.md',
              ROOT / 'docs/cpp_generator_parity_ledger.json', cleanup_path,
              *[report_path(n) for n in names]]
    args.record.write_bytes(gzip.compress(json.dumps(dict(
        complete=True, whole_goal_complete=False, scope=__doc__,
        repaired_false_passes=3, admission_controls=11, binding_controls=8,
        inventory_shape_controls=4, actual_result_mutations_rejected=5,
        native_checks=7, imported_checks=1, direct_bindings=305, custom_registrations_open=170,
        comparator_hashes=comparator_hashes, corpus_hashes=corpus_hashes,
        input_hashes={str(p): digest(p) for p in inputs})).encode(), mtime=0))
    print('Corpus admission and locator repair qualifies; complete SQ02/GP13/P0-P7 remain open')


if __name__ == '__main__':
    main()
