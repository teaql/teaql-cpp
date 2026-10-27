#!/usr/bin/env python3
"""Compare actual inherited sample rows and complete deterministic SQL diagnostics."""
import argparse
import gzip
import hashlib
import itertools
import json
from pathlib import Path

from compare_values import observe, same
from expression_cli_cases import require_cases

ROOT = Path(__file__).resolve().parents[2]
CORPUS = Path(__file__).with_name('inherited_sample_execution_5_0_6_cases.json.gz')


def inputs():
    result = [dict(module=m, root=root, scale='Tiny', seed=seed, rounds=rounds)
              for m, root, seed, rounds in itertools.product(
                  ['plain', 'checker', 'behavior', 'combined'], ['normal', 'missing', 'modified'],
                  [0, 7, 18446744073709551596], [1, 2])]
    result += [dict(module=m, root='normal', scale='Small', seed=7, rounds=rounds)
               for m, rounds in itertools.product(['plain', 'checker', 'behavior', 'combined'], [1, 2])]
    result += [dict(module=m, root='normal', scale='Tiny', seed=7, rounds=2, fault=f)
               for m, f in itertools.product(['plain', 'checker', 'behavior', 'combined'],
                                             ['school', 'academy', 'research_academy'])]
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', required=True)
    parser.add_argument('--rust')
    parser.add_argument('--record', action='store_true')
    parser.add_argument('--smoke', action='store_true')
    args = parser.parse_args()
    directory = ROOT / 'tests/generator/parity/inheritance_rust'
    producer = json.loads((directory / 'PROVENANCE.json').read_text())
    if not (hashlib.sha256((ROOT / producer['model']).read_bytes()).hexdigest() == producer['model_sha256']):
        raise AssertionError()
    for name, digest in producer['files'].items():
        if not (hashlib.sha256((directory / name).read_bytes()).hexdigest() == digest):
            raise AssertionError(name)
    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    cases = inputs()
    if args.smoke:
        if not (args.rust and not args.record):
            raise AssertionError()
        cases = [dict(module=m, root='normal', scale='Tiny', seed=7, rounds=1)
                 for m in ['plain', 'checker', 'behavior', 'combined']]
        cases += [dict(module='combined', root='normal', scale='Tiny', seed=7, rounds=2, fault=f)
                  for f in ['school', 'academy', 'research_academy']]
        cases += [dict(module='combined', root='missing', scale='Tiny', seed=7, rounds=1)]
        expected = observe(args.rust, cases)
    elif args.record:
        if not (args.rust and not CORPUS.exists()):
            raise AssertionError('Capture needs a fresh corpus and real Rust')
        expected = observe(args.rust, cases)
        CORPUS.write_bytes(gzip.compress(json.dumps(dict(
            baseline=baseline, producer=producer,
            scope='Actual public generated module/schema/sample execution. Only wall-clock timings omitted; no data, SQL, trace or error normalization.',
            cases=[dict(input=i, expected=o) for i, o in zip(cases, expected)]
        ), ensure_ascii=False).encode(), mtime=0))
    else:
        corpus = json.loads(gzip.decompress(CORPUS.read_bytes()))
        require_cases(corpus, len(cases), cases)
        if not (corpus['baseline'] == baseline and corpus['producer'] == producer):
            raise AssertionError()
        if not (same(cases, [c['input'] for c in corpus['cases']])):
            raise AssertionError()
        expected = [c['expected'] for c in corpus['cases']]
        if args.rust:
            if not same(expected, observe(args.rust, cases)):
                raise RuntimeError('Original outcomes changed')
    actual = observe(args.cpp, cases)
    failures = [i for i, (a, b) in enumerate(zip(expected, actual)) if not same(a, b)]
    if args.smoke or args.record:
        name = 'inherited_sample_smoke_5_0_6.json.gz' if args.smoke else 'inherited_sample_source_5_0_6.json.gz'
        path = ROOT / 'docs/parity' / name
        if not (not path.exists()):
            raise AssertionError('Retained observations are immutable')
        path.write_bytes(gzip.compress(json.dumps(dict(
            schema=1, baseline=baseline, inputs=cases, original=expected, cpp=actual,
            matching=len(cases)-len(failures), total=len(cases), failures=failures
        ), ensure_ascii=False).encode(), mtime=0))
    for i in failures[:3]:
        a, b = expected[i], actual[i]
        print(f'Case {i}: {cases[i]}; source rounds={len(a.get("rounds", []))}, native rounds={len(b.get("rounds", []))}')
        for n, (x, y) in enumerate(zip(a.get('rounds', []), b.get('rounds', []))):
            for key in x:
                if not same(x[key], y.get(key)):
                    print(f'  round {n} differs: {key}')
                    if key == 'logs':
                        print(f'  log counts: Rust {len(x[key])}, C++ {len(y[key])}')
                        for j, (left, right) in enumerate(zip(x[key], y[key])):
                            if not same(left, right):
                                print(f'  first log difference {j}: {json.dumps(left, ensure_ascii=False)[:1300]}\nC++: {json.dumps(right, ensure_ascii=False)[:1300]}')
                                break
    logs = sum(len(r['logs']) for case in expected for r in case.get('rounds', []))
    print(f'Inherited sample parity: {len(cases)-len(failures)}/{len(cases)}; {logs} exact ordered SQL records')
    return bool(failures)


if __name__ == '__main__':
    raise SystemExit(main())
