#!/usr/bin/env python3
"""Compare complete child enhancement request/selection/core inputs across generated domains."""
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
CORPUS = Path(__file__).with_name('enhancement_inputs_5_0_6_cases.json.gz')

def inputs():
    profiles=[{}, {'child_deleted':True}, {'child_slice':True}, {'child_group':True},
              {'child_relations':True}, {'child_options':True}, {'repeat':True},
              {'child_options':True,'child_relations':True,'repeat':True}]
    result=[]
    for outer,child,outer_minimal,child_minimal,conversion,profile in itertools.product(
            ['person','order'],['person','order','product','school'],[False,True],[False,True],
            ['request','selection','query'],profiles):
        result.append(dict(outer=outer,child=child,outer_minimal=outer_minimal,
            child_minimal=child_minimal,conversion=conversion,alias=False,cutoff=10,**profile))
    for outer,child,conversion in itertools.product(['person','order'],['person','order','product'],['request','selection','query']):
        result.append(dict(outer=outer,child=child,outer_minimal=True,child_minimal=False,
            conversion=conversion,alias=False,cutoff=0,execute=True))
    return result

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', required=True)
    parser.add_argument('--rust')
    parser.add_argument('--record', action='store_true')
    args = parser.parse_args()
    lock = ROOT / 'tests/parity/rust_enhancement_inputs_5_0_6/Cargo.lock'
    reference_lock = ROOT / '.reference/teaql-rs/Cargo.lock'
    reference_versions = versions(reference_lock)
    for name, resolved in versions(lock).items():
        if name not in {'teaql_enhancement_inputs_506', 'cpp-dynamic-json-core', 'cpp-relations-core'}:
            assert resolved <= reference_versions.get(name, set()), name
    assert registry_identities(lock) <= registry_identities(reference_lock)
    provenance={}
    for domain in ['dynamic_json_rust','relations_rust']:
        directory=ROOT/'tests/generator/parity'/domain
        item=json.loads((directory/'PROVENANCE.json').read_text());provenance[domain]=item
        for name,digest in item['files'].items():
            assert hashlib.sha256((directory/name).read_bytes()).hexdigest()==digest,name
    review=json.loads((ROOT/'docs/parity/enhancement_inputs_source_review_5_0_6.json').read_text())
    for name,digest in review['fixture_sha256'].items():
        assert hashlib.sha256((ROOT/'tests/support'/name).read_bytes()).hexdigest()==digest,name
    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    __import__('os').environ['TEAQL_LOG_ENDPOINT']='off'
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
        path=ROOT / 'docs/parity/enhancement_inputs_initial_5_0_6.json.gz'
        assert not path.exists()
        path.write_bytes(gzip.compress(json.dumps(dict(inputs=cases,original=expected,cpp=actual,failures=failures),ensure_ascii=False).encode(),mtime=0))
    for i in failures[:3]:
        print('Case',i,cases[i])
        for name in expected[i]:
            if not same(expected[i][name],actual[i][name]):
                print(name, 'Rust:',expected[i][name], 'C++:',actual[i][name])
    print(f'Child enhancement inputs: {len(cases)-len(failures)}/{len(cases)} exact original observations')
    return bool(failures)

if __name__ == '__main__':
    raise SystemExit(main())
