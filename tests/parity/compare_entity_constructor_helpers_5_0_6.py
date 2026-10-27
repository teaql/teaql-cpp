#!/usr/bin/env python3
"""Compare original private construction and inherited child scalar helpers."""
from compare_values import diagnostic_path
import argparse
import gzip
import itertools
import json
import os
from pathlib import Path

from compare_values import same
from sample_clock_driver import observe_fixed

ROOT = Path(__file__).resolve().parents[2]
CORPUS = Path(__file__).with_name('entity_constructor_helpers_5_0_6_cases.json.gz')


def profiles():
    for model, field in (('document', 'name'), ('intermediate_record', 'name'),
                         ('invoice', 'amount'), ('receipt', 'status'),
                         ('special_invoice', 'amount'), ('special_invoice', 'reference')):
        decimal = field == 'amount'
        values = [dict(kind='decimal', value=v) for v in
                  ('0', '12.50', '-1.250', '0.0000001', '79228162514264337593543950335')] if decimal else [
                      dict(kind='text', value=v) for v in ('', '  spaced  ', '\u0085\u3000', '雪\0' * 128)]
        for initial, value, op in itertools.product(('new', 'row'), values, ('none', 'update', 'self', 'overlay')):
            yield dict(model=model, field=field, initial=initial, value=value, op=op)
        for initial, value in itertools.product(('new', 'row'),
                (dict(kind='null'), dict(kind='typed_null'), dict(kind='bool', value=False),
                 dict(kind='text', value='incompatible') if decimal else dict(kind='i64', value='7'))):
            yield dict(model=model, field=field, initial=initial, value=value, op='overlay')


def cases():
    for value in profiles():
        for wall in (1001, 1704067200789):
            yield dict(value, wall_millis=wall)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', required=True)
    parser.add_argument('--rust')
    parser.add_argument('--clock', required=True)
    parser.add_argument('--record', action='store_true')
    args = parser.parse_args()
    if args.record and not args.rust:
        parser.error('--record requires actual original method execution')
    os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
    inputs = list(cases())
    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    if args.record:
        expected = observe_fixed(args.rust, inputs, args.clock)
    else:
        data = json.loads(gzip.decompress(CORPUS.read_bytes()))
        if data['baseline'] != baseline or not same(inputs, [v['input'] for v in data['cases']]):
            raise RuntimeError('Constructor helper corpus baseline/matrix differs')
        expected = [v['expected'] for v in data['cases']]
        if args.rust and not same(expected, observe_fixed(args.rust, inputs, args.clock)):
            raise RuntimeError('Constructor helper corpus differs from original methods')
    native = observe_fixed(args.cpp, inputs, args.clock)
    gaps = [dict(input=value, rust=source, cpp=actual)
            for value, source, actual in zip(inputs, expected, native) if not same(source, actual)]
    if gaps:
        path = diagnostic_path(ROOT / 'docs/parity/entity_constructor_helpers_5_0_6_initial_gaps.json.gz')
        if not path.exists():
            path.write_bytes(gzip.compress(json.dumps(gaps, indent=2).encode(), mtime=0))
        print(json.dumps(gaps[0], indent=2))
        raise SystemExit(f'{len(gaps)}/{len(inputs)} constructor/helper differences')
    if args.record:
        data = dict(baseline=baseline, scope='Unchanged original entity modules compiled inside a '
                    'test-only crate with narrow runtime_new export wrappers. Actual private default '
                    'construction, row/recursive root attachment, inherited and child-only scalar '
                    'getter/update/changed/eval, snapshot/clone and incompatible ledger overlays. '
                    'Fixed process wall clock; no algorithm replacement or normalization.',
                    cases=[dict(input=value, expected=result) for value, result in zip(inputs, expected)])
        CORPUS.write_bytes(gzip.compress(json.dumps(data, separators=(',', ':')).encode(), mtime=0))
    print(f'Constructor/inherited child helpers: {len(inputs)}/{len(inputs)} exact outcomes')


if __name__ == '__main__':
    main()
