#!/usr/bin/env python3
"""Compare original generated request views and distinct consuming conversions."""
import argparse
import gzip
import hashlib
import itertools
import json
import sys
from pathlib import Path
from compare_values import observe, same

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from verify_constant_closure_5_0_6 import registry_identities
from verify_state_baselines_5_0_6 import versions
CORPUS = Path(__file__).with_name('generated_request_state_5_0_6_cases.json.gz')

def inputs():
    names = ['options', 'child', 'relation', 'filter', 'shape']
    operations = [[name for i, name in enumerate(names) if mask & (1 << i)] for mask in range(32)]
    operations += [list(reversed(names)), names + names, ['relation', 'options', 'relation'], ['filter', 'child', 'filter']]
    return [dict(entity=entity, minimal=minimal, text=text, operations=ops)
            for entity, minimal, text, ops in itertools.product(
                ['person', 'order'], [False, True], ['Alpha', '', 'Long reference \"雪\" with NUL\0 and raw % _ \\'], operations)]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', required=True)
    parser.add_argument('--rust')
    parser.add_argument('--record', action='store_true')
    args = parser.parse_args()
    lock = ROOT / 'tests/parity/rust_request_state_5_0_6/Cargo.lock'
    reference_lock = ROOT / '.reference/teaql-rs/Cargo.lock'
    reference_versions = versions(reference_lock)
    for name, resolved in versions(lock).items():
        if name not in {'teaql_request_state_506', 'cpp-dynamic-json-core'}:
            if not (resolved <= reference_versions.get(name, set())):
                raise AssertionError(name)
    if not (registry_identities(lock) <= registry_identities(reference_lock)):
        raise AssertionError()
    directory = ROOT / 'tests/generator/parity/dynamic_json_rust'
    provenance = json.loads((directory / 'PROVENANCE.json').read_text())
    if not (hashlib.sha256((ROOT / provenance['model']).read_bytes()).hexdigest() == provenance['model_sha256']):
        raise AssertionError()
    for name, digest in provenance['files'].items():
        if not (hashlib.sha256((directory / name).read_bytes()).hexdigest() == digest):
            raise AssertionError(name)
    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    cases = inputs()
    if args.record:
        if not (args.rust and not CORPUS.exists()):
            raise AssertionError()
        expected = observe(args.rust, cases)
        CORPUS.write_bytes(gzip.compress(json.dumps(dict(baseline=baseline, producer=provenance,
            cases=[dict(input=i, expected=e) for i,e in zip(cases,expected)]),ensure_ascii=False).encode(),mtime=0))
    else:
        corpus = json.loads(gzip.decompress(CORPUS.read_bytes()))
        if not (corpus['baseline'] == baseline and corpus['producer'] == provenance):
            raise AssertionError()
        if not (same(cases, [c['input'] for c in corpus['cases']])):
            raise AssertionError()
        expected = [c['expected'] for c in corpus['cases']]
        if args.rust:
            if not same(expected, observe(args.rust, cases)):
                raise RuntimeError('Original request observations changed')
    actual = observe(args.cpp, cases)
    failures = [i for i,(a,b) in enumerate(zip(expected,actual)) if not same(a,b)]
    if args.record:
        path=ROOT / 'docs/parity/generated_request_state_initial_5_0_6.json.gz'
        if not (not path.exists()):
            raise AssertionError()
        path.write_bytes(gzip.compress(json.dumps(dict(inputs=cases,original=expected,cpp=actual,failures=failures),ensure_ascii=False).encode(),mtime=0))
    for i in failures[:3]:
        print('Case',i,cases[i])
        for name in expected[i]:
            if not same(expected[i][name],actual[i][name]):
                print(name, 'Rust:',expected[i][name], 'C++:',actual[i][name])
    print(f'Generated request state: {len(cases)-len(failures)}/{len(cases)} exact original observations; {len(cases)*7} complete views/conversions')
    return bool(failures)

if __name__ == '__main__':
    raise SystemExit(main())
