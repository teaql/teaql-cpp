#!/usr/bin/env python3
"""Compare actual runtime COUNT/facet diagnostic-source execution."""
import argparse
import gzip
import hashlib
import itertools
import json
import os
from pathlib import Path
import subprocess
from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]
CORPUS = Path(__file__).with_name('derived_query_diagnostics_5_0_6_cases.json.gz')


def inputs():
    for kind, endpoint, logging, failure, invalid, secret in itertools.product(
            ('relation', 'projection', 'order', 'facet'),
            ('all', 'owned', 'list', 'aggregate', 'stream'),
            (False, True), (False, True), (False, True),
            ('PRIVATE-DESCENDANT', 'private-\u03b1-\U0001f642', 'long-' + 'x' * 1024)):
        yield dict(kind=kind, endpoint=endpoint, logging=logging, failure=failure,
                   invalid_intent=invalid, secret=secret)
    for endpoint, logging, failure, invalid, secret in itertools.product(
            ('entities', 'enhanced', 'enhanced_aggregate', 'enhanced_owned'),
            (False, True), (False, True), (False, True),
            ('PRIVATE-DESCENDANT', 'private-\u03b1-\U0001f642', 'long-' + 'x' * 1024)):
        yield dict(kind='facet', endpoint=endpoint, logging=logging, failure=failure,
                   invalid_intent=invalid, secret=secret)


def metadata():
    files = ('teaql-runtime/src/generated_support.rs', 'teaql-runtime/src/data_service/resolved.rs',
             'teaql-provider-sqlite/tests/request_trace_log.rs')
    return dict(baseline=json.loads((ROOT / 'upstream.lock.json').read_text())['commit'],
                source_hashes={name: hashlib.sha256((ROOT / '.reference/teaql-rs' / name).read_bytes()).hexdigest()
                               for name in files}, adaptation='None. Exact physical results, errors and complete retained SQL/intent/parameter/trace diagnostics.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', required=True)
    parser.add_argument('--rust', required=True)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    subprocess.run(['python3', str(ROOT / 'scripts/verify_upstream.py')], check=True)
    os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
    cases = list(inputs())
    original, native = observe(args.rust, cases), observe(args.cpp, cases)
    gaps = [dict(index=i, input=x, rust=o, cpp=c)
            for i, (x, o, c) in enumerate(zip(cases, original, native)) if not same(o, c)]
    if gaps:
        target = ROOT / 'docs/parity/derived_query_diagnostics_5_0_6_initial_gaps.json'
        if not target.exists():
            target.write_text(json.dumps(dict(count=len(gaps), differences=gaps), indent=2) + '\n')
        print(json.dumps(gaps[:1], indent=2))
        raise SystemExit(f'{len(gaps)}/{len(cases)} derived query diagnostic-source differences')
    for case, output in zip(cases, original):
        if not case['logging'] or case['invalid_intent']:
            assert output['logs'] == []
        elif output['logs']:
            assert case['secret'] not in json.dumps(output['logs'], ensure_ascii=False)
        if case['logging'] and not case['failure']:
            assert output['independent_logs'][0]['comment'] == case['secret']
    corpus = dict(**metadata(), cases=[dict(input=x, expected=o) for x, o in zip(cases, original)])
    if args.check:
        with gzip.open(CORPUS, 'rt') as source:
            assert same(corpus, json.load(source)), 'Original corpus changed'
    else:
        CORPUS.write_bytes(gzip.compress((json.dumps(corpus, indent=2) + '\n').encode(), mtime=0))
    print(f'Compared {len(cases)}/{len(cases)} actual original/native derived query diagnostics')


if __name__ == '__main__':
    main()
