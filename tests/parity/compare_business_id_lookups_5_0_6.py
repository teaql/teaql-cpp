#!/usr/bin/env python3
"""Compare original typed Q lookups with shared composition or generated factories."""
import argparse
from contextlib import redirect_stdout
import gzip
import hashlib
import importlib.util
import io
import itertools
import json
import os
from pathlib import Path
import sys
import tempfile
from compare_values import observe, same
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts'))
from verify_constant_closure_5_0_6 import registry_identities
from verify_state_baselines_5_0_6 import versions
CORPUS=Path(__file__).with_name('business_id_lookups_5_0_6_cases.json.gz')


def inherited_bridge_identity():
    """Reconstruct the guarded fixture without relying on a prior build tree."""
    builder = ROOT / 'tests/parity/build_inherited_business_binding_bridge_5_0_6.py'
    spec = importlib.util.spec_from_file_location('teaql_lookup_bridge_fixture', builder)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    with tempfile.TemporaryDirectory(prefix='teaql_lookup_bridge_506_') as directory:
        module.DESTINATION = Path(directory) / 'lib'
        with redirect_stdout(io.StringIO()):
            module.main()
        original = ROOT / 'tests/generator/parity/inheritance_business_ids_rust/lib'
        assert (module.DESTINATION / 'src/q.rs').read_bytes() == (original / 'src/q.rs').read_bytes(), \
            'Lookup source must remain original'
        return json.loads((Path(directory) / 'provenance.json').read_text())


def inputs():
    values=['match',' miss ','\u2003match\u2003','',' \t',' \u3000 ',"quoted'ID",'"','长号码','NUL\0id','x'*513,'match?']
    cases=[]
    for entity in ['receipt','ticket','document']:
        for text,operation,mode in itertools.product(values,['none','larger','offset','deleted'],
                ['construct'] if entity=='document' else ['construct','rows','list','count','one']):
            cases.append(dict(entity=entity,text=text,operation=operation,mode=mode))
    assert len(cases)==528
    return cases


def identity():
    reference=ROOT/'.reference/teaql-rs/Cargo.lock'
    baseline=json.loads((ROOT/'upstream.lock.json').read_text())['commit']
    wanted=versions(reference)
    projects=['rust_expression_business_ids_5_0_6','rust_inherited_business_binding_bridge_5_0_6']
    sources={}
    for name in projects:
        project=ROOT/'tests/parity'/name
        assert registry_identities(project/'Cargo.lock')<=registry_identities(reference)
        for package,values in versions(project/'Cargo.lock').items():
            if package not in {'teaql_expression_business_ids_506','teaql_cpp_inherited_business_binding_bridge',
                               'cpp-business-ids-core','cpp-inheritance-business-ids-core'}:
                assert values<=wanted.get(package,set()),package
        for p in [project/'Cargo.toml',project/'Cargo.lock',project/'src/bin/business_id_lookups.rs']:
            sources[str(p.relative_to(ROOT))]=hashlib.sha256(p.read_bytes()).hexdigest()
    provenance={}
    for name in ['business_ids_rust','inheritance_business_ids_rust']:
        sdk=ROOT/'tests/generator/parity'/name
        record=json.loads((sdk/'PROVENANCE.json').read_text())
        assert record['runtime_commit']==baseline
        for p,digest in record['files'].items():
            assert hashlib.sha256((sdk/p).read_bytes()).hexdigest()==digest,p
        assert hashlib.sha256((ROOT/record['model']).read_bytes()).hexdigest()==record['model_sha256']
        provenance[name]=record
    bridge_record=inherited_bridge_identity()
    for name in ['tests/support/business_id_lookup_rows.sql',
                 'tests/parity/rust_field_layout/src/bin/original_like_privacy_5_0_6.rs',
                 'tests/parity/build_inherited_business_binding_bridge_5_0_6.py']:
        sources[name]=hashlib.sha256((ROOT/name).read_bytes()).hexdigest()
    return dict(baseline=baseline,provenance=provenance,sources=sources,
                inherited_binding_bridge=bridge_record,
                adaptation='Document uses the existing qualified inherited model-binding bridge; q.rs and lookup mechanics are byte-identical to original. Not untouched whole-crate acceptance.')


def original(args,cases):
    assert args.rust_regular and args.rust_inherited
    regular=[case for case in cases if case['entity']!='document']
    inherited=[case for case in cases if case['entity']=='document']
    return observe(args.rust_regular,regular)+observe(args.rust_inherited,inherited)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp');parser.add_argument('--rust-regular');parser.add_argument('--rust-inherited')
    parser.add_argument('--runtime-composition',action='store_true')
    parser.add_argument('--record-source',action='store_true')
    parser.add_argument('--record-comparison',type=Path)
    args=parser.parse_args();os.environ['TEAQL_LOG_ENDPOINT']='off'
    os.environ['TEAQL_BUSINESS_ID_KEY_VERSION']='1'
    os.environ['TEAQL_BUSINESS_ID_KEY_V1_HEX']=bytes(range(32)).hex()
    current,cases=identity(),inputs()
    if args.record_source:
        assert not CORPUS.exists()
        expected=original(args,cases)
        for row in expected:
            if 'lookup' in row:assert row['lookup']==row['manual']
        CORPUS.write_bytes(gzip.compress(json.dumps(dict(**current,cases=[dict(input=i,expected=e)
            for i,e in zip(cases,expected)]),ensure_ascii=False).encode(),mtime=0))
    else:
        corpus=json.loads(gzip.decompress(CORPUS.read_bytes()))
        for key,value in current.items():assert corpus[key]==value,key
        assert same(cases,[row['input'] for row in corpus['cases']])
        expected=[row['expected'] for row in corpus['cases']]
        if args.rust_regular or args.rust_inherited:assert same(expected,original(args,cases))
    if not args.cpp:
        assert args.record_source
        print('Recorded original typed lookup outcomes:',len(cases));return 0
    actual=observe(args.cpp,cases,arguments=['--runtime-composition'] if args.runtime_composition else [])
    failures=[i for i,(a,b) in enumerate(zip(expected,actual)) if not same(a,b)]
    if args.record_comparison:
        assert not args.record_comparison.exists()
        args.record_comparison.write_bytes(gzip.compress(json.dumps(dict(inputs=cases,original=expected,
            cpp=actual,failures=failures,runtime_composition=args.runtime_composition),ensure_ascii=False).encode(),mtime=0))
    for i in failures[:3]:
        print('Case',i,cases[i]);print('Different fields:',[key for key in expected[i]
              if key not in actual[i] or not same(expected[i][key],actual[i][key])])
    print(f'Typed Business-ID lookup: {len(cases)-len(failures)}/{len(cases)} exact original outcomes')
    return bool(failures)


if __name__=='__main__':raise SystemExit(main())
