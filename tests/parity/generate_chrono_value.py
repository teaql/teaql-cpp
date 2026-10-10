#!/usr/bin/env python3
"""Observe actual Value::from(DateTime<Utc>) across signed fractional boundaries."""
import argparse
import json
import subprocess
from pathlib import Path
from compare_values import observe


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--rust', required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    subprocess.run(['python3', str(root / 'scripts/verify-upstream.py')], check=True)
    ticks = [-1000000001, -1000000, -1001, -1000, -999, -1, 0, 1, 999, 1000,
             1001, 999999, 1000000, 1000000001]
    bounds = {'seconds': [-8000000000000, 8000000000000],
              'milliseconds': [-8000000000000000, 8000000000000000],
              'microseconds': [-8000000000000000000, 8000000000000000000],
              'nanoseconds': [-9223372036854775808, 9223372036854775807]}
    cases = [dict(unit=unit, ticks=str(n)) for unit, extra in bounds.items() for n in ticks + extra]
    outputs = observe(args.rust, cases)
    corpus = dict(baseline=json.loads((root / 'upstream.lock.json').read_text())['commit'],
                  note='Original UTC conversion; all original chrono instants are representable.',
                  cases=[dict(input=i, expected=o) for i, o in zip(cases, outputs)])
    Path(__file__).with_name('chrono_value_cases.json').write_text(json.dumps(corpus, indent=2) + '\n')
    print(f'Recorded {len(cases)} original UTC Value conversion observations')


if __name__ == '__main__':
    main()
