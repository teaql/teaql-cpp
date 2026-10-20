#!/usr/bin/env python3
"""Replay retained original bootstrap/typed Q/E observations for one configured domain."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path

from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]
CASES = ('equivalent_integer', 'equivalent_string_alias', 'field_parameter',
         'numeric_alias', 'string_alias', 'unsigned_recipe')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--case', choices=CASES, required=True)
    parser.add_argument('--cpp', required=True)
    parser.add_argument('--rust')
    args = parser.parse_args()
    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    case = ROOT / 'tests/generator/parity/type_configuration_contracts' / args.case
    original = None
    for language in ('rust', 'cpp'):
        output = case / language
        provenance = json.loads((output / 'PROVENANCE.json').read_text())
        if (provenance['runtime_commit'] != baseline or provenance['hand_edited']
                or provenance['model_sha256'] != hashlib.sha256((case / 'model.xml').read_bytes()).hexdigest()):
            raise RuntimeError('Configured-type fixture provenance changed')
        actual = {p.relative_to(output).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
                  for p in output.rglob('*') if p.is_file() and p.name != 'PROVENANCE.json'}
        if actual != provenance['files']:
            raise RuntimeError('Configured-type complete generated file set or bytes changed')
        if language == 'rust':
            original = provenance
    with gzip.open(Path(__file__).with_name(f'type_configuration_execution_{args.case}_5_0_6_cases.json.gz'), 'rt') as file:
        corpus = json.load(file)
    if corpus['baseline'] != baseline or corpus['case'] != args.case or not same(original, corpus['original_provenance']):
        raise RuntimeError('Configured-type corpus belongs to another source/model')
    inputs = [row['input'] for row in corpus['cases']]
    expected = [row['expected'] for row in corpus['cases']]
    if args.rust and not same(observe(args.rust, inputs), expected):
        raise RuntimeError('Configured-type corpus differs from live unchanged original')
    actual = observe(args.cpp, inputs)
    differences = [(i, a, b) for i, (a, b) in enumerate(zip(expected, actual)) if not same(a, b)]
    for i, source, native in differences[:5]:
        print(f'Case {i}: {inputs[i]}\nRust: {source}\nC++: {native}')
    print(f'Configured type {args.case}: {len(inputs)-len(differences)}/{len(inputs)} exact observations')
    return bool(differences)


if __name__ == '__main__':
    raise SystemExit(main())
