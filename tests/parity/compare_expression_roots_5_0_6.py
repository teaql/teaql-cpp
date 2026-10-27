#!/usr/bin/env python3
"""Compare root identity and actual generated C++ expression-entry presentation."""
import argparse
import gzip
import hashlib
import itertools
import json
import os
from pathlib import Path
import re
import sys
from compare_values import observe,same
from compare_generated_expression_diagnostics import adapt
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts'))
from verify_constant_closure_5_0_6 import registry_identities
BASE=ROOT/'tests/generator/parity/expression_root_contracts'
CORPUS=Path(__file__).with_name('expression_root_5_0_6_cases.json.gz')


def inputs():
    return [dict(entity=entity,selection=selection,path=path,id=identity,deleted=deleted,action=action)
            for entity,selection,path,identity,deleted,action in itertools.product(
                ['class','operator','template','namespace','plain_record'],
                ['empty','name','removed','replaced','platform_minimal','platform_named','self'],
                ['name','platform'],[10,12],[False,True],['eval','unwrap'])]


def identity():
    provenance=json.loads((BASE/'rust/PROVENANCE.json').read_text())
    for name,digest in provenance['files'].items():
        assert hashlib.sha256((BASE/'rust'/name).read_bytes()).hexdigest()==digest,name
    assert provenance['runtime_commit']==json.loads((ROOT/'upstream.lock.json').read_text())['commit']
    assert hashlib.sha256((ROOT/provenance['model']).read_bytes()).hexdigest()==provenance['model_sha256']
    project=ROOT/'tests/parity/rust_expression_roots_5_0_6'
    assert registry_identities(project/'Cargo.lock')<=registry_identities(ROOT/'.reference/teaql-rs/Cargo.lock')
    paths=[project/name for name in ['Cargo.toml','Cargo.lock','src/main.rs']]
    paths += [ROOT/'tests/support/expression_root_sql.sql',ROOT/'tests/parity/rust_field_layout/src/bin/original_like_privacy_5_0_6.rs']
    generated=(BASE/'cpp/include/cpp_expression_roots/expressions.hpp').read_text()
    entries=dict(re.findall(r'static\s+(\w+)Expression\s+(\w+)\(const\s+\1&\s+value\)',generated))
    assert len(entries)==6,entries
    return dict(producer=provenance,native_entries=entries,sources={str(path.relative_to(ROOT)):hashlib.sha256(path.read_bytes()).hexdigest() for path in paths})


def adapted(output,entries):
    if output['diagnostic'] is None:return output
    result=output.copy()
    diagnostic=adapt(output['diagnostic'])
    root=diagnostic['root'].split('(')[0]
    assert root in entries,root
    expression=diagnostic['original_expr_with_broken_point']
    diagnostic['original_expr_with_broken_point']=re.sub(r'^E::\w+(?=\()',lambda _: 'E::'+entries[root],expression,count=1)
    result['diagnostic']=diagnostic
    return result


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp');parser.add_argument('--rust');parser.add_argument('--record-source',action='store_true')
    parser.add_argument('--record-comparison',type=Path)
    args=parser.parse_args();os.environ['TEAQL_LOG_ENDPOINT']='off'
    current,cases=identity(),inputs()
    if args.record_source:
        assert args.rust and not CORPUS.exists()
        expected=observe(args.rust,cases)
        CORPUS.write_bytes(gzip.compress(json.dumps(dict(**current,cases=[dict(input=i,expected=e) for i,e in zip(cases,expected)]),ensure_ascii=False).encode(),mtime=0))
    else:
        corpus=json.loads(gzip.decompress(CORPUS.read_bytes()))
        for key,value in current.items():assert corpus[key]==value,key
        assert same(cases,[row['input'] for row in corpus['cases']])
        expected=[row['expected'] for row in corpus['cases']]
        if args.rust:
            if not same(expected,observe(args.rust,cases)):
                raise RuntimeError('Original expression-root outcomes changed')
    if not args.cpp:
        print('Recorded actual original expression-root observations:',len(cases));return 0
    actual=observe(args.cpp,cases)
    failures=[i for i,(a,b) in enumerate(zip(expected,actual)) if not same(adapted(a,current['native_entries']),b)]
    if args.record_comparison:
        assert not args.record_comparison.exists()
        args.record_comparison.write_bytes(gzip.compress(json.dumps(dict(inputs=cases,original=expected,cpp=actual,failures=failures),ensure_ascii=False).encode(),mtime=0))
    for index in failures[:2]:
        print('Case',index,cases[index]);print('Different fields:',[key for key in expected[index] if not same(adapted(expected[index],current['native_entries'])[key],actual[index][key])])
    print(f'Generated expression roots: {len(cases)-len(failures)}/{len(cases)} actual original outcomes agree (explicit native entry presentation)')
    return bool(failures)


if __name__=='__main__':raise SystemExit(main())
