#!/usr/bin/env python3
"""Check protected stream shapes/configuration, exact errors and actual connection reuse."""
import argparse
import copy
import gzip
import hashlib
import itertools
import json
import os
import re
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
    assert len(cases) == 13104
    assert len({json.dumps(case, sort_keys=True) for case in cases}) == len(cases)
    return cases


def identity():
    value = source_identity()
    path = ROOT / 'tests/parity/rust_dynamic_json_privacy_5_0_6/src/bin/stream_shapes.rs'
    value['sources'][str(path.relative_to(ROOT))] = hashlib.sha256(path.read_bytes()).hexdigest()
    return value


def write_record(path, value):
    assert not path.exists(), path
    path.write_bytes(gzip.compress(json.dumps(value, ensure_ascii=False).encode(), mtime=0))


def startup_disposition(cases, original):
    """Apply only the reviewed eager-native/lazy-Rust startup boundary.

    Preserve raw observations. Source first-poll siblings provide complete Policy,
    error and safe diagnostic values; no SQL, trace, privacy or row value is erased.
    DiagnosticStream's original unpolled-drop contract independently requires a
    Cancelled terminal event with zero delivered rows once the wrapper exists.
    """
    def key(case):
        return json.dumps(case, sort_keys=True)

    lookup = {key(case): value for case, value in zip(cases, original)}
    required = []
    adapted = []
    for index, (case, raw) in enumerate(zip(cases, original)):
        expected = copy.deepcopy(raw)
        if case['chunk'] != '0' and case['intent'] == 'valid':
            blocked = case['policy'] == 'deny' or case['shape'] in {
                'relation', 'child', 'object_group'}
            if case['stop'] == '0':
                assert raw['opened'] and raw['error'] is None and not raw['rows']
                assert not raw['policy'] and not raw['logs'] and raw['selects'] == 0
                sibling = lookup[key(dict(case, stop='1'))]
                assert len(sibling['policy']) == 1
                expected['policy'] = copy.deepcopy(sibling['policy'])
                if blocked:
                    assert sibling['error'] and not sibling['logs'] and sibling['selects'] == 0
                    expected['error'] = sibling['error']
                elif case['logging']:
                    assert len(sibling['logs']) == 1
                    log = copy.deepcopy(sibling['logs'][0])
                    log['context'], count = re.subn(
                        r'execution_outcome: Some\((Success|Failure|Cancelled)\)',
                        'execution_outcome: Some(Cancelled)', log['context'])
                    assert count == 1
                    log['result_count'] = 0
                    log['summary'] = '0 rows returned'
                    expected['logs'] = [log]
            expected['opened'] = not blocked
        if not same(raw, expected):
            adapted.append(index)
        required.append(expected)
    return required, adapted


def verify_boundaries(cases, original, actual):
    for case, source, native in zip(cases, original, actual):
        for output in (source, native):
            assert output['released'], case
            if case['policy'] == 'deny':
                assert output['reuse']['value'] is None
                assert output['reuse']['error'] == 'graph write error: request policy error: COMPOSED_QUERY_DENIED'
                assert output['reuse']['selects'] == 0
            else:
                assert output['reuse']['value'] is True and output['reuse']['error'] is None
                assert output['reuse']['selects'] > 0
            assert len(output['reuse']['policy']) == 1
            if not case['logging']:
                assert output['logs'] == []
        if case['chunk'] == '0' or case['intent'] != 'valid':
            for output in (source, native):
                assert not output['policy'] and not output['logs'] and output['selects'] == 0
                assert output['opened'] is None and output['error'] is None and not output['rows']
                message = ('stream chunk size must be positive' if case['chunk'] == '0'
                           else 'query comment must not be empty' if case['intent'] == 'blank_comment'
                           else 'query purpose must not be empty')
                assert output['panic'] == message


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
        write_record(CORPUS, dict(**current, cases=[dict(input=i, expected=e)
                                                   for i, e in zip(cases, expected)]))
    else:
        corpus = json.loads(gzip.decompress(CORPUS.read_bytes()))
        for key, value in current.items():
            assert same(corpus[key], value), key
        assert same(cases, [row['input'] for row in corpus['cases']])
        expected = [row['expected'] for row in corpus['cases']]
        if args.rust:
            assert same(expected, observe(args.rust, cases)), 'Original stream observations changed'
    if not args.cpp:
        print('Actual original stream observations:', len(cases))
        return 0
    actual = observe(args.cpp, cases)
    required, adaptations = startup_disposition(cases, expected)
    raw_failures = [i for i, (a, b) in enumerate(zip(expected, actual)) if not same(a, b)]
    failures = [i for i, (a, b) in enumerate(zip(required, actual)) if not same(a, b)]
    if args.record_comparison:
        write_record(args.record_comparison,
                     dict(inputs=cases, original=expected, cpp=actual,
                          raw_failures=raw_failures, startup_adaptations=adaptations,
                          failures=failures))
    for index in failures[:4]:
        print('Case', index, cases[index])
        print('Different fields:', [key for key in set(required[index]) | set(actual[index])
                                    if not same(required[index].get(key), actual[index].get(key))])
    if not failures:
        verify_boundaries(cases, expected, actual)
    print(f'Stream shape contracts: {len(cases)-len(failures)}/{len(cases)}; '
          f'{len(cases)-len(raw_failures)} raw exact, {len(adaptations)} explicit startup adaptations')
    return bool(failures)


if __name__ == '__main__':
    raise SystemExit(main())
