#!/usr/bin/env python3
"""Record original diagnostic-source composition; native comparison is pending."""
import argparse
import gzip
import itertools
import json
import os
from pathlib import Path
from compare_values import observe, same
from generate_derived_query_diagnostics_5_0_6 import metadata

CORPUS = Path(__file__).with_name('derived_query_composition_source_5_0_6_cases.json.gz')


def inputs():
    first, second = 'PRIVATE-FUTURE-A', 'PRIVATE-FUTURE-B'
    for endpoint, logging, failure, invalid, shape, ordering in itertools.product(
            ('all', 'owned', 'list', 'aggregate', 'stream'),
            (False, True), (False, True), (False, True),
            ('direct', 'aggregate', 'nested'), ('facets', 'count_between', 'clone_between')):
        steps = [dict(action='facet', shape=shape, secret=first)]
        if ordering == 'count_between':
            steps.append(dict(action='count', alias='again'))
        elif ordering == 'clone_between':
            steps.append(dict(action='clone'))
        steps += [dict(action='facet', shape=shape, secret=second), dict(action='empty')]
        yield dict(kind='relation', endpoint=endpoint, logging=logging, failure=failure,
                   invalid_intent=invalid,
                   secret=f'PRIVATE-REMOVED-COUNT {first} {second}', diagnostic_steps=steps)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rust', required=True)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
    cases = list(inputs())
    expected = observe(args.rust, cases)
    for case, output in zip(cases, expected):
        if not case['logging'] or case['invalid_intent']:
            assert output['logs'] == []
        for log in output['logs']:
            text = json.dumps(log, ensure_ascii=False)
            for private in ('PRIVATE-REMOVED-COUNT', 'PRIVATE-FUTURE-A', 'PRIVATE-FUTURE-B'):
                assert private not in text, 'Original privacy contract changed'
        if case['logging'] and not case['failure']:
            assert output['independent_logs'][0]['comment'] == case['secret']
    corpus = dict(**metadata(), qualification='original-only; corresponding native sequence adapter and comparison remain required',
                  cases=[dict(input=x, expected=y) for x, y in zip(cases, expected)])
    if args.check:
        with gzip.open(CORPUS, 'rt') as source:
            assert same(corpus, json.load(source)), 'Original composition outcomes changed'
    else:
        CORPUS.write_bytes(gzip.compress((json.dumps(corpus, indent=2) + '\n').encode(), mtime=0))
    print(f'Recorded {len(cases)} actual original composition outcomes; native qualification pending')


if __name__ == '__main__':
    main()
