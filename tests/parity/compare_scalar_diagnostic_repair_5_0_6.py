#!/usr/bin/env python3
"""Compare actual scalar keyword repairs with the existing explicit E presentation adapter."""
import argparse
import gzip
import hashlib
import itertools
import json
import os
from pathlib import Path
from compare_values import observe, same
from compare_query_alias_5_0_6 import identity as alias_identity
from compare_generated_expression_diagnostics import adapt
ROOT = Path(__file__).resolve().parents[2]
CORPUS = Path(__file__).with_name('scalar_diagnostic_repair_5_0_6_cases.json.gz')


def inputs():
    return [dict(field=field, selection=selection, minimal=minimal, id=identity,
                 deleted=deleted, action=action, style=style)
            for field, selection, minimal, identity, deleted, action, style in itertools.product(
                ['class', 'operator', 'template'], ['empty', 'selected', 'removed', 'replaced', 'self'],
                [False, True], [10, 12], [False, True], ['unwrap', 'eval'], ['legacy', 'canonical'])]


def identity():
    result = alias_identity()
    source = ROOT / 'tests/parity/rust_query_alias_5_0_6/src/bin/scalar_diagnostic_repair.rs'
    result['sources'][str(source.relative_to(ROOT))] = hashlib.sha256(source.read_bytes()).hexdigest()
    return result


def adapted(output):
    if output['diagnostic'] is None:
        return output
    result = output.copy()
    result['diagnostic'] = adapt(result['diagnostic'])
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp')
    parser.add_argument('--rust')
    parser.add_argument('--record-source', action='store_true')
    parser.add_argument('--record-comparison', type=Path)
    args = parser.parse_args()
    assert args.cpp or args.record_source
    os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
    current, cases = identity(), inputs()
    if args.record_source:
        assert args.rust and not CORPUS.exists()
        expected = observe(args.rust, cases)
        CORPUS.write_bytes(gzip.compress(json.dumps(dict(**current, cases=[dict(input=i, expected=e)
            for i,e in zip(cases,expected)]), ensure_ascii=False).encode(), mtime=0))
    else:
        corpus = json.loads(gzip.decompress(CORPUS.read_bytes()))
        for key, value in current.items():
            assert corpus[key] == value, key
        assert same(cases, [row['input'] for row in corpus['cases']])
        expected = [row['expected'] for row in corpus['cases']]
        if args.rust:
            if not same(expected, observe(args.rust, cases)):
                raise RuntimeError('Original scalar diagnostic behavior changed')
    if not args.cpp:
        print('Recorded unchanged original scalar diagnostic observations:', len(cases))
        return 0
    actual = observe(args.cpp, cases)
    failures = [index for index,(a,b) in enumerate(zip(expected,actual)) if not same(adapted(a),b)]
    if args.record_comparison:
        assert not args.record_comparison.exists()
        args.record_comparison.write_bytes(gzip.compress(json.dumps(dict(inputs=cases,original=expected,
            cpp=actual,failures=failures),ensure_ascii=False).encode(),mtime=0))
    for index in failures[:3]:
        print('Case',index,cases[index]);print('Different fields:',[key for key in expected[index]
            if not same(adapted(expected[index])[key],actual[index][key])])
    print(f'Scalar keyword diagnostic repairs: {len(cases)-len(failures)}/{len(cases)} original outcomes agree (existing C++ presentation adaptation)')
    return bool(failures)


if __name__ == '__main__':
    raise SystemExit(main())
