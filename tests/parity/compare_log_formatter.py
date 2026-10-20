#!/usr/bin/env python3
"""Compare actual pinned formatter output (only wall-clock prefixes are removed)."""
import argparse
import json
import gzip
import os
import subprocess
from pathlib import Path
from compare_values import observe, same


def count_fits(input_value, bits):
    kind = input_value.get('kind')
    fields = ('count', 'updated') if kind == 'data_seeded' else (
        ('count',) if kind in ('schema_created', 'schema_verified') else ())
    return all(0 <= int(input_value[field]) < 1 << bits for field in fields)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--cpp', required=True, help='CMake build directory')
    parser.add_argument('--rust', help='Cargo target directory containing debug binaries')
    parser.add_argument('--cpp-emulator', default='')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    corpus = json.loads(gzip.decompress(Path(__file__).with_name('log_formatter_baseline_5_0_6_cases.json.gz').read_bytes()))
    if corpus['baseline'] != json.loads((root / 'upstream.lock.json').read_text())['commit']:
        raise RuntimeError('Formatter observations belong to a different baseline')
    if args.rust:
        subprocess.run(['python3', str(root / 'scripts/verify_upstream.py')], check=True, cwd=root)
    probes = {'audit': ('teaql_audit_event_probe', 'audit_event_oracle'),
              'sql': ('teaql_runtime_log_probe', 'runtime_log_oracle'),
              'config': ('teaql_log_formatter_probe', 'log_formatter_oracle')}
    failed = 0
    rejected = 0
    emulator = args.cpp_emulator.split(';') if args.cpp_emulator else []
    for group, (cpp, rust) in probes.items():
        cases = [c for c in corpus['cases'] if c['probe'] == group]
        inputs = [c['input'] for c in cases]
        expected = [c['expected'] for c in cases]
        if args.rust and not same(expected, observe(str(Path(args.rust) / rust), inputs)):
            raise RuntimeError(f'{group}: stored observations differ from actual Rust')
        prefix = os.environ.get('TEAQL_PARITY_CPP_PREFIX', 'teaql_')
        executable = str(Path(args.cpp) / cpp.replace('teaql_', prefix, 1))
        if group == 'audit':
            bits = int(subprocess.check_output(emulator + [executable, '--pointer-bits'], text=True))
            if bits not in (32, 64):
                raise RuntimeError('Unsupported audit probe size_t width')
            acceptance = [dict(accepted=count_fits(value, bits)) for value in inputs]
            actual_acceptance = observe(executable, inputs, emulator, ['--validate-counts'])
            if not same(acceptance, actual_acceptance):
                raise RuntimeError('Audit probe count parser differs from its target width')
            if bits == 32 and args.rust:
                rust_acceptance = observe(str(Path(args.rust) / rust), inputs,
                                          arguments=['--validate-counts-32'])
                if not same(acceptance, rust_acceptance):
                    raise RuntimeError('Target count rejection differs from actual Rust u32 parsing')
            valid = [i for i, value in enumerate(acceptance) if value['accepted']]
            rejected += len(inputs) - len(valid)
            inputs = [inputs[i] for i in valid]
            expected = [expected[i] for i in valid]
        actual = observe(executable, inputs, emulator)
        differences = [(n, a, b) for n, (a, b) in enumerate(zip(expected, actual)) if not same(a, b)]
        for n, a, b in differences[:5]:
            print(f'{group} case {n}: {inputs[n]}\nRust: {a}\nC++: {b}')
        failed += len(differences)
    total = len(corpus['cases'])
    print(f'Log formatter parity: {total - rejected - failed}/{total - rejected} outputs match; '
          f'{rejected} target-width inputs reject before formatting')
    return bool(failed)


if __name__ == '__main__':
    raise SystemExit(main())
