#!/usr/bin/env python3
"""Compare actual pinned formatter output (only wall-clock prefixes are removed)."""
import argparse
import json
import subprocess
from pathlib import Path
from compare_values import observe, same


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--cpp', required=True, help='CMake build directory')
    parser.add_argument('--rust', help='Cargo target directory containing debug binaries')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    corpus = json.loads(Path(__file__).with_name('log_formatter_cases.json').read_text())
    if corpus['baseline'] != json.loads((root / 'upstream.lock.json').read_text())['commit']:
        raise RuntimeError('Formatter observations belong to a different baseline')
    if args.rust:
        subprocess.run(['python3', str(root / 'scripts/verify-upstream.py')], check=True, cwd=root)
    probes = {'audit': ('teaql_audit_event_probe', 'audit_event_oracle'),
              'sql': ('teaql_runtime_log_probe', 'runtime_log_oracle'),
              'config': ('teaql_log_formatter_probe', 'log_formatter_oracle')}
    failed = 0
    for group, (cpp, rust) in probes.items():
        cases = [c for c in corpus['cases'] if c['probe'] == group]
        inputs = [c['input'] for c in cases]
        expected = [c['expected'] for c in cases]
        if args.rust and not same(expected, observe(str(Path(args.rust) / rust), inputs)):
            raise RuntimeError(f'{group}: stored observations differ from actual Rust')
        actual = observe(str(Path(args.cpp) / cpp), inputs)
        differences = [(n, a, b) for n, (a, b) in enumerate(zip(expected, actual)) if not same(a, b)]
        for n, a, b in differences[:5]:
            print(f'{group} case {n}: {inputs[n]}\nRust: {a}\nC++: {b}')
        failed += len(differences)
    print(f'Log formatter parity: {len(corpus["cases"]) - failed}/{len(corpus["cases"])} observations match')
    return bool(failed)


if __name__ == '__main__':
    raise SystemExit(main())
