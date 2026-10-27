#!/usr/bin/env python3
"""Compare canonical relationship metadata, query state, typed JSON and SQL logs."""
import argparse
import gzip
import hashlib
import itertools
import json
import os
from pathlib import Path
import sys
from compare_values import observe,same
from expression_cli_cases import require_cases
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts'))
from verify_constant_closure_5_0_6 import registry_identities
CORPUS=Path(__file__).with_name('relation_identity_5_0_6_cases.json.gz')


def inputs():
    result=[]
    sequences=[[],['self'],['first'],['second'],['first_with'],['second_with'],
               ['first_with','second_with'],['all'],['children'],['first_with','remove_first'],
               ['first','first_with'],['deleted','children']]
    for entity,ops,minimal,mode in itertools.product(['entry','owner','profile'],sequences,[False,True],['list','rows']):
        result.append(dict(entity=entity,ops=ops,minimal=minimal,mode=mode,json='{}'))
    for entity,value in [('entry','{"namespace.name":"Alpha"}'),('owner','{"class.name":"First"}'),('profile','{"owner.name":"Alpha"}')]:
        for minimal,mode,ops in itertools.product([False,True],['list','rows'],[['json'],['json','children']]):
            result.append(dict(entity=entity,ops=ops,minimal=minimal,mode=mode,json=value))
    return result


def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def identity():
    base=ROOT/'tests/generator/parity/relation_identity_contracts/rust'
    provenance=json.loads((base/'PROVENANCE.json').read_text())
    baseline=json.loads((ROOT/'upstream.lock.json').read_text())['commit']
    assert provenance['runtime_commit']==baseline
    for path,expected in provenance['files'].items():assert digest(base/path)==expected,path
    assert digest(ROOT/provenance['model'])==provenance['model_sha256']
    project=ROOT/'tests/parity/rust_relation_identity_5_0_6'
    assert registry_identities(project/'Cargo.lock')<=registry_identities(ROOT/'.reference/teaql-rs/Cargo.lock')
    paths=[project/'Cargo.toml',project/'Cargo.lock',project/'src/main.rs',ROOT/'tests/support/relation_identity_sql.sql',ROOT/'tests/parity/rust_field_layout/src/bin/original_like_privacy_5_0_6.rs']
    return dict(baseline=baseline,producer=provenance,sources={str(p.relative_to(ROOT)):digest(p) for p in paths})


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--cpp',required=True);parser.add_argument('--rust');parser.add_argument('--record',action='store_true');args=parser.parse_args()
    os.environ['TEAQL_LOG_ENDPOINT']='off';current=identity();cases=inputs()
    if args.record:
        assert args.rust and not CORPUS.exists()
        expected=observe(args.rust,cases)
        CORPUS.write_bytes(gzip.compress(json.dumps(dict(**current,cases=[dict(input=i,expected=e) for i,e in zip(cases,expected)]),ensure_ascii=False).encode(),mtime=0))
    else:
        corpus=json.loads(gzip.decompress(CORPUS.read_bytes()))
        require_cases(corpus, len(cases), cases)
        for name,item in current.items():assert corpus[name]==item,name
        assert same(cases,[c['input'] for c in corpus['cases']]);expected=[c['expected'] for c in corpus['cases']]
        if args.rust:assert same(expected,observe(args.rust,cases)),'Original relationship behavior changed'
    actual=observe(args.cpp,cases)
    if len(actual) != len(cases):
        raise RuntimeError('Native observations must cover the complete input matrix')
    failures=[i for i,(a,b) in enumerate(zip(expected,actual)) if not same(a,b)]
    if args.record:
        p=ROOT/'docs/parity/relation_identity_initial_5_0_6.json.gz';assert not p.exists()
        p.write_bytes(gzip.compress(json.dumps(dict(inputs=cases,original=expected,cpp=actual,failures=failures),ensure_ascii=False).encode(),mtime=0))
    for i in failures[:2]:
        print('Case',i,cases[i]);print('Different fields:',[k for k in expected[i] if not same(expected[i][k],actual[i][k])])
    print(f'Relationship identities: {len(cases)-len(failures)}/{len(cases)} exact original observations')
    return bool(failures)


if __name__=='__main__':raise SystemExit(main())
