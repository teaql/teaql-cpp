#!/usr/bin/env python3
"""Replay actual checked row merges and transient shape/conversion sequences."""
import argparse
import gzip
import hashlib
import itertools
import json
import os
from pathlib import Path
import subprocess
import sys
from verify_constant_closure_5_0_6 import registry_identities
ROOT=Path(__file__).resolve().parents[1]
PARITY=ROOT/'tests/parity'
PROJECT=PARITY/'rust_query_full_state_5_0_6'
sys.path.insert(0,str(PARITY))
from compare_values import same
from row_enhancement_inputs_5_0_6 import cases


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir',type=Path,default=ROOT/'build_native_current')
    parser.add_argument('--module-build-dir',type=Path)
    parser.add_argument('--record-corpus',action='store_true')
    parser.add_argument('--record-constructor-corpus',action='store_true')
    parser.add_argument('--record',type=Path,required=True)
    args=parser.parse_args()
    corpus_path=PARITY/'row_enhancement_5_0_6_cases.json.gz'
    constructor_path=PARITY/'row_constructor_5_0_6_cases.json.gz'
    if args.record_constructor_corpus and constructor_path.exists():parser.error('Do not overwrite constructor source observations')
    if args.record.exists() or (args.record_corpus and corpus_path.exists()):parser.error('Use fresh evidence paths')
    record=dict(complete=False,full_goal_accepted=False,whole_source_accepted=False,steps=[],
                scope='Original checked enhancement failure order and receiver isolation; transient '
                'relation-cache passes, schema attachment, shared shapes, insert/remove and consuming map conversion. '
                'C++ explicitly retains result rows across actual source UserContext deletion.')
    def save():args.record.write_bytes(gzip.compress((json.dumps(record,indent=2)+'\n').encode(),compresslevel=1,mtime=0))
    environment=dict(os.environ,CARGO_TARGET_DIR='/tmp/teaql_cpp_core_baselines_506',
                     CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc',TEAQL_LOG_ENDPOINT='off')
    def run(command,payload=None):
        q=subprocess.run([str(p) for p in command],cwd=ROOT,env=environment,input=payload,text=True,capture_output=True)
        record['steps'].append(dict(command=[str(p) for p in command],exit_code=q.returncode,stdout=q.stdout,stderr=q.stderr))
        if payload is None or q.returncode:save()
        q.check_returncode();return q.stdout
    run(['python3',ROOT/'scripts/verify_upstream.py'])
    baseline=json.loads((ROOT/'upstream.lock.json').read_text())['commit']
    assert registry_identities(PROJECT/'Cargo.lock') <= registry_identities(ROOT/'.reference/teaql-rs/Cargo.lock')
    run(['cargo','build','--offline','--locked','--manifest-path',PROJECT/'Cargo.toml','--example','row_enhancement','--example','row_constructor'])
    inputs=cases();assert len(inputs)==1373
    expected=None
    if not args.record_corpus:
        corpus=json.loads(gzip.decompress(corpus_path.read_bytes()))
        assert corpus['baseline']==baseline and [c['input'] for c in corpus['cases']]==inputs
        expected=[c['expected'] for c in corpus['cases']]
    record['inputs']=inputs
    payload=''.join(json.dumps(v)+'\n' for v in inputs)
    executables=[('original',Path(environment['CARGO_TARGET_DIR'])/'debug/examples/row_enhancement'),
                 ('native',args.build_dir.resolve()/'teaql_row_enhancement_probe')]
    if args.module_build_dir:executables.append(('modules',args.module_build_dir.resolve()/'modules/teaql_module_row_enhancement_probe'))
    gaps=[]
    for label,executable in executables:
        output=run([executable],payload);assert output.endswith('\n')
        actual=[json.loads(line) for line in output.split('\n')[:-1]];assert len(actual)==len(inputs)
        if expected is None:expected=actual
        for i,(a,b) in enumerate(zip(expected,actual)):
            if label=='original' and inputs[i]['kind']=='merge':
                assert b['original']==b['left_before'] and b['right_after']==b['right_before']
                if b['error'] is not None:assert b['left_unchanged'] and b['layout_same']
            if not same(a,b):gaps.append(dict(frontend=label,index=i,input=inputs[i],expected=a,actual=b))
        print(label,len(actual),'observations',flush=True)
    corpus=dict(baseline=baseline,cases=[dict(input=i,expected=o) for i,o in zip(inputs,expected)])
    compressed=gzip.compress((json.dumps(corpus,separators=(',',':'))+'\n').encode(),mtime=0)
    if args.record_corpus:corpus_path.write_bytes(compressed)
    else:assert corpus_path.read_bytes()==compressed,'Source corpus bytes changed'
    constructor_inputs=[dict(kind='constructor',columns=a,values=b,entry=e)
                        for a,b,e in itertools.product(range(4),range(4),('new','with_layout'))]
    constructor_expected=None
    if not args.record_constructor_corpus:
        previous=json.loads(gzip.decompress(constructor_path.read_bytes()))
        assert previous['baseline']==baseline and [c['input'] for c in previous['cases']]==constructor_inputs
        constructor_expected=[c['expected'] for c in previous['cases']]
    constructor_payload=''.join(json.dumps(v)+'\n' for v in constructor_inputs)
    for label,executable in executables:
        if label=='original':executable=executable.parent/'row_constructor'
        output=run([executable],constructor_payload);assert output.endswith('\n')
        actual=[json.loads(line) for line in output.split('\n')[:-1]];assert len(actual)==32
        if constructor_expected is None:constructor_expected=actual
        for i,(a,b) in enumerate(zip(constructor_expected,actual)):
            if not same(a,b):gaps.append(dict(frontend=label,index=i,input=constructor_inputs[i],expected=a,actual=b))
        print(label,'32 constructor observations',flush=True)
    constructor_corpus=dict(baseline=baseline,cases=[dict(input=i,expected=o) for i,o in zip(constructor_inputs,constructor_expected)])
    constructor_compressed=gzip.compress((json.dumps(constructor_corpus,separators=(',',':'))+'\n').encode(),mtime=0)
    if args.record_constructor_corpus:constructor_path.write_bytes(constructor_compressed)
    else:assert constructor_path.read_bytes()==constructor_compressed,'Constructor source corpus bytes changed'
    paths=['teaql_core/src/row.cpp','teaql_core/include/teaql/core/row.hpp','teaql_core/include/teaql/core/debug.hpp',
           'tests/parity/row_enhancement_probe.cpp','tests/parity/row_enhancement_inputs_5_0_6.py',
           'tests/parity/debug_modes.hpp','tests/parity/fixture_value.hpp','tests/parity/rust_values/src/fixture_value.rs',
           'tests/parity/rust_query_full_state_5_0_6/examples/row_enhancement.rs',
           'tests/parity/rust_query_full_state_5_0_6/examples/row_constructor.rs',
           'tests/parity/rust_query_full_state_5_0_6/Cargo.toml','tests/parity/rust_query_full_state_5_0_6/Cargo.lock',
           '.reference/teaql-rs/teaql-core/src/query.rs','.reference/teaql-rs/Cargo.lock',str(Path(__file__).relative_to(ROOT))]
    record.update(complete=not gaps,differences=gaps,observations=len(inputs),
                  constructor_observations=32,constructor_corpus_sha256=hashlib.sha256(constructor_path.read_bytes()).hexdigest(),
                  merge_observations=sum(c['kind']=='merge' for c in inputs),
                  shape_observations=sum(c['kind']=='shapes' for c in inputs),
                  record_observations=sum(c['kind']=='record' for c in inputs),
                  corpus_sha256=hashlib.sha256(corpus_path.read_bytes()).hexdigest(),
                  sources={p:dict(sha256=hashlib.sha256((ROOT/p).read_bytes()).hexdigest(),text=(ROOT/p).read_text()) for p in paths})
    save();assert not gaps,f'{len(gaps)} checked enhancement/shape outputs differ'
    print(len(inputs),'exact source outcomes per frontend',flush=True)


if __name__=='__main__':main()
