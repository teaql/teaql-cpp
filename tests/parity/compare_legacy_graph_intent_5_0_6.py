#!/usr/bin/env python3
"""Compare complete valid-intent legacy graph observations, including trace IDs."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path

from compare_values import observe, same


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', required=True)
    parser.add_argument('--cpp-emulator', default='')
    parser.add_argument('--record', type=Path)
    parser.add_argument('--original', type=Path)
    args = parser.parse_args()
    if args.record and args.record.exists():
        parser.error('Use a fresh receipt')
    root = Path(__file__).resolve().parents[2]
    source = args.original or root / 'docs/parity/legacy_graph_intent_original_5_0_6.json.gz'
    with gzip.open(source, 'rt') as stream:
        original = json.load(stream)
    if not original['complete'] or not original['provenance']['all_original_bodies_preserved']:
        raise RuntimeError('Incomplete original runtime evidence')
    inputs = [dict(case, bound_intent='verify legacy graph trace') for case in original['inputs']]
    actual = observe(args.cpp, inputs, args.cpp_emulator.split(';') if args.cpp_emulator else ())
    expected = original['observations']
    mismatches = [i for i, (a, b) in enumerate(zip(expected, actual)) if not same(a, b)]
    receipt = dict(complete=not mismatches, count=len(inputs), mismatches=mismatches,
                   original_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
                   executable_sha256=hashlib.sha256(Path(args.cpp).read_bytes()).hexdigest(),
                   inputs=inputs, expected=expected, actual=actual)
    if args.record:
        with gzip.open(args.record, 'wt') as stream:
            json.dump(receipt, stream, indent=2)
    print(f'Legacy graph valid-intent parity: {len(inputs) - len(mismatches)}/{len(inputs)}')
    for index in mismatches[:3]:
        print(json.dumps(dict(index=index, expected=expected[index], actual=actual[index])))
    return bool(mismatches)


if __name__ == '__main__':
    raise SystemExit(main())
