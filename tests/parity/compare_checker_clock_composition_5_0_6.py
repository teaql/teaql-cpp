#!/usr/bin/env python3
"""Compare inherited alias/JSON and relation/Context-root clock composition."""
import argparse
import gzip
import itertools
import json
from pathlib import Path

from compare_values import same
from sample_clock_driver import observe_fixed

ROOT = Path(__file__).resolve().parents[2]
CORPUS = Path(__file__).with_name('checker_clock_composition_5_0_6_cases.json.gz')


def cases():
    for status, root, foreign_id, sparse, null_optional, wall in itertools.product(
        ('create', 'update', 'unknown'), (None, 42, 43), (0, 42, 43),
        (False, True), (False, True), (1001, 1704067200789)):
        yield dict(status=status, root=root, foreign_id=foreign_id, sparse=sparse,
                   null_optional=null_optional, wall_millis=wall)


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
            parser.error('--record requires the live original entity/Checker observer')
        expected = observe_fixed(args.rust, inputs, args.clock)
    else:
        data = json.loads(gzip.decompress(CORPUS.read_bytes()))
        if data['baseline'] != baseline or not same(inputs, [c['input'] for c in data['cases']]):
            raise RuntimeError('Clock composition matrix/baseline differs')
        expected = [c['expected'] for c in data['cases']]
        if args.rust and not same(expected, observe_fixed(args.rust, inputs, args.clock)):
            raise RuntimeError('Live original clock composition differs')
    actual = observe_fixed(args.cpp, inputs, args.clock)
    gaps = [dict(input=value, rust=source, cpp=native)
            for value, source, native in zip(inputs, expected, actual) if not same(source, native)]
    if gaps:
        path = ROOT / 'docs/parity/checker_clock_composition_initial_gaps_5_0_6.json.gz'
        if not path.exists():
            path.write_bytes(gzip.compress(json.dumps(dict(gaps=gaps), indent=2).encode(), mtime=0))
        print(json.dumps(gaps[0], ensure_ascii=True, indent=2))
        raise SystemExit(f'{len(gaps)}/{len(inputs)} clock composition differences')
    if args.record:
        CORPUS.write_bytes(gzip.compress(json.dumps(dict(baseline=baseline,
            scope='Original complete entity/Checker files, explicit required JSON, aliases, inheritance '
                  'and required/optional relationship FKs. Direct Checker only; full original runtime '
                  'assembly error retained separately. No SQL/save acceptance claim.',
            cases=[dict(input=value, expected=result) for value, result in zip(inputs, expected)]),
            separators=(',', ':')).encode(), mtime=0))
    print(f'Clock composition: {len(inputs)} cases / {len(inputs) * 6} field outcomes match')


if __name__ == '__main__':
    main()
