#!/usr/bin/env python3
"""Compare native diagnostic composition with untouched original observations."""
from compare_values import diagnostic_path
import argparse
import gzip
import json
import os
from pathlib import Path
from compare_values import observe, same
from generate_derived_query_composition_source_5_0_6 import CORPUS
from generate_derived_query_diagnostics_5_0_6 import metadata

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', required=True)
    parser.add_argument('--rust')
    args = parser.parse_args()
    os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
    with gzip.open(CORPUS, 'rt') as source:
        corpus = json.load(source)
    for key, value in metadata().items():
        if not (same(corpus[key], value)):
            raise AssertionError(f'Changed provenance: {key}')
    inputs = [case['input'] for case in corpus['cases']]
    expected = [case['expected'] for case in corpus['cases']]
    if args.rust:
        if not same(expected, observe(args.rust, inputs)):
            raise RuntimeError('Live original changed')
    native = observe(args.cpp, inputs)
    gaps = [dict(index=i, input=x, rust=o, cpp=c)
            for i, (x, o, c) in enumerate(zip(inputs, expected, native)) if not same(o, c)]
    if gaps:
        target = diagnostic_path(ROOT / 'docs/parity/derived_query_composition_5_0_6_initial_gaps.json')
        if not target.exists():
            target.write_text(json.dumps(dict(count=len(gaps), differences=gaps), indent=2) + '\n')
        print(json.dumps(gaps[:1], indent=2))
        raise SystemExit(f'{len(gaps)}/{len(inputs)} diagnostic-source composition differences')
    print(f'Derived query composition: {len(inputs)}/{len(inputs)} exact original/native observations match')


if __name__ == '__main__':
    main()
