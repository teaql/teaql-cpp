#!/usr/bin/env python3
"""Record actual Rust subsecond carry/borrow and full-range time arithmetic."""
import argparse
import itertools
import json
import subprocess
from pathlib import Path
from compare_values import observe


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--rust', required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    subprocess.run(['python3', str(root / 'scripts/verify_upstream.py')], check=True)
    seconds = [-(1 << 63), -(1 << 63) + 1, -10000000000, -9223372037, -1, 0, 1,
               9223372036, 10000000000, (1 << 63) - 2, (1 << 63) - 1]
    durations = [0, 1, 2, (1 << 63) - 1, 1 << 63, (1 << 64) - 2, (1 << 64) - 1]
    cases = [dict(time=dict(seconds=str(s), nanos=n), duration=dict(seconds=str(d), nanos=dn))
             for s, n, d, dn in itertools.product(seconds, [0, 1, 500000000, 999999999],
                                                   durations, [0, 1, 500000000, 999999999])]
    outputs = observe(args.rust, cases)
    corpus = dict(baseline=json.loads((root / 'upstream.lock.json').read_text())['commit'],
                  note='Actual original SystemTime checked and assignment duration operations; no copied arithmetic algorithm.',
                  cases=[dict(input=i, expected=o) for i, o in zip(cases, outputs)])
    Path(__file__).with_name('system_time_arithmetic_scope_cases.json').write_text(
        json.dumps(corpus, separators=(',', ':')) + '\n')
    print(f'Recorded {len(cases)} original time arithmetic inputs')


if __name__ == '__main__':
    main()
