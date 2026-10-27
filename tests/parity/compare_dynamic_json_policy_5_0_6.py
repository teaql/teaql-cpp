#!/usr/bin/env python3
"""Replay actual source-generated JSON/RequestPolicy observations."""
import argparse
import gzip
import json
import os
from compare_values import observe, same
from generate_dynamic_json_policy_5_0_6 import CORPUS, adapted, metadata, inputs as source_inputs
from expression_cli_cases import require_cases


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
    cases = require_cases(corpus, 3072, source_inputs())
    inputs = [case['input'] for case in cases]
    expected = [case['expected'] for case in cases]
    if args.rust:
        if not same(expected, observe(args.rust, inputs)):
            raise RuntimeError('Live original changed')
    actual = observe(args.cpp, inputs)
    gaps = [i for i, (o, c) in enumerate(zip(expected, actual)) if not same(adapted(o), c)]
    if gaps:
        raise SystemExit(f'{len(gaps)}/{len(inputs)} dynamic JSON policy differences')
    print(f'Dynamic JSON policy: {len(inputs)}/{len(inputs)} original observations match')


if __name__ == '__main__':
    main()
