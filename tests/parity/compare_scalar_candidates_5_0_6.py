#!/usr/bin/env python3
"""Replay exact scalar candidate SQLite observations using retained generated domains."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path

from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]
CASES = ('text', 'integer', 'nullable_integer', 'number', 'double', 'date', 'timestamp', 'boolean')


def check_fixture(case, language, baseline, generator):
    output = case / language
    provenance = json.loads((output / 'PROVENANCE.json').read_text())
    if (provenance['runtime_commit'] != baseline or provenance['generator_commit'] != generator
            or provenance['hand_edited']
            or provenance['model'] != (case / 'model.xml').relative_to(ROOT).as_posix()
            or provenance['model_sha256'] != hashlib.sha256((case / 'model.xml').read_bytes()).hexdigest()):
        raise RuntimeError(f'Candidate {language} fixture provenance differs: {case.name}')
    actual = {p.relative_to(output).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
              for p in output.rglob('*') if p.is_file() and p.name != 'PROVENANCE.json'}
    if actual != provenance['files']:
        raise RuntimeError(f'Candidate {language} generated file set or bytes differ: {case.name}')
    return provenance


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--case', required=True, choices=CASES)
    parser.add_argument('--cpp', required=True)
    parser.add_argument('--rust')
    parser.add_argument('--cpp-emulator', default='')
    args = parser.parse_args()
    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    generator = json.loads((ROOT / 'docs/cpp_generator_parity_ledger.json').read_text())['generator_commit']
    case = ROOT / 'tests/generator/parity/scalar_candidate_contracts' / args.case
    original = check_fixture(case, 'rust', baseline, generator)
    check_fixture(case, 'cpp', baseline, generator)
    with gzip.open(Path(__file__).with_name(f'scalar_candidate_execution_{args.case}_5_0_6_cases.json.gz'),
                   'rt', encoding='utf-8') as file:
        corpus = json.load(file)
    if corpus['baseline'] != baseline or corpus['case'] != args.case or not same(corpus['original_provenance'], original):
        raise RuntimeError('Scalar candidate corpus belongs to another source/model')
    rows = corpus['cases']
    inputs = [row['input'] for row in rows]
    expected = [row['expected'] for row in rows]
    if args.rust and not same(observe(args.rust, inputs), expected):
        raise RuntimeError('Scalar candidate corpus differs from live original Rust')
    actual = observe(args.cpp, inputs, args.cpp_emulator.split(';') if args.cpp_emulator else ())
    failures = [(i, source, result) for i, (source, result) in enumerate(zip(expected, actual))
                if not same(source, result)]
    for i, source, result in failures[:10]:
        print(f'Case {i}: {inputs[i]}\nRust: {source}\nC++: {result}')
    print(f'Scalar candidate {args.case}: {len(rows) - len(failures)}/{len(rows)} exact observations')
    return bool(failures)


if __name__ == '__main__':
    raise SystemExit(main())
