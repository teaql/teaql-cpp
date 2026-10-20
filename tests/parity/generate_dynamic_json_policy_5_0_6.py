#!/usr/bin/env python3
"""Qualify generated JSON queries with real RequestPolicy and SQLite execution."""
import argparse
import gzip
import hashlib
import itertools
import json
import os
from pathlib import Path
import subprocess
from compare_values import observe, same
from compare_generated_dynamic_json import cpp_observation

ROOT = Path(__file__).resolve().parents[2]
CORPUS = Path(__file__).with_name('dynamic_json_policy_5_0_6_cases.json.gz')


def inputs():
    fields = {
        'person': [{'name': 'Alice'}, {'name': 'absent'}, {'orders.lines.product.sku': 'B'}],
        'order': [{'code': 'Alpha'}, {'amount': [8, 20]}, {'customer.profile.note': '__IS_NULL__'}],
        'product': [{'sku': 'B'}, {'price': [2, 4]}, {'line_list.order.customer.name': 'Alice'}],
        'line': [{'quantity': [1, 3]}, {'product.sku': 'B'}, {'order.customer.name': 'Alice'}],
    }
    for entity, field_filters in fields.items():
        filters = [[json.dumps(value, separators=(',', ':'))]
                   for value in [{}, *field_filters, {'_start': 1, '_size': 1}, {'_size': 0}]]
        filters += [['{"_orderBy":"id"}', '{"_size":1}'], ['{']]
        for sources, policy, execution, minimal, logging in itertools.product(
                filters, ('allow', 'restrict', 'slice', 'deny'), ('list', 'one', 'stream', 'count', 'exists', 'page'),
                (False, True), (False, True)):
            yield dict(entity=entity, filters=sources, policy=policy, execution=execution,
                       execute=True, minimal=minimal, logging=logging,
                       pre_slice=False, unlimited=False, chunk=1, stop=99)


def adapted(original):
    # The existing exact Line foreign-key Debug spelling adaptation applies to
    # complete policy snapshots too. SQL, diagnostics, errors and values stay exact.
    out = cpp_observation(original)
    for call in out.get('policy_calls', []):
        for phase in ('before', 'after'):
            call[phase] = cpp_observation({'query': call[phase]})['query']
    return out


def metadata():
    return dict(baseline=json.loads((ROOT / 'upstream.lock.json').read_text())['commit'],
                producer=json.loads((ROOT / 'tests/generator/parity/dynamic_json_rust/PROVENANCE.json').read_text()),
                fixtures={name: hashlib.sha256((ROOT / 'tests/support' / name).read_bytes()).hexdigest()
                          for name in ('dynamic_json_schema.sql', 'dynamic_json_dataset.sql')},
                adaptation='Only the existing exact quoted Line order-column Debug spelling; applied to query and policy snapshots. No SQL/error/value normalization. Original lazy versus native startup stream delivery remains the documented ownership adaptation.')


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
            for i, (x, o, c) in enumerate(zip(cases, original, native)) if not same(adapted(o), c)]
    if gaps:
        target = ROOT / 'docs/parity/dynamic_json_policy_5_0_6_initial_gaps.json'
        if not target.exists():
            target.write_text(json.dumps(dict(count=len(gaps), differences=gaps), indent=2) + '\n')
        print(json.dumps(gaps[:1], indent=2))
        raise SystemExit(f'{len(gaps)}/{len(cases)} generated JSON policy differences')
    for case, output in zip(cases, original):
        if case['policy'] == 'deny' or 'panic' in output:
            assert output['database_selects'] == 0 and output['logs'] == []
        if not case['logging']:
            assert output['logs'] == []
    corpus = dict(**metadata(), cases=[dict(input=x, expected=o) for x, o in zip(cases, original)])
    if args.check:
        with gzip.open(CORPUS, 'rt') as source:
            assert same(corpus, json.load(source)), 'Original corpus changed'
    else:
        CORPUS.write_bytes(gzip.compress((json.dumps(corpus, indent=2) + '\n').encode(), mtime=0))
    print(f'Compared {len(cases)}/{len(cases)} actual original/native dynamic JSON policy outcomes')


if __name__ == '__main__':
    main()
