#!/usr/bin/env python3
"""Compare real hot-cache execution under Policy changes and explicit cache/store replacement."""
import argparse
import gzip
import hashlib
import itertools
import json
import os
from pathlib import Path

from compare_composed_general_5_0_6 import identity as original_identity
from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]
CORPUS = Path(__file__).with_name('hot_execution_5_0_6_cases.json.gz')


def steps(profile):
    def step(policy='allow', offset='0', **actions):
        return dict(policy=policy, offset=offset, **actions)
    if profile == 'policy':
        return [step(), step(), step('deny'), step('restrict'), step(), step(offset='1')]
    if profile == 'replace':
        return [step(), step(), step(replace=True), step(), step(offset='1')]
    if not (profile == 'membership'):
        raise AssertionError()
    return [step(), step(), step(update=True), step(), step(clear=True),
            step(clear=True, replace=True), step(offset='1'), step('deny')]


def inputs():
    cases = []
    for entity, mode, shape, optimization, cache in itertools.product(
            ['school', 'academy', 'research_academy', 'enrollment'],
            ['list', 'rows', 'first', 'one', 'count', 'exists', 'page', 'stream'],
            ['plain', 'readonly', 'facet', 'grouped'],
            ['none', 'id_set', 'continuous'], [False, True]):
        cases.append(dict(entity=entity, mode=mode, shape=shape,
                          optimization=optimization, cache=cache,
                          profile='policy', steps=steps('policy')))
    for entity, mode, shape, optimization, cache, profile in itertools.product(
            ['school', 'academy', 'research_academy', 'enrollment'],
            ['list', 'one', 'page'], ['plain', 'readonly'],
            ['none', 'id_set', 'continuous'], [False, True], ['replace', 'membership']):
        cases.append(dict(entity=entity, mode=mode, shape=shape,
                          optimization=optimization, cache=cache,
                          profile=profile, steps=steps(profile)))
    if not (len(cases) == 1056):
        raise AssertionError()
    if not (sum(len(case['steps']) for case in cases) == 6480):
        raise AssertionError()
    return cases


def identity():
    value = original_identity()
    path = ROOT / 'tests/parity/rust_dynamic_json_privacy_5_0_6/src/bin/hot_execution.rs'
    value['sources'][str(path.relative_to(ROOT))] = hashlib.sha256(path.read_bytes()).hexdigest()
    return value


def write_record(path, value):
    if not (not path.exists()):
        raise AssertionError(path)
    path.write_bytes(gzip.compress(json.dumps(value, ensure_ascii=False).encode(), mtime=0))


def check_source(cases, outputs):
    for case, rounds in zip(cases, outputs):
        if not (len(rounds) == len(case['steps'])):
            raise AssertionError()
        for step, result in zip(case['steps'], rounds):
            if not (result['panic'] is None and result['released']):
                raise AssertionError((case, step, result))
            if step['policy'] == 'deny':
                if not (result['error'] == 'graph write error: request policy error: COMPOSED_QUERY_DENIED'):
                    raise AssertionError()
                if not (not result['rows'] and result['selects'] == 0):
                    raise AssertionError()
                if not (result['logs'] == [] and result['cache_events'] == []):
                    raise AssertionError()
                if not (len(result['policy']) == 1):
                    raise AssertionError()
            for log in result['logs']:
                # Generated facets own their independent application comment.
                # The source retains that literal text; root binding redactions
                # do not classify arbitrary child free text as a secret.
                if log['purpose'] == 'Compare scope [REDACTED]':
                    if not ('PRIVATE-SCHOOL-CANARY' not in json.dumps(log)):
                        raise AssertionError((case, step, log))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp')
    parser.add_argument('--rust')
    parser.add_argument('--record-source', action='store_true')
    parser.add_argument('--record-source-raw', type=Path)
    parser.add_argument('--record-comparison', type=Path)
    args = parser.parse_args()
    os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
    current, cases = identity(), inputs()
    if args.record_source:
        if not (args.rust and not CORPUS.exists()):
            raise AssertionError()
        expected = observe(args.rust, cases)
        if args.record_source_raw:
            write_record(args.record_source_raw, dict(**current, inputs=cases, original=expected))
        check_source(cases, expected)
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
        check_source(cases, expected)
        if args.rust:
            if not same(expected, observe(args.rust, cases)):
                raise RuntimeError('Actual original hot-cache outcomes changed')
    if not args.cpp:
        print('Recorded actual original hot-cache scenarios: 1056 / 6480 generated executions')
        return 0
    actual = observe(args.cpp, cases)
    failures = [i for i, (a, b) in enumerate(zip(expected, actual)) if not same(a, b)]
    if args.record_comparison:
        write_record(args.record_comparison,
                     dict(inputs=cases, original=expected, cpp=actual, failures=failures))
    for index in failures[:3]:
        print('Case', index, cases[index])
        for round_index, (a, b) in enumerate(zip(expected[index], actual[index])):
            fields = [key for key in set(a) | set(b) if not same(a.get(key), b.get(key))]
            if fields:
                print('Round', round_index, 'different fields:', fields)
    if not failures:
        check_source(cases, actual)
    print(f'Protected hot-cache execution: {len(cases)-len(failures)}/{len(cases)} '
          'exact scenarios / 6480 generated executions')
    return bool(failures)


if __name__ == '__main__':
    raise SystemExit(main())
