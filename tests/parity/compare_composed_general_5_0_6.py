#!/usr/bin/env python3
"""Compare composed trusted query, Policy, privacy and execution boundaries exactly."""
import argparse
import gzip
import hashlib
import itertools
import json
import os
from pathlib import Path
import sys
from compare_values import observe,same
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts'))
from verify_constant_closure_5_0_6 import registry_identities
from verify_state_baselines_5_0_6 import versions
CORPUS=Path(__file__).with_name('composed_general_5_0_6_cases.json.gz')


def inputs():
    entities=['school','academy','research_academy','enrollment']
    modes=['list','rows','first','one','count','exists','page','stream']
    shapes=['plain','readonly','raw_projection','raw_filter','raw_outer','facet','grouped','missing_table']
    cases=[]
    for entity,mode,shape,policy,logging in itertools.product(entities,modes,shapes,
            ['allow','restrict','deny','reject_raw'],[False,True]):
        for optimization in (['none','id_set','continuous'] if mode in ['list','first','one','page'] else ['none']):
            cases.append(dict(entity=entity,mode=mode,shape=shape,policy=policy,logging=logging,optimization=optimization,intent='valid'))
    for entity,mode,shape,policy,logging,intent in itertools.product(entities,modes,['plain','raw_outer','facet'],
            ['allow','deny'],[False,True],['blank_comment','blank_purpose']):
        cases.append(dict(entity=entity,mode=mode,shape=shape,policy=policy,logging=logging,optimization='none',intent=intent))
    if not (len(cases)==4864):
        raise AssertionError()
    return cases


def identity():
    project=ROOT/'tests/parity/rust_dynamic_json_privacy_5_0_6'
    reference=ROOT/'.reference/teaql-rs/Cargo.lock'
    if not (registry_identities(project/'Cargo.lock')<=registry_identities(reference)):
        raise AssertionError()
    wanted=versions(reference)
    for name,values in versions(project/'Cargo.lock').items():
        if name not in {'teaql_dynamic_json_privacy_506','cpp-inheritance-graph-core'}:
            if not (values<=wanted.get(name,set())):
                raise AssertionError(name)
    generated=ROOT/'tests/generator/parity/inheritance_graph_rust'
    producer=json.loads((generated/'PROVENANCE.json').read_text())
    baseline=json.loads((ROOT/'upstream.lock.json').read_text())['commit']
    if not (producer['runtime_commit']==baseline):
        raise AssertionError()
    for name,digest in producer['files'].items():
        if not (hashlib.sha256((generated/name).read_bytes()).hexdigest()==digest):
            raise AssertionError(name)
    if not (hashlib.sha256((ROOT/producer['model']).read_bytes()).hexdigest()==producer['model_sha256']):
        raise AssertionError()
    paths=[project/'Cargo.toml',project/'Cargo.lock',project/'src/bin/composed_general.rs',
           ROOT/'tests/parity/rust_field_layout/src/bin/original_like_privacy_5_0_6.rs',
           ROOT/'tests/support/dynamic_json_inherited_rows.sql',ROOT/'tests/support/dynamic_json_privacy_values.sql']
    return dict(baseline=baseline,producer=producer,sources={str(path.relative_to(ROOT)):hashlib.sha256(path.read_bytes()).hexdigest() for path in paths})


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp');parser.add_argument('--rust');parser.add_argument('--record-source',action='store_true')
    parser.add_argument('--record-comparison',type=Path)
    args=parser.parse_args();os.environ['TEAQL_LOG_ENDPOINT']='off'
    current,cases=identity(),inputs()
    if args.record_source:
        if not (args.rust and not CORPUS.exists()):
            raise AssertionError()
        expected=observe(args.rust,cases)
        if not (all(row['panic'] is None for case,row in zip(cases,expected) if case['intent']=='valid')):
            raise AssertionError('Inspect source preparation failures before recording')
        for case,row in zip(cases,expected):
            if case['intent']!='valid':
                if not (row['panic']==('query comment must not be empty' if case['intent']=='blank_comment' else 'query purpose must not be empty')):
                    raise AssertionError()
                if not (row['selects']==0 and row['policy']==[] and row['logs']==[]):
                    raise AssertionError()
            if not (row['released']):
                raise AssertionError()
            if not case['logging']:
                if not (row['logs']==[]):
                    raise AssertionError()
        CORPUS.write_bytes(gzip.compress(json.dumps(dict(**current,cases=[dict(input=i,expected=e) for i,e in zip(cases,expected)]),ensure_ascii=False).encode(),mtime=0))
    else:
        corpus=json.loads(gzip.decompress(CORPUS.read_bytes()))
        for key,value in current.items():
            if not (corpus[key]==value):
                raise AssertionError(key)
        if not (same(cases,[row['input'] for row in corpus['cases']])):
            raise AssertionError()
        expected=[row['expected'] for row in corpus['cases']]
        if args.rust:
            if not same(expected,observe(args.rust,cases)):
                raise RuntimeError('Original composed observations changed')
    if not args.cpp:
        print('Recorded actual original composed query outcomes:',len(cases));return 0
    actual=observe(args.cpp,cases)
    failures=[i for i,(a,b) in enumerate(zip(expected,actual)) if not same(a,b)]
    if args.record_comparison:
        if not (not args.record_comparison.exists()):
            raise AssertionError()
        args.record_comparison.write_bytes(gzip.compress(json.dumps(dict(inputs=cases,original=expected,cpp=actual,failures=failures),ensure_ascii=False).encode(),mtime=0))
    for index in failures[:3]:
        print('Case',index,cases[index]);print('Different fields:',[key for key in expected[index] if key not in actual[index] or not same(expected[index][key],actual[index][key])])
    print(f'Composed general query execution: {len(cases)-len(failures)}/{len(cases)} exact original outcomes')
    return bool(failures)


if __name__=='__main__':raise SystemExit(main())
