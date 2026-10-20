#!/usr/bin/env python3
"""Replay unchanged Context observers and original tests through both frontends."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET

from reference_runtime_view_5_0_6 import ReferenceRuntimeView
from verify_constant_closure_5_0_6 import registry_identities

ROOT = Path(__file__).resolve().parents[1]
PARITY = ROOT/'tests/parity'
PROJECT = PARITY/'rust_context_source_5_0_6'
sys.path.insert(0, str(PARITY))
from compare_values import same

MATRICES = (
    ('context_defaults', 'context_defaults_5_0_6_cases.json.gz', 75),
    ('context_model', 'context_model_5_0_6_cases.json.gz', 234),
    ('runtime_context', 'runtime_context_baseline_5_0_6_cases.json.gz', 410),
    ('checker', 'checker_5_0_6_cases.json.gz', 1362),
    ('data_store', 'data_store_5_0_6_cases.json.gz', 251),
    ('business_service', 'business_service_baseline_5_0_6_cases.json.gz', 859),
    ('internal_id', 'internal_id_5_0_6_cases.json.gz', 171),
)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path, required=True)
    parser.add_argument('--modules', action='store_true')
    args=parser.parse_args()
    args.record=args.record.resolve()
    if args.record.exists():
        parser.error('Use a fresh receipt path')
    record=dict(complete=False, whole_context_accepted=False, full_goal_accepted=False,
                steps=[], matrices=[], profiles={}, scope='Context source observer, original '
                'test and canonical frontend continuation. Whole-source/capability exits remain separate.')
    env=dict(os.environ, CARGO_TARGET_DIR='/tmp/teaql_cpp_core_baselines_506',
             CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc', TEAQL_LOG_ENDPOINT='off')

    def save():
        args.record.write_bytes(gzip.compress((json.dumps(record,indent=2)+'\n').encode(),
                                             compresslevel=1,mtime=0))

    def run(command,payload=None):
        command=[str(value) for value in command]
        result=subprocess.run(command,cwd=ROOT,env=env,input=payload,text=True,capture_output=True)
        record['steps'].append(dict(command=command,exit_code=result.returncode,
                                    stdout=result.stdout if payload is None else None,
                                    stdout_sha256=hashlib.sha256(result.stdout.encode()).hexdigest(),
                                    stderr=result.stderr))
        save();result.check_returncode();return result.stdout

    run(['python3','scripts/verify_upstream.py'])
    assert registry_identities(PROJECT/'Cargo.lock') <= registry_identities(ROOT/'.reference/teaql-rs/Cargo.lock')
    run(['cargo','build','--offline','--locked','--manifest-path',PROJECT/'Cargo.toml','--bins'])
    with tempfile.TemporaryDirectory(prefix='teaql_context_original_source_506_') as directory:
        view=ReferenceRuntimeView(ROOT,Path(directory)/'runtime',{},run)
        registry=run(['cargo','test','--offline','--locked','--manifest-path',view.directory/'Cargo.toml',
                      '--lib','--','--list'])
        assert 'generated_support_source_contracts_506' not in registry
        original_names=re.findall(r'^([^ ]+): test$',registry,re.M)
        assert len(original_names)==273
        record['original_test_registry']=original_names
        names=[]
        for selector,count in [('context::active_root_tests::',1),
                               ('context::sql_log_option_tests::',4),
                               ('context::entity_runtime_state_tests::',1)]:
            output=run(['cargo','test','--offline','--locked','--manifest-path',view.directory/'Cargo.toml',
                        '--lib',selector,'--','--nocapture','--test-threads=1'])
            observed=re.findall(r'^test (context::[^ ]+) \.\.\. ok$',output,re.M)
            assert len(observed)==count and f'{count} passed; 0 failed; 0 ignored;' in output
            names.extend(observed)
        record['original_context_tests']=names
        record['original_runtime_view']=view.provenance()
    baseline=json.loads((ROOT/'upstream.lock.json').read_text())['commit']
    for stem,filename,count in MATRICES:
        path=PARITY/filename;corpus=json.loads(gzip.decompress(path.read_bytes()))
        assert corpus['baseline']==baseline and len(corpus['cases'])==count
        if corpus.get('driver'):
            assert digest(ROOT/corpus['driver'])==corpus['driver_sha256']
        values=[case['input'] for case in corpus['cases']]
        payload=''.join(json.dumps(value)+'\n' for value in values)
        original=[json.loads(line) for line in run([Path(env['CARGO_TARGET_DIR'])/'debug'/
                  (stem+'_context_source_506')],payload).rstrip('\n').split('\n')]
        assert same(original,[case['expected'] for case in corpus['cases']]),stem
        record['matrices'].append(dict(name=stem,cases=count,inputs=values,original=original,
                                        corpus=filename,corpus_sha256=digest(path),frontends={}))
        save()
    units=['context_defaults','context_model','runtime_context','data_store','checker']
    names=[stem+'_rust_parity' for stem,_,_ in MATRICES]
    names += ['context_defaults_contracts','context_model_contracts','runtime_context_resource_contracts',
              'data_store_contracts','checker_context_contracts','checker_reentrant_close',
              'checker_registration_reentrant_close','checker_typed_direct_reentrant_close',
              'original_runtime_context_contracts','original_runtime_context_tail_contracts']
    for profile in ['native']+(['modules_native'] if args.modules else []):
        modules=profile=='modules_native';prefix='teaql_module_' if modules else 'teaql_'
        build=ROOT/('build_modules_native' if modules else 'build_native_current')
        run(['cmake','--preset',profile])
        targets=[prefix+stem+'_probe' for stem,_,_ in MATRICES]+[prefix+stem+'_tests' for stem in units]
        targets += [prefix+'original_context_tests',prefix+('original_runtime_context_tail' if modules
                                                             else 'original_runtime_context_tail_tests')]
        run(['cmake','--build',build,'--target',*targets,'--parallel','4'])
        for matrix in record['matrices']:
            binary=(build/'modules' if modules else build)/(prefix+matrix['name']+'_probe')
            payload=''.join(json.dumps(value)+'\n' for value in matrix['inputs'])
            actual=[json.loads(line) for line in run([binary],payload).rstrip('\n').split('\n')]
            differences=[dict(index=i,input=matrix['inputs'][i],original=a,actual=b)
                         for i,(a,b) in enumerate(zip(matrix['original'],actual)) if not same(a,b)]
            matrix['frontends'][profile]=dict(binary_sha256=digest(binary),outputs=actual,differences=differences)
            save();assert len(actual)==matrix['cases'] and not differences,(profile,matrix['name'])
        selected=[('module_original_runtime_context_tail'
                   if modules and name=='original_runtime_context_tail_contracts'
                   else ('module_' if modules else '')+name) for name in names]
        junit=args.record.with_name(args.record.name+'.'+profile+'.xml')
        run(['ctest','--test-dir',build,'--no-tests=error','--output-on-failure','--output-junit',junit,
             '-R','^('+'|'.join(selected)+')$'])
        checks=ET.parse(junit).getroot().findall('.//testcase')
        assert {check.attrib['name'] for check in checks}==set(selected)
        assert not any(check.find(tag) is not None for check in checks for tag in ('failure','error','skipped'))
        record['profiles'][profile]=dict(passed=len(checks),junit=junit.read_text())
        assert 'no work to do' in run(['cmake','--build',build,'--target',*targets,'--parallel','4'])
        save();print(profile,'3362 exact source observations; 17/17 checks; final no-work build',flush=True)
    paths=[PROJECT/'Cargo.toml',PROJECT/'Cargo.lock',Path(__file__).resolve(),
           ROOT/'modules/contracts.json',ROOT/'modules/CMakeLists.txt',ROOT/'CMakePresets.json',
           ROOT/'.reference/teaql-rs/teaql-runtime/src/context.rs']
    record.update(complete=True,observations_per_frontend=3362,normalization='none',
                  hashes={str(path.relative_to(ROOT)):digest(path) for path in paths})
    save()


if __name__=='__main__':main()
