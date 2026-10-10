#!/usr/bin/env python3
"""Observe every existing key input using original methods and 32-bit usize preparation."""
import argparse
import gzip
import json
import subprocess
from pathlib import Path
from compare_values import observe, same


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--rust32', required=True)
    parser.add_argument('--rust64', required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    subprocess.run(['python3', str(root / 'scripts/verify-upstream.py')], check=True)
    source = json.loads(Path(__file__).with_name('query_keys_cases.json').read_text())
    inputs = [case['input'] for case in source['cases']]
    original = observe(args.rust64, inputs)
    if not same(original, [case['expected'] for case in source['cases']]):
        raise RuntimeError('Default host oracle changed existing original observations')
    outputs = observe(args.rust32, inputs)
    corpus = dict(baseline=source['baseline'],
                  note='All original inputs retained. Host Rust prepares 32-bit usize values before calling unchanged pinned key methods; this is not a Rust MIPS execution claim.',
                  cases=[dict(input=i, expected=o) for i, o in zip(inputs, outputs)])
    if not same(inputs, [case['input'] for case in corpus['cases']]):
        raise RuntimeError('Target-width recording changed original inputs')
    payload = (json.dumps(corpus, separators=(',', ':')) + '\n').encode()
    Path(__file__).with_name('query_keys_cases_32.json.gz').write_bytes(gzip.compress(payload, mtime=0))
    print(f'Recorded all {len(inputs)} unchanged key inputs for 32-bit usize; 64-bit corpus unchanged')


if __name__ == '__main__':
    main()
