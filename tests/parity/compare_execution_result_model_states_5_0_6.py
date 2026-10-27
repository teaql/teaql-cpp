#!/usr/bin/env python3
"""Exercise loaded NULL and NotLoaded through disjoint result-model layouts."""
import argparse
import gzip
import hashlib
import itertools
import json
import os
from pathlib import Path

from compare_execution_result_models_5_0_6 import check_source, identity as base_identity, write_record
from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]
CORPUS = Path(__file__).with_name('execution_result_model_states_5_0_6_cases.json.gz')


def inputs():
    return [dict(entity=entity, target=target, minimal=True, mode=mode, shape=shape,
                 optimization='none', policy='allow', intent='valid', logging=True)
            for entity, target, mode, shape in itertools.product(
                ['school', 'enrollment'], ['research_academy', 'enrollment'],
                ['list', 'rows', 'first', 'one', 'count', 'exists', 'page', 'stream'],
                ['readonly_null', 'scalar_null'])]


def identity():
    result = base_identity()
    path = ROOT / 'tests/parity/rust_dynamic_json_privacy_5_0_6/src/bin/execution_result_model_states.rs'
    result['sources'][str(path.relative_to(ROOT))] = hashlib.sha256(path.read_bytes()).hexdigest()
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp')
    parser.add_argument('--rust')
    parser.add_argument('--record-source', action='store_true')
    parser.add_argument('--record-comparison', type=Path)
    args = parser.parse_args()
    os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
    current, cases = identity(), inputs()
    assert len(cases) == 64
    if args.record_source:
        assert args.rust and not CORPUS.exists()
        expected = observe(args.rust, cases)
        check_source(cases, expected)
        write_record(CORPUS, dict(**current, cases=[dict(input=i, expected=e)
                                                   for i, e in zip(cases, expected)]))
    else:
        corpus = json.loads(gzip.decompress(CORPUS.read_bytes()))
        for key, value in current.items():
            assert same(corpus[key], value), key
        assert same(cases, [row['input'] for row in corpus['cases']])
        expected = [row['expected'] for row in corpus['cases']]
        check_source(cases, expected)
        if args.rust:
            if not same(expected, observe(args.rust, cases)):
                raise RuntimeError('Original source observations changed')
    if not args.cpp:
        print('Actual original result-model load-state observations:64')
        return 0
    actual = observe(args.cpp, cases)
    failures = [i for i, (a, b) in enumerate(zip(expected, actual)) if not same(a, b)]
    if args.record_comparison:
        write_record(args.record_comparison,
                     dict(inputs=cases, original=expected, cpp=actual, failures=failures))
    for index in failures[:3]:
        print('Case', index, cases[index])
        print('Different fields:', [key for key in set(expected[index]) | set(actual[index])
                                    if not same(expected[index].get(key), actual[index].get(key))])
    print(f'Execution result-model load states: {len(cases)-len(failures)}/{len(cases)} exact original observations')
    return bool(failures)


if __name__ == '__main__':
    raise SystemExit(main())
