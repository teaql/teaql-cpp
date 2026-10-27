#!/usr/bin/env python3
"""Verify actual original typed Business-ID E snapshots and fallback constraints."""
import argparse
import gzip
import hashlib
import itertools
import json
from pathlib import Path

from compare_values import observe, same
from expression_diagnostic_framing import to_source

ROOT = Path(__file__).resolve().parents[2]
CORPUS = Path(__file__).with_name('expression_business_ids_5_0_6_cases.json.gz')


def inputs():
    values = [dict(kind='text', value=v) for v in ['', 'RCP-1', '  padded  ',
                                                   'Unicode 数据\0' + 'x' * 1200]]
    values += [dict(kind='null'), dict(kind='typed_null'),
               dict(kind='i64', value='1'), dict(kind='json', value='RCP-1')]
    return [dict(model=m, id=i, loaded=loaded, value=value)
            for m, i, loaded, value in itertools.product(
                ['Receipt', 'Ticket'], [0, 7, 18446744073709551615], [False, True], values)]


def adapted(value):
    if isinstance(value, dict):
        if set(value) == {'panic'} and isinstance(value['panic'], dict):
            return dict(panic=to_source(value['panic']))
        return {k: adapted(v) for k, v in value.items()}
    return value


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', required=True)
    parser.add_argument('--rust')
    parser.add_argument('--record', action='store_true')
    args = parser.parse_args()
    directory = ROOT / 'tests/generator/parity/business_ids_rust'
    producer = json.loads((directory / 'PROVENANCE.json').read_text())
    assert hashlib.sha256((ROOT / producer['model']).read_bytes()).hexdigest() == producer['model_sha256']
    for name, digest in producer['files'].items():
        assert hashlib.sha256((directory / name).read_bytes()).hexdigest() == digest, name
    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    cases = inputs()
    if args.record:
        assert args.rust and not CORPUS.exists(), 'Capture requires a fresh corpus and actual Rust'
        expected = observe(args.rust, cases)
        CORPUS.write_bytes(gzip.compress(json.dumps(dict(
            baseline=baseline, producer=producer,
            cases=[dict(input=i, expected=o) for i, o in zip(cases, expected)]
        ), ensure_ascii=False).encode(), mtime=0))
    else:
        corpus = json.loads(gzip.decompress(CORPUS.read_bytes()))
        assert corpus['baseline'] == baseline and corpus['producer'] == producer
        assert same(cases, [c['input'] for c in corpus['cases']])
        expected = [c['expected'] for c in corpus['cases']]
        if args.rust:
            if not same(expected, observe(args.rust, cases)):
                raise RuntimeError('Original source results changed')
    actual = observe(args.cpp, cases)
    failures = [i for i, (a, b) in enumerate(zip(expected, actual)) if not same(a, adapted(b))]
    if args.record:
        path = ROOT / 'docs/parity/expression_business_ids_source_5_0_6.json.gz'
        assert not path.exists()
        path.write_bytes(gzip.compress(json.dumps(dict(
            baseline=baseline, original=expected, cpp=actual, failures=failures,
            adaptation='Complete native diagnostic frames map only documented naming/English presentation.'
        ), ensure_ascii=False).encode(), mtime=0))
    for i in failures[:5]:
        print(f'Case {i}: {cases[i]}\nRust: {expected[i]}\nC++: {actual[i]}')
    resolutions = sum(12 for e in expected if 'decode_error' not in e)
    print(f'Business-ID E parity: {len(cases)-len(failures)}/{len(cases)}; '
          f'{resolutions} actual resolutions across before/captured/current snapshots')
    return bool(failures)


if __name__ == '__main__':
    raise SystemExit(main())
