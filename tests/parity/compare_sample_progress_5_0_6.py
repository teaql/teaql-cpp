#!/usr/bin/env python3
"""Compare actual original Info messages, ordering and underlying sample outcomes."""
import argparse
import gzip
import hashlib
import itertools
import json
from pathlib import Path

from compare_values import observe, same
from expression_cli_cases import require_cases

ROOT = Path(__file__).resolve().parents[2]
CORPUS = Path(__file__).with_name('sample_progress_5_0_6_cases.json.gz')


def inputs():
    cases = [dict(module='combined', root=root, scale=scale, seed=seed,
                  rounds=rounds, enabled=enabled)
             for root, scale, seed, rounds, enabled in itertools.product(
                 ['normal', 'missing'], ['Tiny', 'Small'], [0, 7], [1, 2], [False, True])]
    cases += [dict(module='combined', root=root, scale='Medium', seed=0,
                   rounds=1, enabled=True) for root in ['normal', 'missing']]
    cases += [dict(module='combined', root='normal', scale='Tiny', seed=7,
                   rounds=2, enabled=enabled, fault=f)
              for f, enabled in itertools.product(['school', 'academy', 'research_academy'], [False, True])]
    return cases


def projected(original):
    if type(original) is not dict or set(original) != {'outcome', 'records'}:
        raise RuntimeError('Sample progress source root shape changed')
    if type(original['records']) is not list:
        raise RuntimeError('Sample progress source records must be a list')
    for record in original['records']:
        if type(record) is not dict or set(record) != {'level', 'target', 'message'}:
            raise RuntimeError('Sample progress source log shape changed')
        assert record['level'] == 'INFO'
        assert record['target'] == 'cpp_inheritance_core::sample_data'
    # Rust module_path targets remain in the raw archive. C++ dispatch uses a
    # Context-owned sample-specific sink, whose info() method fixes the level.
    return dict(outcome=original['outcome'], messages=[r['message'] for r in original['records']])


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--cpp', required=True)
    p.add_argument('--rust')
    p.add_argument('--record', action='store_true')
    p.add_argument('--cases', type=int, nargs='+',
                   help='Replay selected retained cases for focused consumer/lifetime checks')
    args = p.parse_args()
    if args.record and args.cases:
        p.error('Initial capture must retain the complete source matrix')
    directory = ROOT / 'tests/generator/parity/inheritance_rust'
    producer = json.loads((directory / 'PROVENANCE.json').read_text())
    assert hashlib.sha256((ROOT / producer['model']).read_bytes()).hexdigest() == producer['model_sha256']
    for name, digest in producer['files'].items():
        assert hashlib.sha256((directory / name).read_bytes()).hexdigest() == digest, name
    cases = inputs()
    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    if args.record:
        assert args.rust and not CORPUS.exists()
        expected = observe(args.rust, cases)
        CORPUS.write_bytes(gzip.compress(json.dumps(dict(
            baseline=baseline, producer=producer,
            adaptation='Original log facade becomes an explicitly registered Context sample Info sink. Original level/target/message records retained; exact messages and full timeless sample execution outcomes compared.',
            cases=[dict(input=i, expected=e) for i, e in zip(cases, expected)]
        ), ensure_ascii=False).encode(), mtime=0))
    else:
        corpus = json.loads(gzip.decompress(CORPUS.read_bytes()))
        require_cases(corpus, len(cases), cases)
        assert corpus['baseline'] == baseline and corpus['producer'] == producer
        assert same(cases, [c['input'] for c in corpus['cases']])
        expected = [c['expected'] for c in corpus['cases']]
        if args.cases:
            assert len(set(args.cases)) == len(args.cases)
            assert all(0 <= i < len(cases) for i in args.cases)
            cases = [cases[i] for i in args.cases]
            expected = [expected[i] for i in args.cases]
        if args.rust:
            if not same(expected, observe(args.rust, cases)):
                raise RuntimeError('Original logger/execution outcomes changed')
    actual = observe(args.cpp, cases)
    failures = [i for i, (e, a) in enumerate(zip(expected, actual)) if not same(projected(e), a)]
    if args.record:
        path = ROOT / 'docs/parity/sample_progress_initial_5_0_6.json.gz'
        assert not path.exists()
        path.write_bytes(gzip.compress(json.dumps(dict(
            schema=1, inputs=cases, original=expected, cpp=actual, failures=failures
        ), ensure_ascii=False).encode(), mtime=0))
    for i in failures[:3]:
        print(f'Case {i}: {cases[i]}')
        print('Rust messages:', projected(expected[i])['messages'])
        print('C++ messages:', actual[i]['messages'])
        print('Underlying execution matches:', same(expected[i]['outcome'], actual[i]['outcome']))
    count = sum(len(e['records']) for e in expected)
    print(f'Sample progress parity: {len(cases)-len(failures)}/{len(cases)}; {count} exact original Info messages')
    return bool(failures)


if __name__ == '__main__':
    raise SystemExit(main())
