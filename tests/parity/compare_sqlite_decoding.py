#!/usr/bin/env python3
"""Compare public SQLite decoding against source with matching arithmetic checks."""
import argparse
import gzip
import json
from pathlib import Path
import subprocess
import sys

from compare_values import observe, same


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', required=True)
    parser.add_argument('--rust')
    parser.add_argument('--cpp-emulator', default='')
    args = parser.parse_args()
    emulator = args.cpp_emulator.split(';') if args.cpp_emulator else []
    profile = subprocess.run([*emulator, args.cpp, '--profile'], input='', text=True,
                             capture_output=True, check=True).stdout.strip()
    if profile not in ('checked', 'unchecked'):
        raise RuntimeError(f'Unknown decoding probe arithmetic profile: {profile}')
    qualifier = '_unchecked' if profile == 'unchecked' else ''
    path = Path(__file__).with_name(f'sqlite_decoding{qualifier}_5_0_6_cases.json.gz')
    with gzip.open(path, 'rt', encoding='utf-8') as source:
        corpus = json.load(source)
    root = Path(__file__).resolve().parents[2]
    lock = json.loads((root / 'upstream.lock.json').read_text())
    if (corpus['baseline'] != lock['commit'] or type(corpus.get('checked_arithmetic')) is not bool
            or corpus['checked_arithmetic'] != (profile == 'checked')):
        raise RuntimeError('Decoding corpus baseline/profile differs from the compiled probe')
    cases = corpus['cases']
    if type(cases) is not list or not cases:
        raise RuntimeError('Comparison corpus cases must be a nonempty list')
    inputs = [case['input'] for case in cases]
    expected = [case['expected'] for case in cases]
    if args.rust:
        subprocess.run([sys.executable, str(root / 'scripts/verify_upstream.py')], check=True)
        if not same(expected, observe(args.rust, inputs)):
            raise RuntimeError('Decoding corpus differs from actual original source')
    actual = observe(args.cpp, inputs, emulator=emulator)
    failures = [i for i, (a, b) in enumerate(zip(expected, actual)) if not same(a, b)]
    for index in failures[:3]:
        print(index, inputs[index], expected[index], actual[index])
    print(f'SQLite decoding ({profile}): {len(inputs)-len(failures)}/{len(inputs)} match')
    return bool(failures)


if __name__ == '__main__':
    raise SystemExit(main())
