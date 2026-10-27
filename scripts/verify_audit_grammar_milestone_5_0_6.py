#!/usr/bin/env python3
"""Reconcile the bounded builtin repair with current sources and retained evidence."""
import argparse
from collections import Counter
import gzip
import hashlib
import json
from pathlib import Path
import re
import sys

from qualify_audit_builtin_5_0_6 import INSERTION, SOURCE

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tests/parity'))
from compare_values import same


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(name):
    path = ROOT / 'docs/parity' / (name + '_5_0_6.json.gz')
    return json.loads(gzip.decompress(path.read_bytes()))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    assert not args.record.exists(), 'Preserve earlier receipts'
    names = ['audit_grammar_branch_initial', 'audit_grammar_builtin_proposal',
             'audit_grammar_builtin_candidate', 'audit_builtin_callers_proposal',
             'audit_builtin_callers_candidate', 'audit_builtin_previous_junit_control',
             'audit_builtin_previous_junit', 'audit_builtin_installed_source',
             'audit_grammar_branch_installed', 'audit_builtin_sdk_refresh',
             'audit_builtin_sdk_refresh_audit', 'audit_builtin_native_gates',
             'audit_builtin_metadata_type_controls_replay',
             'audit_grammar_branch_final_guards']
    evidence = {name: read(name) for name in names}
    expected_failures = {'audit_grammar_branch_initial', 'audit_builtin_previous_junit'}
    for name, value in evidence.items():
        assert value['complete'] is (name not in expected_failures), name
    initial = evidence['audit_grammar_branch_initial']
    installed = evidence['audit_grammar_branch_installed']
    source = evidence['audit_builtin_installed_source']
    candidate = evidence['audit_grammar_builtin_candidate']
    assert len(initial['differences']) == 45
    assert all('builtin' in case['input'] for case in initial['differences'])
    assert len(installed['cases']) == len(candidate['cases']) == len(source['cases']) == 8594
    assert source['junit_tests'] == 120 and source['junit_passed'] is True
    assert sum(case['original']['parsed'] is True for case in source['cases']) == 8492
    for before, after in zip(initial['cases'], installed['cases']):
        assert before['name'] == after['name'] and before['input'] == after['input']
        assert same(before['original'], after['original'])
    fixture = json.loads((ROOT / 'tests/parity/audit_grammar_branch_inputs_5_0_6.json').read_text())
    assert len(fixture['branches']) == 92
    assert Counter(b['classification'] for b in fixture['branches']) == {
        'text_source': 90, 'non_text_group': 2}
    witnessed = {c['name']: c['source_witness'] for c in installed['cases'] if c['source_witness']}
    assert len(witnessed) == 270
    for case in fixture['cases']:
        if case['witness']:
            assert witnessed[case['name']]['variant'] == case['witness']['variant']
    for name in ('audit_builtin_installed_source', 'audit_grammar_branch_installed'):
        value = evidence[name]
        assert value['input_hashes_before'] == value['input_hashes_after']
        for path, expected in value['input_hashes_before'].items():
            assert digest(Path(path)) == expected, path
    proposal = evidence['audit_grammar_builtin_proposal']
    assert SOURCE.read_text() == proposal['candidate_source']
    assert digest(SOURCE) == proposal['candidate_sha256']
    assert SOURCE.read_text().count(INSERTION) == 1
    previous = SOURCE.read_text().replace(INSERTION, '', 1)
    assert hashlib.sha256(previous.encode()).hexdigest() == proposal['original_sha256']
    callers = evidence['audit_builtin_callers_candidate']
    assert proposal['candidate_sha256'] in callers['input_hashes_before'].values()
    assert len(callers['cases']) == 9 and all(c['match'] for c in callers['cases'])
    assert sum(len(c['original']) for c in callers['cases']) == 27
    assert sum(len(r['policies']) for c in callers['cases'] for r in c['original']) == 126
    old_control = evidence['audit_builtin_previous_junit_control']
    assert old_control['junit_tests'] == 120 and old_control['junit_failures'] == 1
    controls = evidence['audit_builtin_metadata_type_controls_replay']['controls']
    assert len(controls) == 8 and all(c['current_rejected'] for c in controls)
    assert sum(c['previous_accepted'] for c in controls) == 5

    refresh = evidence['audit_builtin_sdk_refresh']
    assert len(refresh['sdks']) == 80
    assert sum(len(s['after']) for s in refresh['sdks']) == 3560
    for sdk in refresh['sdks']:
        assert sdk['before'] == sdk['after'] and not sdk['changes']
        for relative, expected in sdk['after'].items():
            assert digest(ROOT / sdk['path'] / relative) == expected
    assert len(refresh['protected_artifacts']) == 2467
    for relative, expected in refresh['protected_artifacts'].items():
        assert digest(ROOT / relative) == expected
    manifests = read('audit_builtin_sdk_provenance_before')['manifests']
    key = 'generator/src/main/java/com/skynet/codegenerator/cpp/CppRustLiteralMetadata.java'
    for manifest in manifests:
        before = json.loads(manifest['text'])
        after = json.loads((ROOT / manifest['path']).read_text())
        assert after['files'] == before['files']
        changed = {k for k, v in before['generator_implementation_sha256'].items()
                   if after['generator_implementation_sha256'][k] != v}
        assert changed == {key}
        assert after['generator_implementation_sha256'][key] == digest(SOURCE)
        after['generator_implementation_sha256'] = before['generator_implementation_sha256']
        assert json.dumps(after, indent=2) + '\n' == manifest['text']
    gates = evidence['audit_builtin_native_gates']
    assert gates['profiles'] == [{'preset': 'native', 'test_count': 2},
                                 {'preset': 'modules_native', 'test_count': 2}]
    assert all(step['exit_code'] == 0 for step in gates['steps'])

    cleanup = []
    for name, value in evidence.items():
        if 'owned_directory' in value:
            directory = Path(value['owned_directory'])
            assert directory.parent == Path('/tmp') and directory.name.startswith('teaql_audit_')
            assert value['owned_directory_removed'] is True and not directory.exists()
            cleanup.append(dict(receipt=name, path=str(directory), removed=True,
                                measured_bytes=value['owned_bytes_removed']))
    sdk_directories = {Path(re.match(r'/tmp/teaql_forward_relation_arguments_[A-Za-z0-9_]+', str(arg))[0])
                       for step in refresh['steps'] for arg in step['command']
                       if str(arg).startswith('/tmp/teaql_forward_relation_arguments_')}
    assert len(sdk_directories) == 1
    for directory in sdk_directories:
        assert directory.parent == Path('/tmp') and not directory.exists()
        cleanup.append(dict(receipt='audit_builtin_sdk_refresh', path=str(directory),
                            removed=True, measured_bytes=None))
    cleanup_path = ROOT / 'docs/parity/audit_grammar_branch_cleanup_5_0_6.json.gz'
    assert not cleanup_path.exists()
    cleanup_path.write_bytes(gzip.compress(json.dumps(dict(
        complete=True, scope='Exact task-owned disposable paths; no shared-cache cleanup',
        paths=cleanup, measured_bytes_removed=sum(c['measured_bytes'] or 0 for c in cleanup),
        retained='Stable native/modules preset outputs, producer target, source and compact evidence'
    )).encode(), mtime=0))
    ledger = json.loads((ROOT / 'docs/cpp_generator_parity_ledger.json').read_text())
    capabilities = Counter(c['status'] for c in ledger['capabilities'])
    dispositions = json.loads((ROOT / 'docs/parity/v5_0_6/source_dispositions.json').read_text())
    sources = Counter(s['status'] for s in dispositions['sources'])
    assert capabilities == {'qualified': 14, 'partial': 3}
    assert sources == {'accepted': 122, 'excluded_external': 2}
    files = [Path(__file__), SOURCE, ROOT / 'docs/audit_grammar_branch_review_5_0_6.md',
             ROOT / 'docs/cpp_generator_parity_ledger.json',
             ROOT / 'tests/parity/verify_audit_metadata_types_5_0_6.py', cleanup_path]
    files += [ROOT / 'docs/parity' / (name + '_5_0_6.json.gz') for name in names]
    record = dict(complete=True, whole_goal_complete=False, scope=__doc__,
                  explicit_reuse='Installed parser bytes equal the exact candidate used by nine fresh model pairs',
                  capabilities=capabilities, source_status=sources,
                  summary=dict(source_results=8594, java_tests=120, ast_witnesses=270,
                               caller_pairs=9, sdk_destinations=80, generated_files=3560,
                               native=2, modules_native=2, metadata_controls=8),
                  input_hashes={str(p): digest(p) for p in files})
    args.record.write_bytes(gzip.compress(json.dumps(record).encode(), mtime=0))
    print('Qualified bounded builtin milestone; capabilities 14/3, sources 122/0/2; whole goal remains open')


if __name__ == '__main__':
    main()
