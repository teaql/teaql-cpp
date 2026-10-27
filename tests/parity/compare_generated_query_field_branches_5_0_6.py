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
CORPUS = Path(__file__).with_name('generated_query_field_branches_5_0_6_cases.json.gz')

def inputs():
    fields=['id','version','label','optional_label','notes','secret','active','optional_active','amount','optional_amount','height','ratio','counter','day','optional_day','moment','optional_moment']
    values=[dict(kind='null'),dict(kind='typed_null',value='text'),dict(kind='text',value=''),
            dict(kind='text',value='雪\0value'),dict(kind='bool',value=True),dict(kind='i64',value='-3'),
            dict(kind='u64',value='18446744073709551615'),dict(kind='f64',value='2.5'),
            dict(kind='decimal',value='2.50'),dict(kind='date',value='2026-10-14'),dict(kind='timestamp',value='1234')]
    result=[]
    for field in fields:
        ops=['asc','desc'] if field=='version' else ['eq','ne','in','not_in','asc','desc'] if field=='id' else ['positive','negative','unknown','known','asc','desc'] if field in ['active','optional_active'] else ['eq','ne','gt','lt','before','after','between','in','not_in','unknown','known','asc','desc']
        for op,minimal,repeat,operand in itertools.product(ops,[False,True],[False,True],values):
            result.append(dict(field=field,operation=op,minimal=minimal,repeat=repeat,value=operand,
                               upper=dict(kind='i64',value='9')))
    return result

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', required=True)
    parser.add_argument('--rust')
    parser.add_argument('--record', action='store_true')
    args = parser.parse_args()
    lock = ROOT / 'tests/parity/rust_query_field_branches_5_0_6/Cargo.lock'
    reference_lock = ROOT / '.reference/teaql-rs/Cargo.lock'
    reference_versions = versions(reference_lock)
    for name, resolved in versions(lock).items():
        if name not in {'teaql_query_field_branches_506', 'cpp-query-branches-core'}:
            assert resolved <= reference_versions.get(name, set()), name
    assert registry_identities(lock) <= registry_identities(reference_lock)
    directory = ROOT / 'tests/generator/parity/query_field_branches_rust'
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
            if not same(expected, observe(args.rust, cases)):
                raise RuntimeError('Original request observations changed')
    actual = observe(args.cpp, cases)
    failures = [i for i,(a,b) in enumerate(zip(expected,actual)) if not same(a,b)]
    if args.record:
        path=ROOT / 'docs/parity/generated_query_field_branches_initial_5_0_6.json.gz'
        assert not path.exists()
        path.write_bytes(gzip.compress(json.dumps(dict(inputs=cases,original=expected,cpp=actual,failures=failures),ensure_ascii=False).encode(),mtime=0))
    for i in failures[:3]:
        print('Case',i,cases[i])
        for name in expected[i]:
            if not same(expected[i][name],actual[i][name]):
                print(name, 'Rust:',expected[i][name], 'C++:',actual[i][name])
    print(f'Generated query field branches: {len(cases)-len(failures)}/{len(cases)} exact original observations')
    return bool(failures)

if __name__ == '__main__':
    raise SystemExit(main())
