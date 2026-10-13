#!/usr/bin/env python3
"""Compare original public scalar/topology constant bootstrap against C++."""
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
    for fixture, state, rounds in itertools.product(
            ['numeric', 'catalog'], ['empty', 'existing', 'corrupt', 'missing', 'deleted', 'extra'], [1, 2, 3]):
        yield dict(fixture=fixture, state=state, rounds=rounds)


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
        raise SystemExit(f'Constant variant probe failed: {error.stderr[-3000:]}') from error
    failures = [(i, case, a, b) for i, (case, a, b) in enumerate(zip(inputs, expected, actual)) if not same(a, b)]
    for i, case, a, b in failures[:3]:
        print(f'Mismatch {i}: {case}\nRust: {a}\nC++: {b}')
    if failures:
        raise SystemExit(f'{len(failures)} / {len(inputs)} constant variant observations differ')
    lock = json.loads((ROOT/'upstream.lock.json').read_text())
    producers = {name: json.loads((ROOT/f'tests/generator/parity/constants_{name}_rust/PROVENANCE.json').read_text()) for name in ['numeric', 'catalog']}
    corpus = dict(baseline=lock['commit'], version=lock['version'], producers=producers,
                  note='Complete unchanged original generated crates; public ensure_schema on the authentic provider-created schema followed by controlled stored-state changes and a fresh Context. Raw SQLite values/errors/versions/ID floors/root identity; no normalization. Scalar and optional families, multiple/dependent constants, a late root and a standalone constant catalog alongside its ordinary root, both using ID 1.',
                  cases=[dict(input=case, expected=value) for case, value in zip(inputs, expected)])
    Path(__file__).with_name('generated_constant_variants_5_0_6_cases.json.gz').write_bytes(gzip.compress((json.dumps(corpus, indent=2)+'\n').encode(), mtime=0))
    print(f'Compared {len(inputs)} actual original/C++ constant variant observations')


if __name__ == '__main__':
    main()
