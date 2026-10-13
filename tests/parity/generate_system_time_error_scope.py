#!/usr/bin/env python3
"""Record actual source reversed spans, Clone, Debug, Display and Error defaults."""
import argparse
import gzip
import itertools
import json
import subprocess
from pathlib import Path
from compare_values import observe


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--rust', required=True)
    parser.add_argument('--output', type=Path,
                        help='Write a separately versioned corpus without replacing historical evidence')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    subprocess.run(['python3', str(root / 'scripts/verify_upstream.py')], check=True)
    points = [dict(seconds=str(s), nanos=n) for s, n in itertools.product(
        [-(1 << 63), -10000000000, -1, 0, 1, 10000000000, (1 << 63) - 1],
        [0, 1, 999, 1000, 1500, 2500, 999999, 1000000, 125000000, 135000000,
         500000000, 999999999])]
    cases = [dict(start=p, end=q) for p, q in itertools.product(points, points)]
    outputs = observe(args.rust, cases)
    corpus = dict(baseline=json.loads((root / 'upstream.lock.json').read_text())['commit'],
        note='Actual Rust SystemTime Result/Error, Clone, Duration Debug and Error/Display defaults; no copied source arithmetic.',
        cases=[dict(input=i, expected=o) for i, o in zip(cases, outputs)])
    destination = args.output or Path(__file__).with_name('system_time_error_scope_cases.json.gz')
    with gzip.GzipFile(filename=str(destination),
                       mode='wb', mtime=0) as output:
        output.write((json.dumps(corpus, ensure_ascii=True, separators=(',', ':')) + '\n').encode())
    print(f'Recorded {len(cases)} original time-error inputs')


if __name__ == '__main__':
    main()
