#!/usr/bin/env python3
"""Compare actual generated string-clock Checkers with the locked Rust crate."""
import argparse
import gzip
import itertools
import json
from pathlib import Path

from compare_values import same
from sample_clock_driver import observe_fixed

ROOT = Path(__file__).resolve().parents[2]
CORPUS = Path(__file__).with_name('checker_clock_text_5_0_6_cases.json.gz')


def cases():
    for status, code, sparse, wall in itertools.product(
        ('create', 'update', 'unknown'),
        ('existing code', '', '  spaced  ', '界\x00' + 'long text ' * 64),
        (False, True), (1001, 1704067200789)):
        yield dict(status=status, code=code, sparse=sparse, wall_millis=wall)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', type=Path, required=True)
    parser.add_argument('--rust', type=Path)
    parser.add_argument('--clock', type=Path, required=True)
    parser.add_argument('--record', action='store_true')
    args = parser.parse_args()
    inputs = list(cases())
    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    if args.record:
        if not args.rust:
            parser.error('--record requires the live original Rust observer')
        expected = observe_fixed(args.rust, inputs, args.clock)
    else:
        corpus = json.loads(gzip.decompress(CORPUS.read_bytes()))
        if corpus['baseline'] != baseline or not same(inputs, [c['input'] for c in corpus['cases']]):
            raise RuntimeError('String clock corpus matrix/baseline changed')
        expected = [c['expected'] for c in corpus['cases']]
        if args.rust and not same(expected, observe_fixed(args.rust, inputs, args.clock)):
            raise RuntimeError('Live original string clock outcomes differ')
    actual = observe_fixed(args.cpp, inputs, args.clock)
    gaps = [dict(input=value, rust=source, cpp=native)
            for value, source, native in zip(inputs, expected, actual) if not same(source, native)]
    if gaps:
        print(json.dumps(gaps[0], ensure_ascii=True, indent=2))
        raise SystemExit(f'{len(gaps)}/{len(inputs)} string clock differences')
    if args.record:
        CORPUS.write_bytes(gzip.compress(json.dumps(dict(baseline=baseline,
            scope='Complete unchanged original generated NoteChecker; exact direct string-clock '
                  'stored/ledger/load-state/diagnostic results under the test-only fixed wall clock. '
                  'No SQL or graph-save acceptance claim.',
            cases=[dict(input=value, expected=result) for value, result in zip(inputs, expected)]),
            separators=(',', ':')).encode(), mtime=0))
    print(f'Generated string clock Checkers: {len(inputs)} exact original/native outcomes')


if __name__ == '__main__':
    main()
