#!/usr/bin/env python3
"""Replay original observations with the declared raw SQL identifier adaptation."""
import argparse
import copy
import gzip
import hashlib
import json
from pathlib import Path
from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]


def cpp_observation(original):
    value = copy.deepcopy(original)
    if value.get('query', {}).get('filter') is not None:
        # Exact frozen Line descriptor spelling, not arbitrary identifier rewriting.
        value['query']['filter'] = value['query']['filter'].replace(
            'column_name: "\\\"order\\\""', 'column_name: "order"')
    return value


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', required=True)
    parser.add_argument('--rust')
    args = parser.parse_args()
    with gzip.open(Path(__file__).with_name('generated_dynamic_json_5_0_6_cases.json.gz'), 'rt') as source:
        corpus = json.load(source)
    lock = json.loads((ROOT/'upstream.lock.json').read_text())
    if corpus['baseline'] != lock['commit']:
        raise SystemExit('Dynamic JSON corpus belongs to a different source baseline')
    producer = json.loads((ROOT/'tests/generator/parity/dynamic_json_rust/PROVENANCE.json').read_text())
    if corpus['producer'] != producer:
        raise SystemExit('Dynamic JSON producer provenance differs')
    for name, expected in corpus['fixtures'].items():
        if hashlib.sha256((ROOT/'tests/support'/name).read_bytes()).hexdigest() != expected:
            raise SystemExit(f'Dynamic JSON database fixture changed: {name}')
    inputs = [case['input'] for case in corpus['cases']]
    original = [case['expected'] for case in corpus['cases']]
    if args.rust:
        if not same(original, observe(args.rust, inputs)):
            raise SystemExit('Live original observations differ from the unmodified corpus')
        failure = json.loads((ROOT/'docs/parity/dynamic_json_schema_source_failure_5_0_6.json').read_text())
        if failure['baseline'] != lock['commit'] or failure['model_sha256'] != producer['model_sha256']:
            raise SystemExit('Source schema failure provenance differs')
        if observe(args.rust, [failure['input']])[0] != failure['expected']:
            raise SystemExit('Source schema failure changed')
    actual = observe(args.cpp, inputs)
    mismatches = [(index, case, value) for index, (case, value) in enumerate(zip(original, actual))
                  if not same(cpp_observation(case), value)]
    if mismatches:
        for index, case, value in mismatches[:3]:
            print(f'Mismatch {index}: Rust {case}\nC++ {value}')
        raise SystemExit(f'{len(mismatches)}/{len(inputs)} dynamic JSON observations differ')
    print(f'Generated dynamic JSON: {len(inputs)}/{len(inputs)} observations match')


if __name__ == '__main__':
    main()
