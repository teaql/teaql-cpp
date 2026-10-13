#!/usr/bin/env python3
"""Compare actual public generated root/constant initialization against Rust."""
import argparse
import gzip
import itertools
import json
import os
from pathlib import Path
import subprocess
from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]


def cases():
    for state, rounds in itertools.product(
            ['empty', 'existing', 'corrupt', 'missing', 'deleted', 'foreign_root',
             'extra', 'null_fields', 'root_mismatch'], [1, 2, 3]):
        yield dict(state=state, rounds=rounds, fault='primary', failures=0)
    for state, rounds, fault, failures in itertools.product(
            ['empty', 'corrupt', 'missing'], [1, 2], ['primary', 'secondary', 'ordinary'], [1, 4, 5, 100]):
        yield dict(state=state, rounds=rounds, fault=fault, failures=failures)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rust', type=Path, required=True)
    parser.add_argument('--cpp', type=Path, required=True)
    args = parser.parse_args()
    os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
    subprocess.run(['python3', str(ROOT/'scripts/verify_upstream.py')], check=True)
    inputs = list(cases())
    try:
        expected = observe(str(args.rust.resolve()), inputs)
        actual = observe(str(args.cpp.resolve()), inputs)
    except subprocess.CalledProcessError as error:
        raise SystemExit(f'Bootstrap probe failed: {error.stderr[-3000:]}') from error
    failures = [(i, case, a, b) for i, (case, a, b) in enumerate(zip(inputs, expected, actual)) if not same(a, b)]
    for i, case, a, b in failures[:3]:
        print(f'Mismatch {i}: {case}\nRust: {a}\nC++: {b}')
    if failures:
        raise SystemExit(f'{len(failures)} / {len(inputs)} bootstrap observations differ')
    lock = json.loads((ROOT/'upstream.lock.json').read_text())
    producer = json.loads((ROOT/'tests/generator/parity/candidates_rust/PROVENANCE.json').read_text())
    corpus = dict(baseline=lock['commit'], version=lock['version'], producer=producer,
                  note='Actual unchanged original generated module/service runtime public ensure_schema. Root/constant creation, reconciliation, no-op rounds, ID floors, active-root conflicts, exact errors, soft-deleted/scoped rows, extra custom rows, malformed values and real SQLite trigger conflict retry. No error or database normalization.',
                  cases=[dict(input=case, expected=value) for case, value in zip(inputs, expected)])
    Path(__file__).with_name('generated_bootstrap_5_0_6_cases.json.gz').write_bytes(gzip.compress((json.dumps(corpus, indent=2)+'\n').encode(), mtime=0))
    print(f'Compared {len(inputs)} actual original/C++ bootstrap observations')


if __name__ == '__main__':
    main()
