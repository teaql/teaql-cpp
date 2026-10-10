#!/usr/bin/env python3
"""Compare audit output and explicitly verify target-width count rejection."""
import argparse
import json
import subprocess
from pathlib import Path

from compare_log_formatter import count_fits
from compare_values import observe, same


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--cpp', required=True)
    parser.add_argument('--rust')
    parser.add_argument('--cpp-emulator', default='')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    corpus = json.loads(Path(__file__).with_name('audit_event_cases.json').read_text())
    baseline = json.loads((root / 'upstream.lock.json').read_text())['commit']
    if corpus['baseline'] != baseline:
        raise RuntimeError('Audit observations belong to a different source baseline')
    inputs = [case['input'] for case in corpus['cases']]
    expected = [case['expected'] for case in corpus['cases']]
    emulator = args.cpp_emulator.split(';') if args.cpp_emulator else []
    if args.rust:
        subprocess.run(['python3', str(root / 'scripts/verify-upstream.py')], check=True)
        if not same(expected, observe(args.rust, inputs)):
            raise RuntimeError('Audit observations differ from live original Rust')
    bits = int(subprocess.check_output(emulator + [args.cpp, '--pointer-bits'], text=True))
    if bits not in (32, 64):
        raise RuntimeError('Unsupported audit probe size_t width')
    acceptance = [dict(accepted=count_fits(value, bits)) for value in inputs]
    counted = [index for index, value in enumerate(inputs)
               if value.get('kind') in ('schema_created', 'schema_verified', 'data_seeded')]
    count_inputs = [inputs[index] for index in counted]
    count_acceptance = [acceptance[index] for index in counted]
    actual_acceptance = observe(args.cpp, count_inputs, emulator, ['--validate-counts'])
    if not same(count_acceptance, actual_acceptance):
        raise RuntimeError('Audit count parser differs from its target width')
    if bits == 32 and args.rust:
        rust_acceptance = observe(args.rust, count_inputs, arguments=['--validate-counts-32'])
        if not same(count_acceptance, rust_acceptance):
            raise RuntimeError('Audit count rejection differs from actual Rust u32 parsing')
    valid = [index for index, value in enumerate(acceptance) if value['accepted']]
    actual = observe(args.cpp, [inputs[index] for index in valid], emulator)
    failures = [(index, expected[index], value)
                for index, value in zip(valid, actual) if not same(expected[index], value)]
    for index, source, native in failures[:10]:
        print(f'Case {index}: {inputs[index]}\nRust: {source}\nC++: {native}')
    print(f'Audit events and privacy parity: {len(valid) - len(failures)}/{len(valid)} '
          f'outputs match; {len(inputs) - len(valid)} target-width inputs reject before execution')
    return bool(failures)

if __name__ == "__main__":
    raise SystemExit(main())
