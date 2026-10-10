#!/usr/bin/env python3
"""Record requests/results through the original public EntityDataService."""
import argparse
import itertools
import json
import subprocess
from pathlib import Path

from compare_values import observe


def inputs():
    cases = []
    for capture, cache, backend, aggregate, failure, mode in itertools.product(
            (False, True), (False, True), (False, True), (False, True),
            (False, True), ('rows', 'owned', 'list', 'aggregates')):
        cases.append(dict(capture=capture, cache=cache, backend=backend,
                          aggregate=aggregate, failure=failure, mode=mode,
                          continuous=False, purpose='Read items', comment=None,
                          limit=None, calls=2))
    for capture, aggregate, mode in itertools.product(
            (False, True), (False, True), ('rows', 'list', 'aggregates')):
        cases.append(dict(capture=capture, cache=False, backend=False,
                          aggregate=aggregate, failure=False, mode=mode,
                          continuous=True, purpose='Read items', comment='business',
                          limit=None, calls=3))
    for purpose, limit in itertools.product(
            ('', ' \t\n', '\u2003', '  Inspect items  ', 'Purpose with\nnew line'),
            (None, 0, 1, 2, 1001)):
        cases.append(dict(capture=True, cache=False, backend=False,
                          aggregate=False, failure=False, mode='rows',
                          continuous=False, purpose=purpose, comment='business',
                          limit=limit, calls=1))
    return cases


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--rust', required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    subprocess.run(['python3', str(root / 'scripts/verify-upstream.py')], check=True)
    lock = json.loads((root / 'upstream.lock.json').read_text())
    cases = inputs()
    outputs = observe(args.rust, cases)
    corpus = dict(baseline=lock['commit'],
                  note='Original public fetch_all/owned/list/relation-aggregate calls, with real Context caches/cursor stores; no copied orchestration.',
                  cases=[dict(input=case, expected=output) for case, output in zip(cases, outputs)])
    Path(__file__).with_name('entity_query_cases.json').write_text(
        json.dumps(corpus, indent=2, ensure_ascii=True) + '\n')
    print(f'Recorded {len(cases)} public entity-query observations')


if __name__ == '__main__':
    main()
