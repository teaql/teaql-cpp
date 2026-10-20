#!/usr/bin/env python3
"""Compare actual generated reverse-many handles, evaluation and sparse members."""
import argparse
import gzip
import itertools
import json
import os
from pathlib import Path

from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]
CORPUS = Path(__file__).with_name('generated_reverse_many_helpers_5_0_6_cases.json.gz')


def cases():
    for field, size, loaded, selected, edge, key, child, name in itertools.product(
            ('school_list', 'device_list'), (0, 1, 3), (False, True), (False, True),
            ('absent', 'list', 'wrong'), ('same', 'owner', 'id'), (False, True),
            ('member', '雪\u0085\0' * 32)):
        yield dict(field=field, size=size, list_loaded=loaded, selected=selected,
                   edge=edge, key=key, child_loaded=child, name=name)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', required=True)
    parser.add_argument('--rust')
    parser.add_argument('--record', action='store_true')
    args = parser.parse_args()
    if args.record and not args.rust:
        parser.error('--record requires actual original execution')
    os.environ['TEAQL_LOG_ENDPOINT'] = 'off'
    inputs = list(cases())
    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    if args.record:
        expected = observe(args.rust, inputs)
    else:
        data = json.loads(gzip.decompress(CORPUS.read_bytes()))
        if data['baseline'] != baseline or not same(inputs, [v['input'] for v in data['cases']]):
            raise RuntimeError('Reverse-many corpus baseline/matrix differs')
        expected = [v['expected'] for v in data['cases']]
        if args.rust and not same(expected, observe(args.rust, inputs)):
            raise RuntimeError('Reverse-many corpus differs from actual original')
    native = observe(args.cpp, inputs)
    gaps = [dict(input=value, rust=source, cpp=actual)
            for value, source, actual in zip(inputs, expected, native) if not same(source, actual)]
    if gaps:
        path = ROOT / 'docs/parity/generated_reverse_many_helpers_5_0_6_initial_gaps.json.gz'
        if not path.exists():
            path.write_bytes(gzip.compress(json.dumps(gaps, indent=2).encode(), mtime=0))
        print(json.dumps(gaps[0], indent=2))
        raise SystemExit(f'{len(gaps)}/{len(inputs)} reverse-many differences')
    if args.record:
        record = dict(baseline=baseline, scope='Complete unchanged original relations domain and '
                      'generated school_list/device_list getters/evaluation. Initialized graph, absent/empty/'
                      'nonempty/wrong-type views, separate list flag and parent availability, wrong ownership/'
                      'identity keys and sparse child scalars. Exact panic/exception messages; no normalization.',
                      cases=[dict(input=value, expected=result) for value, result in zip(inputs, expected)])
        CORPUS.write_bytes(gzip.compress(json.dumps(record, separators=(',', ':')).encode(), mtime=0))
    print(f'Reverse-many helpers: {len(inputs)}/{len(inputs)} exact original/native outcomes')


if __name__ == '__main__':
    main()
