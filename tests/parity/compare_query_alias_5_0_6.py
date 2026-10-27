#!/usr/bin/env python3
"""Compare authentic aggregate aliases and SQLite outcomes for escaped member names."""
import argparse
import gzip
import hashlib
import itertools
import json
import os
from pathlib import Path
import sys
from compare_values import observe, same
from expression_cli_cases import require_cases
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts'))
from verify_constant_closure_5_0_6 import registry_identities
CORPUS = Path(__file__).with_name('query_alias_5_0_6_cases.json.gz')


def inputs():
    return [dict(field=field,operation=op,alias=alias,minimal=minimal,repeat=repeat)
            for field in ['class','operator','template','type','match','ordinary','namespace']
            for op,alias,minimal,repeat in itertools.product(
                ['count'] if field=='namespace' else ['count','sum','avg','min','max','std','pop','var','var_pop'],
                [None,'_explicit','alias 雪\0'],[False,True],[False,True])]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def identity():
    directory=ROOT/'tests/generator/parity/query_alias_contracts/rust'
    provenance=json.loads((directory/'PROVENANCE.json').read_text())
    baseline=json.loads((ROOT/'upstream.lock.json').read_text())['commit']
    assert provenance['runtime_commit']==baseline
    for name,value in provenance['files'].items(): assert digest(directory/name)==value,name
    assert digest(ROOT/provenance['model'])==provenance['model_sha256']
    project=ROOT/'tests/parity/rust_query_alias_5_0_6'
    assert registry_identities(project/'Cargo.lock')<=registry_identities(ROOT/'.reference/teaql-rs/Cargo.lock')
    paths=[project/'Cargo.toml',project/'Cargo.lock',project/'src/main.rs',ROOT/'tests/support/query_alias_sql.sql',
           ROOT/'tests/parity/rust_field_layout/src/bin/original_like_privacy_5_0_6.rs']
    return dict(baseline=baseline,producer=provenance,sources={str(p.relative_to(ROOT)):digest(p) for p in paths})


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp',required=True);parser.add_argument('--rust');parser.add_argument('--record',action='store_true')
    args=parser.parse_args();os.environ['TEAQL_LOG_ENDPOINT']='off'
    current=identity();cases=inputs()
    if args.record:
        assert args.rust and not CORPUS.exists()
        expected=observe(args.rust,cases)
        CORPUS.write_bytes(gzip.compress(json.dumps(dict(**current,cases=[dict(input=i,expected=e) for i,e in zip(cases,expected)]),ensure_ascii=False).encode(),mtime=0))
    else:
        corpus=json.loads(gzip.decompress(CORPUS.read_bytes()))
        require_cases(corpus, len(cases), cases)
        for name,item in current.items():assert corpus[name]==item,name
        assert same(cases,[c['input'] for c in corpus['cases']])
        expected=[c['expected'] for c in corpus['cases']]
        if args.rust:
            if not same(expected,observe(args.rust,cases)):
                raise RuntimeError('Original alias observations changed')
    actual=observe(args.cpp,cases)
    if len(actual) != len(cases):
        raise RuntimeError('Native observations must cover the complete input matrix')
    failures=[i for i,(a,b) in enumerate(zip(expected,actual)) if not same(a,b)]
    if args.record:
        path=ROOT/'docs/parity/query_alias_initial_5_0_6.json.gz';assert not path.exists()
        path.write_bytes(gzip.compress(json.dumps(dict(inputs=cases,original=expected,cpp=actual,failures=failures),ensure_ascii=False).encode(),mtime=0))
    for i in failures[:2]:print('Case',i,cases[i],'\nRust:',expected[i],'\nC++:',actual[i])
    print(f'Generated result aliases: {len(cases)-len(failures)}/{len(cases)} exact original observations')
    return bool(failures)


if __name__=='__main__':raise SystemExit(main())
