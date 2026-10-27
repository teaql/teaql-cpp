#!/usr/bin/env python3
"""Compare inherited Business-ID custom Checker and governed SQLite saves."""
from compare_values import diagnostic_path
import argparse
import gzip
import itertools
import json
import os
from pathlib import Path
from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]
CORPUS = Path(__file__).with_name('inherited_business_extensions_5_0_6_cases.json.gz')


def cases():
    for model, logic, allow, name in itertools.product(
            ('document', 'intermediate_record', 'invoice', 'receipt', 'special_invoice'),
            ('inspect', 'deny', 'fix'), (False, True), ('ok', '', 'long name')):
        for operation, faults in (('create', ('none', 'trigger')),
                                  ('update', ('none', 'trigger', 'conflict'))):
            for fault in faults:
                yield dict(model=model, logic=logic, allow=allow, name=name,
                           operation=operation, fault=fault)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', required=True)
    parser.add_argument('--rust')
    parser.add_argument('--record', action='store_true')
    args = parser.parse_args()
    if args.record and not args.rust:
        parser.error('--record requires actual source execution')
    os.environ.update(TEAQL_LOG_ENDPOINT='off', TEAQL_BUSINESS_ID_KEY_VERSION='1',
                      TEAQL_BUSINESS_ID_KEY_V1_HEX=bytes(range(32)).hex())
    inputs = list(cases())
    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    if args.record:
        expected = observe(args.rust, inputs)
    else:
        corpus = json.loads(gzip.decompress(CORPUS.read_bytes()))
        if corpus['baseline'] != baseline or not same(inputs, [c['input'] for c in corpus['cases']]):
            raise RuntimeError('Inherited extension corpus baseline/matrix differs')
        expected = [c['expected'] for c in corpus['cases']]
        if args.rust and not same(expected, observe(args.rust, inputs)):
            raise RuntimeError('Inherited extension corpus differs from actual source')
    actual = observe(args.cpp, inputs)
    gaps = [dict(input=value, rust=source, cpp=native)
            for value, source, native in zip(inputs, expected, actual) if not same(source, native)]
    if gaps:
        path = diagnostic_path(ROOT / 'docs/parity/inherited_business_extensions_5_0_6_initial_gaps.json.gz')
        if not path.exists():
            path.write_bytes(gzip.compress(json.dumps(gaps, indent=2).encode(), mtime=0))
        print(json.dumps(gaps[0], indent=2))
        raise SystemExit(f'{len(gaps)}/{len(inputs)} inherited extension differences')
    if args.record:
        corpus = dict(baseline=baseline,
                      generator_commit=json.loads((ROOT / 'docs/cpp_generator_parity_ledger.json').read_text())['generator_commit'],
                      source_artifact_acceptance=False,
                      scope='Five models use the accepted inherited Business-ID binding adaptation. '
                      'Existing generated Checker/entity/runtime methods unchanged. Actual public '
                      'audited Save, custom logic, immutable whole-plan Policy, SQLite commit/trigger/'
                      'optimistic failures, source diagnostics and ordered fix evidence. Clock '
                      'values are projected by presence/retention rather than normalized.',
                      cases=[dict(input=value, expected=output) for value, output in zip(inputs, expected)])
        CORPUS.write_bytes(gzip.compress(json.dumps(corpus, separators=(',', ':')).encode(), mtime=0))
    print(f'Inherited business extensions: {len(inputs)}/{len(inputs)} exact adapted source/native outcomes')


if __name__ == '__main__':
    main()
