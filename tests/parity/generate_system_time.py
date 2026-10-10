#!/usr/bin/env python3
"""Record original Unix SystemTime boundaries, arithmetic and metadata Debug."""
import argparse
import gzip
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
    seconds = [-(1 << 63), -10000000000, -9223372037, -1, 0, 1,
               9223372036, 9223372037, 10000000000, (1 << 63) - 1]
    nanos = [0, 1, 145224192, 854775807, 999999999]
    points = [dict(seconds=str(s), nanos=n) for s in seconds for n in nanos]
    deltas = [0, 1, (1 << 63) - 1, 1 << 63, (1 << 64) - 1]
    cases = [dict(start=p, end=points[(index * 7 + 1) % len(points)], delta=str(d))
             for index, p in enumerate(points) for d in deltas]
    cases.extend(dict(start=a, end=b, delta='0') for a in (points[0], points[-1])
                 for b in (points[0], points[-1]))
    outputs = observe(args.rust, cases)
    corpus = dict(baseline=json.loads((root / 'upstream.lock.json').read_text())['commit'],
                  note='Actual Rust Unix SystemTime at full signed-second boundaries; metadata derives and checked duration arithmetic. Chrono narrowing is independently observed through i64 conversion of original as_nanos.',
                  cases=[dict(input=i, expected=o) for i, o in zip(cases, outputs)])
    encoded = (json.dumps(corpus, separators=(',', ':')) + '\n').encode()
    Path(__file__).with_name('system_time_cases.json.gz').write_bytes(gzip.compress(encoded, mtime=0))
    print(f'Recorded {len(cases)} original full-range SystemTime inputs')


if __name__ == '__main__':
    main()
