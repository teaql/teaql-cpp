#!/usr/bin/env python3
"""Requalify the whole memory query/evaluator source against exact dependencies."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
from verify_constant_closure_5_0_6 import registry_identities
ROOT=Path(__file__).resolve().parents[1]
PARITY=ROOT/'tests/parity'
PROJECT=PARITY/'rust_inmemory_source_5_0_6'
sys.path.insert(0,str(PARITY))
from compare_values import same
from inmemory_source_inputs_5_0_6 import cases


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir',type=Path,default=ROOT/'build_native_current')
    parser.add_argument('--module-build-dir',type=Path)
    parser.add_argument('--record-corpus',action='store_true')
    parser.add_argument('--record',type=Path,required=True)
    args=parser.parse_args()
    corpus_path=PARITY/'inmemory_source_5_0_6_cases.json.gz'
    if args.record.exists() or (args.record_corpus and corpus_path.exists()):parser.error('Use fresh evidence paths')
    record=dict(complete=False,whole_source_accepted=False,full_goal_accepted=False,steps=[],matrices=[],
        scope='Complete engine/evaluator production and nineteen original tests. Fresh source outcomes include typed-LIKE provenance and complete row read-metadata pipeline states. Absolute clock values are not compared; call brackets and ordering are observed. Current qualification is native64 only.')
    def save():args.record.write_bytes(gzip.compress((json.dumps(record,indent=2)+'\n').encode(),compresslevel=1,mtime=0))
    env=dict(os.environ,CARGO_TARGET_DIR='/tmp/teaql_cpp_core_baselines_506',
             CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc',TEAQL_LOG_ENDPOINT='off')
    def run(command,payload=None):
        q=subprocess.run([str(p) for p in command],cwd=ROOT,env=env,input=payload,text=True,capture_output=True)
        record['steps'].append(dict(command=[str(p) for p in command],exit_code=q.returncode,stdout=q.stdout,stderr=q.stderr))
        if payload is None or q.returncode:save()
        q.check_returncode();return q.stdout
    run(['python3',ROOT/'scripts/verify_upstream.py'])
    assert registry_identities(PROJECT/'Cargo.lock') <= registry_identities(ROOT/'.reference/teaql-rs/Cargo.lock')
    output=run(['cargo','test','--offline','--locked','--manifest-path',ROOT/'.reference/teaql-rs/teaql-runtime/Cargo.toml','--lib','inmemory_engine::tests::'])
    tests=re.findall(r'^test inmemory_engine::tests::([^ ]+) \.\.\. ok$',output,re.M)
    assert len(tests)==19 and '19 passed; 0 failed; 0 ignored;' in output
    record['original_tests']=tests
    run(['cargo','build','--offline','--locked','--manifest-path',PROJECT/'Cargo.toml','--bins'])
    baseline=json.loads((ROOT/'upstream.lock.json').read_text())['commit']
    retained_path=PARITY/'inmemory_baseline_5_0_6_cases.json.gz'
    retained=json.loads(gzip.decompress(retained_path.read_bytes()));assert retained['baseline']==baseline and retained['pointer_bits']==64
    matrices=[('retained',[c['input'] for c in retained['cases']],[c['expected'] for c in retained['cases']]),('source',cases(),None)]
    if not args.record_corpus:
        stored=json.loads(gzip.decompress(corpus_path.read_bytes()));assert stored['baseline']==baseline and [c['input'] for c in stored['cases']]==matrices[1][1]
        matrices[1]=(matrices[1][0],matrices[1][1],[c['expected'] for c in stored['cases']])
    for name,inputs,expected in matrices:
        assert len(inputs)==(6532 if name=='retained' else 2200)
        payload=''.join(json.dumps(i)+'\n' for i in inputs)
        stem='inmemory_probe' if name=='retained' else 'inmemory_source_probe'
        executables=[('original',Path(env['CARGO_TARGET_DIR'])/'debug'/('inmemory_retained_source_506' if name=='retained' else 'inmemory_source_506')),
                     ('native',args.build_dir.resolve()/('teaql_'+stem))]
        if args.module_build_dir:executables.append(('modules',args.module_build_dir.resolve()/'modules'/('teaql_module_'+stem)))
        matrix=dict(name=name,observations=len(inputs),inputs=inputs,outputs={},differences=[])
        for label,executable in executables:
            output=run([executable],payload);assert output.endswith('\n')
            actual=[json.loads(line) for line in output.split('\n')[:-1]];assert len(actual)==len(inputs)
            matrix['outputs'][label]=dict(step=len(record['steps'])-1,binary_sha256=hashlib.sha256(executable.read_bytes()).hexdigest())
            if expected is None:expected=actual
            for i,(a,b) in enumerate(zip(expected,actual)):
                if not same(a,b):matrix['differences'].append(dict(frontend=label,index=i,input=inputs[i],expected=a,actual=b))
            print(name,label,len(actual),'observed',flush=True)
        record['matrices'].append(matrix)
        if name=='source':
            corpus=dict(baseline=baseline,cases=[dict(input=i,expected=o) for i,o in zip(inputs,expected)])
            compressed=gzip.compress((json.dumps(corpus,separators=(',',':'))+'\n').encode(),mtime=0)
            if args.record_corpus:corpus_path.write_bytes(compressed)
            else:assert corpus_path.read_bytes()==compressed,'Exact new source corpus bytes changed'
        save();assert not matrix['differences'],f'{name}: actual source/frontend outputs differ'
    paths=['.reference/teaql-rs/teaql-runtime/src/inmemory_engine.rs','.reference/teaql-rs/Cargo.lock',
           'teaql_runtime/src/inmemory_engine.cpp','teaql_runtime/include/teaql/runtime/inmemory_engine.hpp',
           'teaql_runtime/tests/inmemory_tests.cpp','tests/parity/teaql_runtime/inmemory_engine.cpp',
           'tests/parity/inmemory_probe.cpp','tests/parity/inmemory_source_probe.cpp',
           'tests/parity/inmemory_source_5_0_6.rs','tests/parity/inmemory_source_inputs_5_0_6.py',
           'tests/parity/debug_modes.hpp','tests/parity/fixture_value.hpp',
           'tests/parity/rust_values/src/bin/inmemory_oracle.rs','tests/parity/rust_values/src/fixture_value.rs',
           'tests/parity/rust_inmemory_source_5_0_6/Cargo.toml','tests/parity/rust_inmemory_source_5_0_6/Cargo.lock',
           'scripts/verify_inmemory_source_5_0_6.py']
    record.update(complete=True,observations=8732,retained_corpus_sha256=hashlib.sha256(retained_path.read_bytes()).hexdigest(),corpus_sha256=hashlib.sha256(corpus_path.read_bytes()).hexdigest(),
                  sources={p:dict(sha256=hashlib.sha256((ROOT/p).read_bytes()).hexdigest(),text=(ROOT/p).read_text()) for p in paths})
    save();print('Whole-source candidate:8732 exact outputs per frontend,19 original tests; qualification remains separate.',flush=True)


if __name__=='__main__':main()
