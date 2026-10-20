#!/usr/bin/env python3
"""Compare temporal bootstrap, clock replacement and registered Checker overrides."""
import argparse
import gzip
import itertools
import json
import os
from pathlib import Path

from compare_values import same
from sample_clock_driver import observe_fixed

ROOT = Path(__file__).resolve().parents[2]
CORPUS = Path(__file__).with_name('bootstrap_temporal_extensions_5_0_6_cases.json.gz')


def cases():
    for state, module, checker, swap, wall, rounds in itertools.product(
            ('empty', 'existing', 'corrupt'), ('plain', 'checker', 'behavior', 'combined'),
            ('none', 'observe', 'fix', 'deny', 'late_deny'), (False, True), (1001, 1790812800123), (1, 2)):
        yield dict(state=state, module=module, checker=checker, wall_millis=wall, rounds=rounds,
                   business_millis=-1 if swap else 1704067200789,
                   replacement_business_millis=1704067200789 if swap else -1,
                   tick_per_round=1000, logging=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', required=True)
    parser.add_argument('--rust')
    parser.add_argument('--clock', required=True)
    parser.add_argument('--record', action='store_true')
    args = parser.parse_args()
    if args.record and not args.rust:
        parser.error('--record requires the original-source runtime observer')
    os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
    inputs = list(cases())
    if args.record:
        expected = observe_fixed(args.rust, inputs, args.clock)
    else:
        corpus = json.loads(gzip.decompress(CORPUS.read_bytes()))
        assert corpus['baseline'] == json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
        assert same(inputs, [case['input'] for case in corpus['cases']])
        expected = [case['expected'] for case in corpus['cases']]
        if args.rust and not same(expected, observe_fixed(args.rust, inputs, args.clock)):
            raise RuntimeError('Temporal extension corpus differs from fresh original execution')
    actual = observe_fixed(args.cpp, inputs, args.clock)
    gaps = [dict(index=index, input=value, rust=source, cpp=native)
            for index, (value, source, native) in enumerate(zip(inputs, expected, actual))
            if not same(source, native)]
    if gaps:
        path = ROOT / 'docs/parity/bootstrap_temporal_extensions_5_0_6_initial_gaps.json.gz'
        if not path.exists():
            path.write_bytes(gzip.compress(json.dumps(gaps, indent=2).encode(), mtime=0))
        print(json.dumps(gaps[0], indent=2))
        raise SystemExit(f'{len(gaps)} temporal extension differences; see {path}')
    if args.record:
        corpus = dict(baseline=json.loads((ROOT / 'upstream.lock.json').read_text())['commit'],
                      scope='Unchanged generated temporal domain/runtime/SQLite algorithms; explicit '
                            'registered Checker and business clock replacement; fixed test wall clock. '
                            'Complete bootstrap rows/logs/evidence and subsequent audited update '
                            'rows/diagnostics/Checker order. No normalization.',
                      cases=[dict(input=value, expected=source) for value, source in zip(inputs, expected)])
        CORPUS.write_bytes(gzip.compress((json.dumps(corpus, separators=(',', ':')) + '\n').encode(), mtime=0))
    print(f'Temporal bootstrap extensions: {len(inputs)}/{len(inputs)} exact original/native outcomes')


if __name__ == '__main__':
    main()
