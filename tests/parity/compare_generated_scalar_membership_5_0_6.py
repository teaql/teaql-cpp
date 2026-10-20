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
CORPUS = Path(__file__).with_name('generated_scalar_membership_5_0_6_cases.json.gz')

def inputs():
    integer = lambda n: dict(kind='i64', value=str(n))
    vectors = [[], [integer(1)], [integer(1),integer(1),integer(3)],
        [dict(kind='null'),dict(kind='typed_null',value='text'),integer(2)],
        [dict(kind='text',value='Alpha'),dict(kind='text',value='雪\0%_')],
        [dict(kind='bool',value=True),dict(kind='u64',value='3'),dict(kind='decimal',value='2.50')],
        [dict(kind='date',value='2026-10-14'),dict(kind='timestamp',value='1234')],
        [dict(kind='json',value={'key':'value'})],
        *[[integer(i) for i in range(n)] for n in [511,512,513,520]]]
    cases = [dict(field=field,minimal=minimal,negative=negative,repeat=repeat,
                  carrier=carrier,values=items,execute=False)
        for field,minimal,negative,repeat,carrier,items in itertools.product(
            ['name','person_id','code','order_id','priority','amount'],[False,True],
            [False,True],[False,True],['values','view'],vectors)]
    for field,negative,carrier in itertools.product(['name','person_id','code','order_id','priority','amount'],[False,True],['i64','stream','text']):
        items = ([dict(kind='text',value='Alpha'),dict(kind='text',value='Beta')] if carrier=='text'
                 else [integer(1),integer(2),integer(2),integer(9)])
        cases.append(dict(field=field,minimal=True,negative=negative,repeat=True,
                          carrier=carrier,values=items,execute=True))
    return cases

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', required=True)
    parser.add_argument('--rust')
    parser.add_argument('--record', action='store_true')
    args = parser.parse_args()
    lock = ROOT / 'tests/parity/rust_scalar_membership_5_0_6/Cargo.lock'
    reference_lock = ROOT / '.reference/teaql-rs/Cargo.lock'
    reference_versions = versions(reference_lock)
    for name, resolved in versions(lock).items():
        if name not in {'teaql_scalar_membership_506', 'cpp-dynamic-json-core'}:
            assert resolved <= reference_versions.get(name, set()), name
    assert registry_identities(lock) <= registry_identities(reference_lock)
    directory = ROOT / 'tests/generator/parity/dynamic_json_rust'
    provenance = json.loads((directory / 'PROVENANCE.json').read_text())
    assert hashlib.sha256((ROOT / provenance['model']).read_bytes()).hexdigest() == provenance['model_sha256']
    for name, digest in provenance['files'].items():
        assert hashlib.sha256((directory / name).read_bytes()).hexdigest() == digest, name
    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    cases = inputs()
    if args.record:
        assert args.rust and not CORPUS.exists()
        expected = observe(args.rust, cases)
        CORPUS.write_bytes(gzip.compress(json.dumps(dict(baseline=baseline, producer=provenance,
            cases=[dict(input=i, expected=e) for i,e in zip(cases,expected)]),ensure_ascii=False).encode(),mtime=0))
    else:
        corpus = json.loads(gzip.decompress(CORPUS.read_bytes()))
        assert corpus['baseline'] == baseline and corpus['producer'] == provenance
        assert same(cases, [c['input'] for c in corpus['cases']])
        expected = [c['expected'] for c in corpus['cases']]
        if args.rust:
            assert same(expected, observe(args.rust, cases)), 'Original request observations changed'
    actual = observe(args.cpp, cases)
    failures = [i for i,(a,b) in enumerate(zip(expected,actual)) if not same(a,b)]
    if args.record:
        path=ROOT / 'docs/parity/generated_scalar_membership_initial_5_0_6.json.gz'
        assert not path.exists()
        path.write_bytes(gzip.compress(json.dumps(dict(inputs=cases,original=expected,cpp=actual,failures=failures),ensure_ascii=False).encode(),mtime=0))
    for i in failures[:3]:
        print('Case',i,cases[i])
        for name in expected[i]:
            if not same(expected[i][name],actual[i][name]):
                print(name, 'Rust:',expected[i][name], 'C++:',actual[i][name])
    print(f'Generated scalar membership: {len(cases)-len(failures)}/{len(cases)} exact original observations')
    return bool(failures)

if __name__ == '__main__':
    raise SystemExit(main())
