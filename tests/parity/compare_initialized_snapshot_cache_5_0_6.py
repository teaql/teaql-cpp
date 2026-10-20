#!/usr/bin/env python3
"""Compare original initialized cache geometry and explicit ownership adaptations."""
import argparse
import gzip
import itertools
import json
from pathlib import Path

from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]
CORPUS = Path(__file__).with_name('initialized_snapshot_cache_5_0_6_cases.json.gz')


def cases():
    for count, sparse, relations, schema, other in itertools.product(
            (2, 65, 130), (False, True), (False, True), (0, 1, 2), (False, True)):
        selected = [] if sparse else ['id', 'version', 'sql_2', 'sql_64', 'sql_129', '#memo', 'unknown']
        yield dict(kind='layout', count=count, selected=selected, relations=relations,
                   schema=schema, other_entity=other,
                   lookups=['id', 'version', 'field_2', 'sql_2', 'field_64', 'sql_64',
                            'field_129', 'sql_129', 'lines', '#memo', '_rank', 'unknown'])
    for count, release in itertools.product((511, 512, 513, 520), (False, True)):
        yield dict(kind='saturation', shapes=count, release=release)
    yield dict(kind='ownership')


def validate_ownership(source, native):
    expected_source = dict(model='weak_arc', within_row=[True, False], after_row=[False, False])
    expected_native = dict(model='context_arena', within_row=[True, True], after_row=[True, True])
    if source['ownership'] != expected_source or native['ownership'] != expected_native:
        raise RuntimeError(f'Unexpected lifetime adaptation: {source} / {native}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', required=True)
    parser.add_argument('--rust')
    parser.add_argument('--record', action='store_true')
    args = parser.parse_args()
    inputs = list(cases())
    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    native = observe(args.cpp, inputs)
    source = observe(args.rust, inputs) if args.rust else None
    if args.record:
        if source is None:
            parser.error('--record requires the actual original source observer')
    else:
        data = json.loads(gzip.decompress(CORPUS.read_bytes()))
        if data['baseline'] != baseline or inputs != [c['input'] for c in data['cases']]:
            raise RuntimeError('Cache corpus baseline or inputs differ')
        expected = [c['source'] for c in data['cases']]
        if source is not None:
            for saved, live in zip(expected, source):
                # Raw initialized Debug includes randomized maps and allocation keys.
                # Retain it as original output; it is outside the stable comparison contract.
                if not same(saved.get('common'), live.get('common')) or saved.get('ownership') != live.get('ownership'):
                    raise RuntimeError('Live original cache semantics differ')
        else:
            source = expected
    gaps = []
    for entry, original, actual in zip(inputs, source, native):
        if entry['kind'] == 'ownership':
            validate_ownership(original, actual)
        elif not same(original['common'], actual['common']):
            gaps.append(dict(input=entry, rust=original, cpp=actual))
        if entry['kind'] != 'ownership' and not actual.get('diagnostics'):
            raise RuntimeError('Native initialized diagnostics were omitted')
    if gaps:
        path = ROOT / 'docs/parity/initialized_snapshot_cache_initial_gaps_5_0_6.json.gz'
        if not path.exists():
            path.write_bytes(gzip.compress(json.dumps(dict(gaps=gaps), indent=2).encode(), mtime=0))
        print(json.dumps(gaps[0], ensure_ascii=True, indent=2))
        raise SystemExit(f'{len(gaps)}/{len(inputs)-1} initialized cache cases differ')
    if args.record:
        data = dict(baseline=baseline,
                    scope='Actual public snapshot/row APIs. Stable common geometry and identity observations '
                          'compare exactly. Rust weak release and C++ explicit destruction are declared '
                          'lifetime adaptations. Raw initialized diagnostics are retained independently '
                          'and are not normalized or claimed text-equivalent.',
                    cases=[dict(input=entry, source=original, native_diagnostics=actual.get('diagnostics'),
                                native_ownership=actual.get('ownership'))
                           for entry, original, actual in zip(inputs, source, native)])
        CORPUS.write_bytes(gzip.compress(json.dumps(data, separators=(',', ':')).encode(), mtime=0))
    print(f'Initialized snapshot caches: {len(inputs)-1} exact semantic cases and one explicit ownership adaptation')


if __name__ == '__main__':
    main()
