#!/usr/bin/env python3
"""Compare trusted dynamic-field loading, JSON requests and policy execution on SQLite."""
import argparse
import gzip
import hashlib
import itertools
import os
import json
import sys
from pathlib import Path
from compare_values import observe, same
from generate_dynamic_json_policy_5_0_6 import adapted

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts'))
from verify_constant_closure_5_0_6 import registry_identities
from verify_state_baselines_5_0_6 import versions
CORPUS = Path(__file__).with_name('dynamic_json_persistent_5_0_6_cases.json.gz')

def inputs():
    result=[]
    for entity,minimal,configuration,policy,mode,filter_kind,logging in itertools.product(['person','order','product','line'],[False,True],['none','readonly','readonly_alias','empty','all','selected'],['allow','restrict','slice','deny'],['list','one','stream','count','exists','page'],['empty','untrusted_controls','match','missing'],[False,True]):
        key={'person':'name','order':'code','product':'sku','line':'quantity'}[entity]
        matched={'person':'Alice','order':'Alpha','product':'A','line':1}[entity]
        criteria={} if filter_kind=='empty' else {'_computed':999,'#title':'incoming override','_dynamicFieldDefinitions':{'title':'replace'},'_dynamicFieldSelection':'all'} if filter_kind=='untrusted_controls' else {key:matched} if filter_kind=='match' else {key:'absent'}
        result.append(dict(entity=entity,minimal=minimal,configuration=configuration,policy=policy,execution=mode,filters=[json.dumps(criteria)],execute=True,logging=logging,pre_slice=False,unlimited=False,chunk=1,stop=99))
    return result

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', required=True)
    parser.add_argument('--rust')
    parser.add_argument('--record', action='store_true')
    args = parser.parse_args()
    lock = ROOT / 'tests/parity/rust_dynamic_json_persistent_5_0_6/Cargo.lock'
    reference_lock = ROOT / '.reference/teaql-rs/Cargo.lock'
    reference_versions = versions(reference_lock)
    for name, resolved in versions(lock).items():
        if name not in {'teaql_dynamic_json_persistent_506', 'cpp-dynamic-json-core'}:
            if not (resolved <= reference_versions.get(name, set())):
                raise AssertionError(name)
    if not (registry_identities(lock) <= registry_identities(reference_lock)):
        raise AssertionError()
    directory=ROOT/'tests/generator/parity/dynamic_json_rust'
    provenance=json.loads((directory/'PROVENANCE.json').read_text())
    for name,digest in provenance['files'].items():
        if not (hashlib.sha256((directory/name).read_bytes()).hexdigest()==digest):
            raise AssertionError(name)
    os.environ['TEAQL_LOG_ENDPOINT']='off'
    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    schema_hash = {name:hashlib.sha256((ROOT/'tests/support'/name).read_bytes()).hexdigest() for name in ['dynamic_json_schema.sql','dynamic_json_dataset.sql']}
    cases = inputs()
    if args.record:
        if not (args.rust and not CORPUS.exists()):
            raise AssertionError()
        expected = observe(args.rust, cases)
        CORPUS.write_bytes(gzip.compress(json.dumps(dict(baseline=baseline, producer=provenance, schema_sha256=schema_hash,
            cases=[dict(input=i, expected=e) for i,e in zip(cases,expected)]),ensure_ascii=False).encode(),mtime=0))
    else:
        corpus = json.loads(gzip.decompress(CORPUS.read_bytes()))
        if not (corpus['baseline'] == baseline and corpus['producer'] == provenance):
            raise AssertionError()
        if not (corpus['schema_sha256'] == schema_hash):
            raise AssertionError()
        if not (same(cases, [c['input'] for c in corpus['cases']])):
            raise AssertionError()
        expected = [c['expected'] for c in corpus['cases']]
        if args.rust:
            if not same(expected, observe(args.rust, cases)):
                raise RuntimeError('Original request observations changed')
    actual = observe(args.cpp, cases)
    failures = [i for i,(a,b) in enumerate(zip(expected,actual)) if not same(adapted(a),b)]
    if args.record:
        path=ROOT / 'docs/parity/dynamic_json_persistent_initial_5_0_6.json.gz'
        if not (not path.exists()):
            raise AssertionError()
        path.write_bytes(gzip.compress(json.dumps(dict(inputs=cases,original=expected,cpp=actual,failures=failures),ensure_ascii=False).encode(),mtime=0))
    for i in failures[:3]:
        print('Case',i,cases[i])
        for name in expected[i]:
            if not same(expected[i][name],actual[i][name]):
                print(name, 'Rust:',expected[i][name], 'C++:',actual[i][name])
    print(f'Persistent JSON composition: {len(cases)-len(failures)}/{len(cases)} exact original observations')
    return bool(failures)

if __name__ == '__main__':
    raise SystemExit(main())
