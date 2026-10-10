#!/usr/bin/env python3
"""Compare complete pointer conversions against the actual matching Rust word width."""
import argparse
import gzip
import json
import subprocess
from pathlib import Path

from compare_values import observe, same


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--cpp', required=True)
    parser.add_argument('--cpp-emulator', default='')
    parser.add_argument('--rust')
    parser.add_argument('--rust-emulator', default='')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    with gzip.open(Path(__file__).with_name('decimal_pointer_scope_cases.json.gz'), 'rt') as source:
        corpus = json.load(source)
    if corpus['baseline'] != json.loads((root / 'upstream.lock.json').read_text())['commit']:
        raise RuntimeError('Decimal pointer observations belong to a different source baseline')
    emulator = args.cpp_emulator.split(';') if args.cpp_emulator else []
    bits = subprocess.check_output(emulator + [args.cpp, '--pointer-bits'], text=True).strip()
    if bits not in ('32', '64'):
        raise RuntimeError('Unsupported native pointer width')
    cases = corpus['cases']
    inputs = [case['input'] for case in cases]
    expected = [case['expected'][bits] for case in cases]
    if args.rust:
        subprocess.run(['python3', str(root / 'scripts/verify_upstream.py')], check=True)
        rust_emulator = args.rust_emulator.split(';') if args.rust_emulator else []
        rust_bits = subprocess.check_output(rust_emulator + [args.rust, '--pointer-bits'], text=True).strip()
        if rust_bits != bits:
            raise RuntimeError('Live original oracle does not match the C++ pointer width')
        if not same(expected, observe(args.rust, inputs, rust_emulator)):
            raise RuntimeError('Stored pointer conversions differ from live original Rust')
    actual = observe(args.cpp, inputs, emulator)
    failed = [index for index, (source, native) in enumerate(zip(expected, actual)) if not same(source, native)]
    for index in failed[:10]:
        print(f'Case {index}: {inputs[index]}\nRust: {expected[index]}\nC++: {actual[index]}')
    print(f'Decimal pointer conversions ({bits}-bit): {len(cases) - len(failed)}/{len(cases)} match')
    return bool(failed)


if __name__ == '__main__':
    raise SystemExit(main())
