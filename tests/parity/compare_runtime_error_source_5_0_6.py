#!/usr/bin/env python3
"""Compare direct error/trait observations with the immutable actual-source corpus."""
import argparse
import gzip
import json
from pathlib import Path

from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', required=True)
    parser.add_argument('--cpp-emulator', default='')
    args = parser.parse_args()
    data = json.loads(gzip.decompress(Path(__file__).with_name(
        'runtime_error_extended_5_0_6_cases.json.gz').read_bytes()))
    if data['baseline'] != json.loads((ROOT / 'upstream.lock.json').read_text())['commit']:
        raise RuntimeError('Runtime error source corpus uses another baseline')
    prefix = args.cpp_emulator.split(';') if args.cpp_emulator else []
    actual = observe(args.cpp, [case['input'] for case in data['cases']], prefix)
    expected = [case['expected'] for case in data['cases']]
    if not same(actual, expected):
        raise SystemExit('Direct error source observations differ')
    print(f"Direct error source: {len(expected)}/{len(expected)} exact observations")


if __name__ == '__main__':
    main()
