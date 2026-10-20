#!/usr/bin/env python3
"""Replay exact source runtime diagnostic-source observations."""
import argparse
import gzip
import json
import os
from compare_values import observe, same
from generate_derived_query_diagnostics_5_0_6 import CORPUS, metadata


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', required=True)
    parser.add_argument('--rust')
    args = parser.parse_args()
    os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
    with gzip.open(CORPUS, 'rt') as source:
        corpus = json.load(source)
    for key, value in metadata().items():
        assert same(corpus[key], value), f'Changed source provenance: {key}'
    inputs = [case['input'] for case in corpus['cases']]
    expected = [case['expected'] for case in corpus['cases']]
    if args.rust:
        assert same(expected, observe(args.rust, inputs)), 'Live original changed'
    actual = observe(args.cpp, inputs)
    gaps = [i for i, (o, c) in enumerate(zip(expected, actual)) if not same(o, c)]
    if gaps:
        raise SystemExit(f'{len(gaps)}/{len(inputs)} derived query diagnostics differ')
    print(f'Derived query diagnostics: {len(inputs)}/{len(inputs)} exact observations match')


if __name__ == '__main__':
    main()
