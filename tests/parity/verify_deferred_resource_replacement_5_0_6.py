#!/usr/bin/env python3
"""Verify C++ suspended-resource adaptations against retained business results."""
import argparse
import gzip
import itertools
import json
import os
from pathlib import Path
import subprocess
from compare_values import same

ROOT = Path(__file__).resolve().parents[2]


def cases():
    combinations = list(itertools.product(['list', 'one', 'page'],
                                         ['plain', 'id_set', 'continuous'], ['rows', 'facet']))
    combinations += [('page', name, 'count') for name in ['plain', 'continuous']]
    result = []
    for mode, optimization, gate in combinations:
        profiles = [('executor', action) for action in ['resume', 'cancel', 'close']]
        profiles += [('registry', 'resume'), ('provider_dead', 'resume')]
        if gate != 'facet':
            profiles += [('registry_dead', 'resume'), ('policy', 'resume')]
        if optimization == 'id_set':
            profiles += [('store', action) for action in ['resume', 'cancel', 'close']]
            if gate == 'rows':
                profiles += [('store_dead', 'resume')]
        for profile, action in profiles:
            result.append(dict(mode=mode, optimization=optimization, gate=gate,
                               profile=profile, action=action, offset=0, limit=1, include_all=True))
    assert len(result) == 143
    return result


def required_results(case):
    if case['action'] == 'close':
        return set()
    fields = {'retry'}
    if case['action'] == 'resume' and case['profile'] in ('executor', 'registry', 'store'):
        fields.add('result')
    return fields


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', required=True)
    parser.add_argument('--record', type=Path)
    args = parser.parse_args()
    inputs = cases()
    completed = subprocess.run([args.cpp], input=''.join(json.dumps(x) + '\n' for x in inputs),
                               text=True, capture_output=True,
                               env=dict(os.environ, TEAQL_LOG_ENDPOINT='off'))
    if completed.returncode:
        archive = ROOT / 'docs/parity/deferred_resource_replacement_initial_failure_5_0_6.json.gz'
        if not archive.exists():
            archive.write_bytes(gzip.compress(json.dumps(dict(inputs=inputs,
                stdout=completed.stdout, stderr=completed.stderr,
                exit_code=completed.returncode)).encode(), mtime=0))
        index = len(completed.stdout.rstrip().split('\n')) if completed.stdout else 0
        print('Failed after', index, 'cases; next input:', inputs[index] if index < len(inputs) else None)
        print(completed.stderr)
        return 1
    outputs = [json.loads(line) for line in completed.stdout.split('\n') if line]
    assert len(outputs) == len(inputs)
    corpus = ROOT / 'tests/parity/deferred_facet_page_5_0_6_cases.json.gz'
    original = json.loads(gzip.decompress(corpus.read_bytes()))
    if original['baseline'] != json.loads((ROOT / 'upstream.lock.json').read_text())['commit']:
        raise RuntimeError('Deferred resource comparison corpus belongs to another baseline')
    baseline = {(row['input']['mode'], row['input']['optimization']): row['expected']['result']
                for row in original['cases'] if row['input']['gate'] == 'none'
                and row['input']['action'] == 'resume' and row['input']['offset'] == 0
                and row['input']['limit'] == 1 and row['input']['include_all']}
    positive = 0
    for case, actual in zip(inputs, outputs):
        assert actual['pending'] is True and actual['closed'] is True, (case, actual)
        fields = required_results(case)
        if {key for key in ('result', 'retry') if key in actual} != fields:
            raise RuntimeError(f'Required result/retry payloads differ for {case}')
        for field in fields:
            assert same(baseline[(case['mode'], case['optimization'])], actual[field]), (case, field)
            positive += 1
        if case['action'] == 'resume':
            assert actual['expired_rejected'] is case['profile'].endswith('_dead'), case
            if case['profile'] == 'policy':
                assert actual['policy_denied'] is True, case
        elif case['action'] == 'cancel':
            assert actual['cancelled'] is True, case
    assert positive == sum(len(required_results(case)) for case in inputs) == 163
    if args.record:
        assert not args.record.exists()
        args.record.write_bytes(gzip.compress(json.dumps(dict(inputs=inputs, cpp=outputs,
            positive_business_result_comparisons=positive,
            boundary='C++ resource mutation/destruction adaptation; original Rust mutable setters '
                     'cannot run while its future borrows Context. Only unchanged-data business '
                     'results are compared with the original corpus, not invented Rust mutation actions.'),
            ensure_ascii=False).encode(), mtime=0))
    print(f'Verified {len(inputs)} actual suspended resource scenarios; '
          f'{positive} unchanged-data results match original Rust; no query algorithm replaced')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
