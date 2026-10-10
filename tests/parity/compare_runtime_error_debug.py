#!/usr/bin/env python3
"""Compare raw structural diagnostics with actual pinned Rust calls."""
import argparse
import gzip
import json
import subprocess
from pathlib import Path
from compare_values import observe, same


def target_inputs(inputs, bits):
    # Match only the pinned fixture's explicit u64 -> usize LocationSegment cast.
    def check(value):
        value = dict(value)
        value['path'] = [part & ((1 << bits) - 1) if type(part) is int else part
                         for part in value['path']]
        return value
    results = []
    for value in inputs:
        if value['op'] == 'check':
            value = check(value)
        elif value['op'] == 'error' and value['kind'] == 5:
            value = dict(value, checks=[check(item) for item in value['checks']])
        results.append(value)
    return results


def main(corpus_name='runtime_error_debug_cases.json.gz', label='Runtime error and status diagnostics'):
    parser = argparse.ArgumentParser()
    parser.add_argument('--cpp', required=True)
    parser.add_argument('--rust')
    parser.add_argument('--cpp-emulator', default='')
    args = parser.parse_args()
    prefix = args.cpp_emulator.split(';') if args.cpp_emulator else []
    bits = int(subprocess.check_output(prefix + [args.cpp, '--pointer-bits'], text=True))
    if bits not in (32, 64):
        raise RuntimeError('Unsupported runtime-check probe pointer width')
    root = Path(__file__).resolve().parents[2]
    path = Path(__file__).with_name(corpus_name)
    data = json.load(gzip.open(path, 'rt')) if path.suffix == '.gz' else json.loads(path.read_text())
    if data['baseline'] != json.loads((root / 'upstream.lock.json').read_text())['commit']:
        raise RuntimeError('Runtime diagnostic observations use a different source baseline')
    inputs = [case['input'] for case in data['cases']]
    key = 'expected_32' if bits == 32 else 'expected'
    expected = [case.get(key, case['expected']) for case in data['cases']]
    if args.rust:
        subprocess.run(['python3', str(root / 'scripts/verify_upstream.py')], check=True)
        if not same(expected, observe(args.rust, target_inputs(inputs, bits))):
            raise RuntimeError('Committed target-width observations differ from live Rust methods')
    actual = observe(args.cpp, inputs, prefix)
    failures = [(index, inp, exp, act) for index, (inp, exp, act) in
                enumerate(zip(inputs, expected, actual)) if not same(exp, act)]
    for index, inp, exp, act in failures[:5]:
        print(f'Case {index}: {inp}\nRust: {exp}\nC++: {act}')
    print(f'{label} ({bits}-bit usize): {len(inputs)-len(failures)}/{len(inputs)} observations match')
    return bool(failures)

if __name__ == '__main__':
    raise SystemExit(main())
