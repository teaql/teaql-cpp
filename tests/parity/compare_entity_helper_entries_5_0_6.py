#!/usr/bin/env python3
"""Compare generated withId/setLoadState/isLoaded against original entity helpers."""
import argparse
import gzip
import itertools
import json
import os
from pathlib import Path

from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]
CORPUS = Path(__file__).with_name('entity_helper_entries_5_0_6_cases.json.gz')


def cases():
    for model, state, initial, fields, identifier in itertools.product(
            ('platform', 'school', 'device'),
            ('none', 'full', 'partial', 'compact', 'columns', 'indexed', 'other'),
            ('id', 'full'), ([], ['name'], ['id', 'platform', '#extra'],
                           ['name', 'name', 'school_list', 'missing']),
            ('0', '1', '4294967296', '9223372036854775807',
             '9223372036854775808', '18446744073709551615')):
        yield dict(model=model, state=state, initial=initial, fields=fields, identifier=identifier)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', required=True)
    parser.add_argument('--rust')
    parser.add_argument('--record', action='store_true')
    args = parser.parse_args()
    if args.record and not args.rust:
        parser.error('--record requires original source execution')
    os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
    inputs = list(cases())
    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    if args.record:
        expected = observe(args.rust, inputs)
    else:
        corpus = json.loads(gzip.decompress(CORPUS.read_bytes()))
        if corpus['baseline'] != baseline or not same(inputs, [c['input'] for c in corpus['cases']]):
            raise RuntimeError('Entity-helper corpus differs')
        expected = [c['expected'] for c in corpus['cases']]
        if args.rust and not same(expected, observe(args.rust, inputs)):
            raise RuntimeError('Entity-helper corpus differs from actual source')
    actual = observe(args.cpp, inputs)
    gaps = [dict(input=value, rust=source, cpp=native)
            for value, source, native in zip(inputs, expected, actual) if not same(source, native)]
    if gaps:
        path = ROOT / 'docs/parity/entity_helper_entries_5_0_6_initial_gaps.json.gz'
        if not path.exists():
            path.write_bytes(gzip.compress(json.dumps(gaps, indent=2).encode(), mtime=0))
        print(json.dumps(gaps[0], indent=2))
        raise SystemExit(f'{len(gaps)}/{len(inputs)} entity-helper differences')
    if args.record:
        corpus = dict(baseline=baseline, scope='Actual unchanged original Platform/School/Device '
                      'with_id, set_load_state and is_loaded. Seven input-state variants, full U64 '
                      'identifiers, compatible/incompatible indexed layouts, retained old state on failure '
                      'and unchanged scalar/original/mutation data. Failure is projected as rejection; '
                      'Rust panic and C++ exception diagnostic wrappers are separate language boundaries. '
                      'No output normalization.',
                      cases=[dict(input=value, expected=output) for value, output in zip(inputs, expected)])
        CORPUS.write_bytes(gzip.compress(json.dumps(corpus, separators=(',', ':')).encode(), mtime=0))
    print(f'Entity helper entries: {len(inputs)}/{len(inputs)} original/native outcomes')


if __name__ == '__main__':
    main()
