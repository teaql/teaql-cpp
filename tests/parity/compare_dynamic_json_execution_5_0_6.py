#!/usr/bin/env python3
"""Replay the original one/stream corpus with its exact declared column adapter."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
from compare_values import observe, same
from compare_generated_dynamic_json import cpp_observation

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', required=True); parser.add_argument('--rust')
    args = parser.parse_args()
    with gzip.open(Path(__file__).with_name('dynamic_json_execution_5_0_6_cases.json.gz'), 'rt') as f:
        corpus = json.load(f)
    assert corpus['baseline'] == json.load(open(ROOT / 'upstream.lock.json'))['commit']
    assert corpus['producer'] == json.load(open(ROOT / 'tests/generator/parity/dynamic_json_rust/PROVENANCE.json'))
    for name, digest in corpus['fixtures'].items():
        assert hashlib.sha256((ROOT / 'tests/support' / name).read_bytes()).hexdigest() == digest
    inputs = [x['input'] for x in corpus['cases']]; expected = [x['expected'] for x in corpus['cases']]
    if args.rust:
        assert same(expected, observe(args.rust, inputs)), 'Live original observations differ'
    actual = observe(args.cpp, inputs)
    differences = [i for i, (x, y) in enumerate(zip(expected, actual)) if not same(cpp_observation(x), y)]
    if differences:
        raise SystemExit(f'{len(differences)}/{len(inputs)} generated JSON one/stream differences')
    print(f'Dynamic JSON execution: {len(inputs)}/{len(inputs)} original observations match')


if __name__ == '__main__':
    main()
