#!/usr/bin/env python3
"""Compare full typed source/result separation across all eight public execution entries."""
import argparse
import gzip
import hashlib
import itertools
import json
import os
from pathlib import Path

from compare_composed_general_5_0_6 import identity as source_identity
from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]
CORPUS = Path(__file__).with_name('execution_result_models_5_0_6_cases.json.gz')
MODELS = ['school', 'academy', 'research_academy', 'enrollment']
MODES = ['list', 'rows', 'first', 'one', 'count', 'exists', 'page', 'stream']


def inputs():
    cases = []
    for entity, target, minimal, mode, shape, optimization in itertools.product(
            MODELS, MODELS, [False, True], MODES,
            ['plain', 'readonly', 'facet', 'raw_projection'],
            ['none', 'id_set', 'continuous']):
        cases.append(dict(entity=entity, target=target, minimal=minimal, mode=mode,
                          shape=shape, optimization=optimization, policy='allow',
                          intent='valid', logging=True))
    for entity, target, mode, policy in itertools.product(
            MODELS, MODELS, MODES, ['deny', 'reject_raw']):
        cases.append(dict(entity=entity, target=target, minimal=False, mode=mode,
                          shape='readonly', optimization='none', policy=policy,
                          intent='valid', logging=True))
    assert len(cases) == 3328
    return cases


def identity():
    value = source_identity()
    path = ROOT / 'tests/parity/rust_dynamic_json_privacy_5_0_6/src/bin/execution_result_models.rs'
    value['sources'][str(path.relative_to(ROOT))] = hashlib.sha256(path.read_bytes()).hexdigest()
    return value


def check_source(cases, results):
    groups = {}
    for case, result in zip(cases, results):
        assert result['panic'] is None and result['released'], (case, result)
        if case['policy'] != 'allow':
            assert result['error'] == 'graph write error: request policy error: COMPOSED_QUERY_DENIED'
            assert result['selects'] == 0 and not result['logs'] and not result['rows']
        key = json.dumps({name: value for name, value in case.items() if name != 'target'},
                         sort_keys=True)
        previous = groups.setdefault(key, result)
        assert same(previous['query'], result['query']), case
        if previous['policy'] and result['policy']:
            assert same(previous['policy'][0], result['policy'][0]), case
        # These entries never decode R. Require all results, errors, Policy and
        # diagnostics to stay identical across target choices, not just row IDs.
        if case['mode'] in {'rows', 'count', 'exists'}:
            assert same(previous, result), case


def write_record(path, value):
    assert not path.exists(), path
    path.write_bytes(gzip.compress(json.dumps(value, ensure_ascii=False).encode(), mtime=0))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp')
    parser.add_argument('--rust')
    parser.add_argument('--record-source', action='store_true')
    parser.add_argument('--record-comparison', type=Path)
    args = parser.parse_args()
    os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
    current, cases = identity(), inputs()
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
                raise RuntimeError('Original adapted-model outcomes changed')
    if not args.cpp:
        print('Actual original source/result observations:', len(cases))
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
    if not failures:
        check_source(cases, actual)
    print(f'Full execution result models: {len(cases)-len(failures)}/{len(cases)} exact original observations')
    return bool(failures)


if __name__ == '__main__':
    raise SystemExit(main())
