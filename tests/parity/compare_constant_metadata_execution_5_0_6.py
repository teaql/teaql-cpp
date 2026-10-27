#!/usr/bin/env python3
"""Replay unchanged original bootstrap/Q/E outcomes for metadata configurations."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path

from compare_values import observe, same
from configuration_cli_cases_5_0_6 import bootstrap_inputs
from expression_cli_cases import require_cases

ROOT = Path(__file__).resolve().parents[2]
CASES = ('missing_identifier', 'unknown_identifier', 'no_candidates', 'empty_identifier_value',
         'missing_identifier_value', 'missing_seed_field', 'comma_identifier_value', 'unicode_identifier_value')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--case', choices=CASES, required=True)
    parser.add_argument('--cpp', required=True)
    parser.add_argument('--rust')
    args = parser.parse_args()
    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    fixture = ROOT / 'tests/generator/parity/constant_metadata_contracts' / args.case
    original = None
    for language in ('rust', 'cpp'):
        output = fixture / language
        provenance = json.loads((output / 'PROVENANCE.json').read_text())
        if (provenance['runtime_commit'] != baseline or provenance['hand_edited']
                or provenance['model_sha256'] != hashlib.sha256((fixture / 'model.xml').read_bytes()).hexdigest()):
            raise RuntimeError('Metadata execution source/model changed')
        actual = {p.relative_to(output).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
                  for p in output.rglob('*') if p.is_file() and p.name != 'PROVENANCE.json'}
        if actual != provenance['files']:
            raise RuntimeError('Complete metadata execution fixture changed')
        if language == 'rust':
            original = provenance
    path = Path(__file__).with_name(f'constant_metadata_execution_{args.case}_5_0_6_cases.json.gz')
    with gzip.open(path, 'rt') as file:
        corpus = json.load(file)
    if (corpus['baseline'] != baseline or corpus['case'] != args.case
            or not same(original, corpus['original_provenance'])):
        raise RuntimeError('Metadata execution corpus belongs to another source/model')
    rows = require_cases(corpus, 96, bootstrap_inputs())
    inputs = [row['input'] for row in rows]
    expected = [row['expected'] for row in rows]
    if args.rust and not same(observe(args.rust, inputs), expected):
        raise RuntimeError('Metadata execution corpus differs from live original')
    actual = observe(args.cpp, inputs)
    gaps = [(i, a, b) for i, (a, b) in enumerate(zip(expected, actual)) if not same(a, b)]
    for i, source, native in gaps[:5]:
        print(f'Case {i}: {inputs[i]}\nRust: {source}\nC++: {native}')
    print(f'Metadata execution {args.case}: {len(inputs)-len(gaps)}/{len(inputs)} exact observations')
    return bool(gaps)


if __name__ == '__main__':
    raise SystemExit(main())
