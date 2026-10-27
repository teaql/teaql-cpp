#!/usr/bin/env python3
"""Check protected stream shapes/configuration, exact errors and actual connection reuse."""
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
CORPUS = Path(__file__).with_name('stream_shapes_5_0_6_cases.json.gz')
SHAPES = ['plain', 'readonly', 'raw_projection', 'raw_filter', 'raw_outer',
          'facet', 'grouped', 'relation', 'child', 'object_group',
          'count_metric', 'sum_metric', 'statistic', 'missing_table']
ENTITIES = ['school', 'academy', 'research_academy', 'enrollment']


def inputs():
    cases = []
    for entity, shape, optimization, chunk, stop, policy, logging in itertools.product(
            ENTITIES, SHAPES, ['none', 'id_set', 'continuous'],
            ['default', '0', '1', '4'], ['0', '1', '99'],
            ['allow', 'restrict', 'deny'], [False, True]):
        cases.append(dict(entity=entity, shape=shape, optimization=optimization,
                          chunk=chunk, stop=stop, policy=policy, logging=logging,
                          slice='none', intent='valid'))
    for entity, optimization, chunk, stop, slicing in itertools.product(
            ENTITIES, ['none', 'id_set', 'continuous'], ['default', '0', '1', '4'],
            ['0', '1', '99'], ['zero', 'offset', 'beyond']):
        cases.append(dict(entity=entity, shape='plain', optimization=optimization,
                          chunk=chunk, stop=stop, policy='allow', logging=True,
                          slice=slicing, intent='valid'))
    for entity, shape, intent, policy, optimization, logging, stop in itertools.product(
            ENTITIES, ['plain', 'relation', 'raw_outer'],
            ['blank_comment', 'blank_purpose'], ['allow', 'deny'],
            ['none', 'id_set', 'continuous'], [False, True], ['0', '1']):
        cases.append(dict(entity=entity, shape=shape, optimization=optimization,
                          chunk='1', stop=stop, policy=policy, logging=logging,
                          slice='none', intent=intent))
    if not (len(cases) == 13104):
        raise AssertionError()
    if not (len({json.dumps(case, sort_keys=True) for case in cases}) == len(cases)):
        raise AssertionError()
    return cases


def identity():
    value = source_identity()
    path = ROOT / 'tests/parity/rust_dynamic_json_privacy_5_0_6/src/bin/stream_shapes.rs'
    value['sources'][str(path.relative_to(ROOT))] = hashlib.sha256(path.read_bytes()).hexdigest()
    return value


def write_record(path, value):
    if not (not path.exists()):
        raise AssertionError(path)
    path.write_bytes(gzip.compress(json.dumps(value, ensure_ascii=False).encode(), mtime=0))


def verify_boundaries(cases, original, actual):
    for case, source, native in zip(cases, original, actual):
        for output in (source, native):
            if not (output['released']):
                raise AssertionError(case)
            if case['policy'] == 'deny':
                if not (output['reuse']['value'] is None):
                    raise AssertionError()
                if not (output['reuse']['error'] == 'graph write error: request policy error: COMPOSED_QUERY_DENIED'):
                    raise AssertionError()
                if not (output['reuse']['selects'] == 0):
                    raise AssertionError()
            else:
                if not (output['reuse']['value'] is True and output['reuse']['error'] is None):
                    raise AssertionError()
                if not (output['reuse']['selects'] > 0):
                    raise AssertionError()
            if not (len(output['reuse']['policy']) == 1):
                raise AssertionError()
            if not case['logging']:
                if not (output['logs'] == []):
                    raise AssertionError()
        if case['chunk'] == '0' or case['intent'] != 'valid':
            for output in (source, native):
                if not (not output['policy'] and not output['logs'] and output['selects'] == 0):
                    raise AssertionError()
                if not (output['opened'] is None and output['error'] is None and not output['rows']):
                    raise AssertionError()
                message = ('stream chunk size must be positive' if case['chunk'] == '0'
                           else 'query comment must not be empty' if case['intent'] == 'blank_comment'
                           else 'query purpose must not be empty')
                if not (output['panic'] == message):
                    raise AssertionError()


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
        if not (args.rust and not CORPUS.exists()):
            raise AssertionError()
        expected = observe(args.rust, cases)
        write_record(CORPUS, dict(**current, cases=[dict(input=i, expected=e)
                                                   for i, e in zip(cases, expected)]))
    else:
        corpus = json.loads(gzip.decompress(CORPUS.read_bytes()))
        for key, value in current.items():
            if not (same(corpus[key], value)):
                raise AssertionError(key)
        if not (same(cases, [row['input'] for row in corpus['cases']])):
            raise AssertionError()
        expected = [row['expected'] for row in corpus['cases']]
        if args.rust:
            if not same(expected, observe(args.rust, cases)):
                raise RuntimeError('Original stream observations changed')
    if not args.cpp:
        print('Actual original stream observations:', len(cases))
        return 0
    actual = observe(args.cpp, cases)
    failures = [i for i, (a, b) in enumerate(zip(expected, actual)) if not same(a, b)]
    if args.record_comparison:
        write_record(args.record_comparison,
                     dict(inputs=cases, original=expected, cpp=actual,
                          raw_failures=failures, startup_adaptations=[],
                          comparison='Exact original startup and item behavior; no expectation rewrite',
                          failures=failures))
    for index in failures[:4]:
        print('Case', index, cases[index])
        print('Different fields:', [key for key in set(expected[index]) | set(actual[index])
                                    if not same(expected[index].get(key), actual[index].get(key))])
    if not failures:
        verify_boundaries(cases, expected, actual)
    print(f'Stream shape contracts: {len(cases)-len(failures)}/{len(cases)}; '
          f'{len(cases)-len(failures)} raw exact; no startup adaptations')
    return bool(failures)


if __name__ == '__main__':
    raise SystemExit(main())
