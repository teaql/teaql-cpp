#!/usr/bin/env python3
"""Replay the exact native relation gate commands using canonical module binaries."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
from verify_relation_native_closure_5_0_6 import ROOT, TARGETS, TESTS, inputs, digest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        raise RuntimeError('Preserve prior receipts')
    native_path = ROOT/'docs/parity/relation_native_closure_5_0_6.json.gz'
    native = json.loads(gzip.decompress(native_path.read_bytes()))
    assert native['complete'] and native['source_sha256'] == inputs()
    bound = {name:digest(ROOT/name) for name in ['modules/contracts.json','CMakePresets.json']}
    record = dict(complete=False, source_promoted=False, steps=[], source_sha256=inputs(),
                  module_inputs=bound, native_receipt_sha256=digest(native_path),
                  controller=Path(__file__).read_text(), tests=TESTS, targets=TARGETS,
                  free_bytes=os.statvfs(ROOT).f_bavail*os.statvfs(ROOT).f_frsize)

    def save():
        args.output.write_bytes(gzip.compress((json.dumps(record,indent=2)+'\n').encode(),mtime=0))

    def run(command, timeout=None):
        command=list(map(str,command))
        print('Running:', ' '.join(command),flush=True)
        result=subprocess.run(command,cwd=ROOT,env=dict(os.environ,TEAQL_LOG_ENDPOINT='off'),
                              text=True,capture_output=True,timeout=timeout)
        record['steps'].append(dict(command=command,exit_code=result.returncode,
                                    stdout=result.stdout,stderr=result.stderr))
        save()
        print('Exit:',result.returncode,flush=True)
        result.check_returncode()
        return result.stdout

    try:
        run(['python3','scripts/verify_upstream.py'])
        run(['cmake','--build','--preset','modules_native','--target',
             *['teaql_module_'+name for name in TARGETS],'-j','2'])
        pattern='^('+'|'.join(map(re.escape,TESTS))+')$'
        selection=json.loads(run(['ctest','--test-dir','build_native_current','--show-only=json-v1','-R',pattern]))
        assert sorted(test['name'] for test in selection['tests'])==sorted(TESTS)
        replacements={str(ROOT/'build_native_current'/('teaql_'+name)):
                      str(ROOT/'build_modules_native/modules'/('teaql_module_'+name)) for name in TARGETS}
        record['passed_tests']=[]
        for test in selection['tests']:
            command=[replacements.get(arg,arg) for arg in test['command']]
            assert sum(arg in replacements for arg in test['command'])==1,test['name']
            properties={p['name']:p['value'] for p in test['properties']}
            assert not properties.get('ENVIRONMENT') or properties['ENVIRONMENT']==['TEAQL_LOG_ENDPOINT=off']
            run(command,properties.get('TIMEOUT',180))
            record['passed_tests'].append(test['name'])
            save()
        run(['python3','scripts/verify_module_consumers.py','--build-dir','build_modules_native'])
        run(['python3','scripts/verify_module_paths.py'])
        assert inputs()==record['source_sha256']
        assert all(digest(ROOT/name)==value for name,value in bound.items())
        record['complete']=True
        save()
        print('Module relation closure: 26/26 equivalent commands pass; no source promotion',flush=True)
    except BaseException as error:
        record['error']=repr(error)
        save()
        raise


if __name__ == '__main__':
    main()
