#!/usr/bin/env python3
"""Reconcile installed type child corrections, strict comparisons and current artifacts."""
import argparse
from collections import Counter
import gzip
import hashlib
import json
from pathlib import Path

from qualify_audit_type_bounds_5_0_6 import SOURCE, candidate_text
from verify_audit_metadata_source_5_0_6 import same

ROOT = Path(__file__).resolve().parents[1]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def path(name):
    return ROOT / 'docs/parity' / (name + '_5_0_6.json.gz')


def read(name):
    return json.loads(gzip.decompress(path(name).read_bytes()))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    assert not args.record.exists()
    names = ['audit_type_children_initial', 'audit_type_bounds_initial',
             'audit_type_bounds_candidate', 'audit_type_bounds_proposal',
             'audit_type_child_rules_initial', 'audit_type_child_rules_candidate',
             'audit_type_child_rules_proposal', 'audit_type_contract_initial',
             'audit_type_contract_candidate', 'audit_type_contract_proposal',
             'audit_type_child_previous_junit', 'audit_type_child_previous_control',
             'audit_type_child_callers_candidate', 'audit_type_child_callers_proposal',
             'audit_type_child_installed_source', 'audit_type_child_installed_grammar',
             'audit_type_child_sdk_refresh', 'audit_type_child_final_gates',
             'audit_type_child_compiler_before', 'audit_type_child_compiler_controls']
    evidence = {name: read(name) for name in names}
    failed = {'audit_type_children_initial', 'audit_type_bounds_initial',
              'audit_type_bounds_candidate', 'audit_type_bounds_proposal',
              'audit_type_child_rules_initial', 'audit_type_contract_initial',
              'audit_type_child_previous_junit'}
    for name, value in evidence.items():
        assert value['complete'] is (name not in failed), name
    initial = evidence['audit_type_contract_initial']
    installed = evidence['audit_type_child_installed_source']
    grammar = evidence['audit_type_child_installed_grammar']
    assert len(initial['cases']) == len(installed['cases']) == len(grammar['cases']) == 9494
    assert len(initial['differences']) == 156 and not installed['differences']
    assert installed['junit_passed'] and installed['junit_tests'] == 121
    assert sum(c['original']['parsed'] is True for c in installed['cases']) == 9392
    assert sum(c['source_witness'] is not None for c in grammar['cases']) == 270
    for before, after in zip(initial['cases'], installed['cases']):
        assert before['name'] == after['name'] and before['input'] == after['input']
        assert same(before['original'], after['original'])
    for name in ('audit_type_child_installed_source', 'audit_type_child_installed_grammar'):
        value = evidence[name]
        assert value['input_hashes_before'] == value['input_hashes_after']
        for filename, sha in value['input_hashes_before'].items():
            assert digest(Path(filename)) == sha, filename
    proposal = evidence['audit_type_contract_proposal']
    assert digest(SOURCE) == proposal['candidate_sha256']
    assert SOURCE.read_text() == candidate_text(proposal['original_source'])
    callers = evidence['audit_type_child_callers_candidate']
    assert len(callers['cases']) == 12 and all(c['match'] for c in callers['cases'])
    assert digest(SOURCE) in callers['input_hashes_before'].values()
    before = evidence['audit_type_child_compiler_before']
    verifier = before['source'].replace(
        'from verify_audit_metadata_source_5_0_6 import identities\n',
        'from verify_audit_metadata_source_5_0_6 import identities\nfrom compare_values import same\n').replace(
        'case["match"] = case["original"] == actual', 'case["match"] = same(case["original"], actual)')
    assert verifier == (ROOT / 'scripts/verify_audit_metadata_compiler_5_0_6.py').read_text()
    controls = evidence['audit_type_child_compiler_controls']
    assert len(controls['controls']) == 14 and controls['previous_false_acceptances'] == 12
    assert evidence['audit_type_child_previous_control']['junit_failures'] == 1
    refresh = evidence['audit_type_child_sdk_refresh']
    assert len(refresh['sdks']) == 80 and len(refresh['protected_artifacts']) == 2467
    assert sum(len(s['after']) for s in refresh['sdks']) == 3560
    for sdk in refresh['sdks']:
        assert sdk['before'] == sdk['after'] and not sdk['changes']
        for filename, sha in sdk['after'].items():
            assert digest(ROOT / sdk['path'] / filename) == sha
    for filename, sha in refresh['protected_artifacts'].items():
        assert digest(ROOT / filename) == sha
    manifests = read('audit_type_child_sdk_provenance_before')['manifests']
    key = 'generator/src/main/java/com/skynet/codegenerator/cpp/CppRustLiteralMetadata.java'
    assert len(manifests) == 80
    for manifest in manifests:
        old = json.loads(manifest['text'])
        current = json.loads((ROOT / manifest['path']).read_text())
        changed = {k for k, v in old['generator_implementation_sha256'].items()
                   if current['generator_implementation_sha256'][k] != v}
        assert changed == {key}
        assert current['generator_implementation_sha256'][key] == digest(SOURCE)
        current['generator_implementation_sha256'] = old['generator_implementation_sha256']
        assert json.dumps(current, indent=2) + '\n' == manifest['text']
    gates = evidence['audit_type_child_final_gates']
    assert gates['input_hashes_before'] == gates['input_hashes_after']
    assert gates['profiles'] == [{'preset': 'native', 'test_count': 2},
                                 {'preset': 'modules_native', 'test_count': 2}]
    for filename, sha in gates['input_hashes_after'].items():
        assert digest(Path(filename)) == sha
    cleanup = []
    for name, value in evidence.items():
        if 'owned_directory' not in value:
            continue
        owned = Path(value['owned_directory'])
        assert owned.parent == Path('/tmp')
        assert owned.name.startswith(('teaql_audit_', 'teaql_forward_relation_arguments_'))
        assert value['owned_directory_removed'] is True and not owned.exists()
        cleanup.append(dict(receipt=name, path=str(owned), removed=True,
                            measured_bytes=value.get('owned_bytes_removed')))
    cleanup_path = path('audit_type_child_cleanup')
    assert not cleanup_path.exists()
    cleanup_path.write_bytes(gzip.compress(json.dumps(dict(
        complete=True, scope='Only exact disposable paths from this type-child cohort', paths=cleanup,
        measured_bytes_removed=sum(c['measured_bytes'] or 0 for c in cleanup),
        stable_outputs_before=gates['stable_output_bytes_before'],
        stable_outputs_after=gates['stable_output_bytes_after'])).encode(), mtime=0))
    ledger = json.loads((ROOT / 'docs/cpp_generator_parity_ledger.json').read_text())
    capabilities = Counter(c['status'] for c in ledger['capabilities'])
    dispositions = json.loads((ROOT / 'docs/parity/v5_0_6/source_dispositions.json').read_text())
    sources = Counter(s['status'] for s in dispositions['sources'])
    assert capabilities == {'qualified': 14, 'partial': 3}
    assert sources == {'accepted': 122, 'excluded_external': 2}
    inputs = [Path(__file__), SOURCE, cleanup_path, ROOT / 'docs/audit_type_child_rules_5_0_6.md',
              ROOT / 'docs/cpp_generator_parity_ledger.json'] + [path(n) for n in names]
    args.record.write_bytes(gzip.compress(json.dumps(dict(
        complete=True, whole_goal_complete=False, scope=__doc__,
        capabilities=capabilities, source_status=sources,
        summary=dict(source_results=9494, repaired_differences=156, java_tests=121,
                     generated_model_pairs=12, strict_comparator_controls=14,
                     sdk_destinations=80, generated_files=3560, native=2, modules_native=2),
        explicit_reuse='Actual twelve-pair execution uses the exact installed candidate; strict comparison is a separately verified replay of those original outputs',
        input_hashes={str(p): digest(p) for p in inputs})).encode(), mtime=0))
    print('Type child milestone qualifies; capabilities14/3 and source122/0/2; whole goal remains open')


if __name__ == '__main__':
    main()
