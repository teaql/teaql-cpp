#!/usr/bin/env python3
"""Compare generated JSON translation through actual one/stream SQLite execution."""
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


def inputs():
    fields = {
        'person': [{'name': 'Alice'}, {'name': 'Bob'}, {'name': 'absent'}, {'profile.note': 'alpha'},
                   {'profile.note': '__IS_NULL__'}, {'orders.lines.product.sku': 'B'}, {'email': ['alice', 'bob']}],
        'order': [{'code': 'Alpha'}, {'code': 'Beta'}, {'code': 'absent'}, {'customer.name': 'Alice'},
                  {'lines.product.sku': 'B'}, {'customer.profile.note': '__IS_NULL__'}, {'amount': [8, 20]}],
        'product': [{'sku': 'A'}, {'sku': 'B'}, {'sku': 'absent'}, {'price': [2, 4]},
                    {'line_list.quantity': [1, 3]}, {'line_list.order.customer.name': 'Alice'}, {'price': None}],
        'line': [{'quantity': [1, 3]}, {'product.sku': 'B'}, {'quantity': [100, 200]},
                 {'order.customer.name': 'Alice'}, {'order_id': 10}, {'product_id': 6}, {'quantity': None}],
    }
    controls = [{}, {'_orderBy': 'id'}, {'_orderBy': {'id': 'asc'}}, {'_start': 1, '_size': 1},
                {'_page': 2, '_pageSize': 1}, {'_page': 2, '_pageSize': 0}, {'unknown.name': 1},
                {'_size': 0}, {'_orderBy': ['id', {'version': 'desc'}]}]
    for entity in fields:
        filters = [[json.dumps(value, separators=(',', ':'))] for value in fields[entity] + controls]
        filters += [['{"_size":1}', '{"_page":2}'], ['{"_orderBy":"id"}', '{}'], ['{'],
                    ['{"unknown":1}', '{"id":[]}']]
        for sources, minimal, pre_slice in itertools.product(filters, (False, True), (False, True)):
            base = dict(entity=entity, filters=sources, execute=True, minimal=minimal,
                        pre_slice=pre_slice, unlimited=False)
            yield dict(base, execution='one')
            for chunk, stop in itertools.product((1, 8), (0, 1, 99)):
                yield dict(base, execution='stream', chunk=chunk, stop=stop)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', required=True); parser.add_argument('--rust', required=True)
    parser.add_argument('--check', action='store_true'); args = parser.parse_args()
    subprocess.run(['python3', str(ROOT / 'scripts/verify_upstream.py')], check=True)
    os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
    cases = list(inputs()); original = observe(args.rust, cases); actual = observe(args.cpp, cases)
    mismatches = [dict(index=i, input=x, rust=a, cpp=b) for i, (x, a, b) in enumerate(zip(cases, original, actual))
                  if not same(cpp_observation(a), b)]
    if mismatches:
        target = ROOT / 'docs/parity/dynamic_json_execution_5_0_6_initial_gaps.json'
        if not target.exists():
            target.write_text(json.dumps(dict(count=len(mismatches), differences=mismatches), indent=2) + '\n')
        print(json.dumps(mismatches[:1], indent=2))
        raise SystemExit(f'{len(mismatches)}/{len(cases)} original/native dynamic JSON execution differences')
    corpus = dict(baseline=json.load(open(ROOT / 'upstream.lock.json'))['commit'],
                  producer=json.load(open(ROOT / 'tests/generator/parity/dynamic_json_rust/PROVENANCE.json')),
                  adaptation='Only existing exact quoted Line order-column Debug spelling adaptation. Logical stream outcome/early close/reuse compared; source lazy versus C++ startup delivery remains its documented ownership adaptation. No SQL/error/value normalization.',
                  fixtures={name: hashlib.sha256((ROOT / 'tests/support' / name).read_bytes()).hexdigest()
                            for name in ('dynamic_json_schema.sql', 'dynamic_json_dataset.sql')},
                  cases=[dict(input=x, expected=y) for x, y in zip(cases, original)])
    target = Path(__file__).with_name('dynamic_json_execution_5_0_6_cases.json.gz')
    if args.check:
        with gzip.open(target, 'rt') as f:
            if not same(corpus, json.load(f)):
                raise SystemExit('Original dynamic JSON execution observations differ')
    else:
        target.write_bytes(gzip.compress((json.dumps(corpus, indent=2) + '\n').encode(), mtime=0))
    print('Compared', len(cases), 'actual original/native generated JSON one/stream outcomes')


if __name__ == '__main__':
    main()
