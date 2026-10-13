#!/usr/bin/env python3
"""Record unmodified Rust core layout and presentation protocol observations."""
import argparse
import itertools
import json
from pathlib import Path
import subprocess
from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]


def cases():
    schemas = [[], [['_total', 'i64']], [['_total', 'text']], [['_total', 'i64'], ['_title', 'text']]]
    for names, relations, first, second in itertools.product(
            [[], ['id'], ['_total', 'id', 'id']], [[], ['lines'], ['lines', 'lines', 'other']], schemas, schemas):
        yield dict(kind='layout', names=names, relations=relations, schemas=[first, second, second, first])
    for depth, entity, address in itertools.product([0, 1, 2, 127, 128, 129, 255], ['Item', 'Other', ''], [0, 2**32 - 1]):
        operations = [['leave'], *[['enter', entity, address + i] for i in range(depth)],
                      ['enter', entity, address], ['enter', 'Other', address], ['leave'], ['leave'],
                      *[['leave'] for _ in range(depth)], ['enter', entity, address], ['leave'], ['leave']]
        yield dict(kind='traversal', operations=operations)
    for borrowed, payload in itertools.product([False, True], [None, False, 0, '', [], {}, {'nested': [{'text': 'x'*128}], 'value': 2.5}]):
        yield dict(kind='presentation', borrowed=borrowed, payload=payload)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rust', required=True)
    parser.add_argument('--cpp', required=True)
    args = parser.parse_args()
    subprocess.run(['python3', str(ROOT/'scripts/verify_upstream.py')], check=True)
    inputs = list(cases())
    expected, actual = observe(args.rust, inputs), observe(args.cpp, inputs)
    failures = [(i, c, a, b) for i, (c, a, b) in enumerate(zip(inputs, expected, actual)) if not same(a, b)]
    for i, case, a, b in failures[:5]:
        print(f'Mismatch {i}: {case}\nRust: {a}\nC++: {b}')
    if failures:
        raise SystemExit(f'{len(failures)}/{len(inputs)} core shape/presentation cases differ')
    lock = json.loads((ROOT/'upstream.lock.json').read_text())
    corpus = dict(baseline=lock['commit'], version=lock['version'],
                  note='Direct public frozen core calls. Layout names/equality/schema reuse and preserved relation availability; path-local enter/leave; custom presentation fixture exercises original Entity defaults without generated model claims.',
                  cases=[dict(input=c, expected=v) for c, v in zip(inputs, expected)])
    Path(__file__).with_name('core_shape_presentation_5_0_6_cases.json').write_text(json.dumps(corpus, indent=2)+'\n')
    print(f'Compared {len(inputs)} original/C++ core shape/presentation cases')


if __name__ == '__main__':
    main()
