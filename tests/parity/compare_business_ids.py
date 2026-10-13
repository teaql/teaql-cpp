#!/usr/bin/env python3
import argparse
import gzip
import json
from pathlib import Path
from compare_values import observe, same
from generate_business_ids import adapt_source, RESOURCE_MESSAGES


def main():
    parser = argparse.ArgumentParser(description='Compare Business-ID corpus and explicit C++ type-name adaptation')
    parser.add_argument('--cpp', required=True)
    parser.add_argument('--rust')
    args = parser.parse_args()
    corpus = json.loads(gzip.decompress(Path(__file__).with_name('business_ids_5_0_6_cases.json.gz').read_bytes()))
    lock = json.loads(Path(__file__).resolve().parents[2].joinpath('upstream.lock.json').read_text())
    producer = json.loads(Path(__file__).resolve().parents[2].joinpath(
        'tests/generator/parity/business_ids_rust/PROVENANCE.json').read_text())
    if corpus['baseline'] != lock['commit'] or corpus['resource_message_mapping'] != RESOURCE_MESSAGES:
        raise SystemExit('Business-ID corpus baseline or adaptation differs')
    if corpus['producer'] != producer:
        raise SystemExit('Business-ID corpus original producer fixture is stale')
    cases = corpus['cases']
    for case in cases:
        if not same(adapt_source(case['source_expected']), case['expected']):
            raise SystemExit('Business-ID corpus changed beyond its declared type-name adaptation')
    inputs = [case['input'] for case in cases]
    actual = observe(args.cpp, inputs)
    if len(actual) != len(cases):
        raise SystemExit('Incomplete C++ Business-ID output')
    for index, (case, value) in enumerate(zip(cases, actual)):
        if not same(case['expected'], value):
            raise SystemExit(f'Business-ID mismatch {index}')
    if args.rust:
        source = observe(args.rust, inputs)
        if len(source) != len(cases) or any(not same(case['source_expected'], value)
                                          for case, value in zip(cases, source)):
            raise SystemExit('Live original Business-ID observations differ')
    print(f'Compared {len(cases)} generated Business-ID inputs with declared resource type-name adaptation')
    return 0

if __name__ == '__main__':
    raise SystemExit(main())
