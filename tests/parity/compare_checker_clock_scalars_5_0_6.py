#!/usr/bin/env python3
"""Compare configured scalar clock fixes through original entity/Checker bodies."""
import argparse
import gzip
import itertools
import json
from pathlib import Path

from compare_values import same
from sample_clock_driver import observe_fixed

ROOT = Path(__file__).resolve().parents[2]
CORPUS = Path(__file__).with_name('checker_clock_scalars_5_0_6_cases.json.gz')


def cases():
    defaults = {
        'text': ('text', '  界\x00' + 'long text ' * 32), 'code': ('text', '  code  '),
        'integer': ('i64', '17'), 'long': ('i64', '9223372036854775807'),
        'decimal': ('decimal', '12.50'), 'float': ('f64', '1.25'), 'double': ('f64', '-2.5'),
        'active': ('bool', True), 'date': ('date', '2024-01-02'), 'time': ('text', '12:34:56'),
        'payload': ('json', {'nested': '界\x00', 'values': [None, True, 7]}),
    }
    for status, profile, sparse, wall in itertools.product(
            ('create', 'update', 'unknown'), ('values', 'null', 'omitted'),
            (False, True), (1001, 1704067200789)):
        values = {'timestamp': dict(kind='timestamp', value='111')}
        for name, (kind, data) in defaults.items():
            values[name] = dict(kind=kind, value=data)
            if profile != 'omitted':
                values['optional_' + name] = dict(kind='null') if profile == 'null' else dict(kind=kind, value=data)
        # JSON fields are nullable in the locked default type definition.
        if profile == 'null':
            values['payload'] = dict(kind='null')
        yield dict(status=status, profile=profile, sparse=sparse, wall_millis=wall, values=values)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', type=Path, required=True)
    parser.add_argument('--rust', type=Path)
    parser.add_argument('--clock', type=Path, required=True)
    parser.add_argument('--record', action='store_true')
    args = parser.parse_args()
    inputs = list(cases())
    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    if args.record:
        if not args.rust:
            parser.error('--record requires live original entity/Checker execution')
        expected = observe_fixed(args.rust, inputs, args.clock)
    else:
        data = json.loads(gzip.decompress(CORPUS.read_bytes()))
        if data['baseline'] != baseline or not same(inputs, [c['input'] for c in data['cases']]):
            raise RuntimeError('Scalar clock matrix/baseline differs')
        expected = [c['expected'] for c in data['cases']]
        if args.rust and not same(expected, observe_fixed(args.rust, inputs, args.clock)):
            raise RuntimeError('Live original scalar clock results differ')
    actual = observe_fixed(args.cpp, inputs, args.clock)
    gaps = [dict(input=value, rust=source, cpp=native)
            for value, source, native in zip(inputs, expected, actual) if not same(source, native)]
    if gaps:
        print(json.dumps(gaps[0], ensure_ascii=True, indent=2))
        raise SystemExit(f'{len(gaps)}/{len(inputs)} configured scalar clock differences')
    if args.record:
        CORPUS.write_bytes(gzip.compress(json.dumps(dict(baseline=baseline,
            source_scope='Unmodified complete generated Note entity/Checker files compiled by path. '
                'Full original crate Boolean request errors are retained separately. '
                'No request/SQL/save acceptance follows from this component matrix.',
            cases=[dict(input=value, expected=result) for value, result in zip(inputs, expected)]),
            separators=(',', ':')).encode(), mtime=0))
    print(f'Configured scalar clock Checkers: {len(inputs)} cases / {len(inputs) * 23} field outcomes match')


if __name__ == '__main__':
    main()
