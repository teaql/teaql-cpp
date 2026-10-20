#!/usr/bin/env python3
"""Reconcile complete Checker contracts with exact source and canonical frontends."""
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
PROJECT = PARITY/'rust_checker_source_5_0_6'
sys.path.insert(0, str(PARITY))
from compare_values import same

MATRICES = (
    ('checker', 'checker_5_0_6_cases.json.gz', 1362),
    ('check_model', 'check_model_5_0_6_cases.json.gz', 220),
    ('runtime_check', 'runtime_check_baseline_5_0_6_cases.json.gz', 1134),
)
TESTS = (
    'checker_rust_parity', 'check_model_rust_parity', 'runtime_check_rust_parity',
    'checker_context_contracts', 'checker_reentrant_close', 'checker_registration_reentrant_close',
    'checker_typed_direct_reentrant_close', 'runtime_check_error_contracts', 'check_model_contracts',
    'original_runtime_checker_locale_contracts',
    'checker_constructor_contracts',
)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path, required=True)
    parser.add_argument('--modules', action='store_true')
    args = parser.parse_args()
    args.record = args.record.resolve()
    if args.record.exists():
        parser.error('Use a fresh evidence path')
    record = dict(complete=False, whole_checker_accepted=False, full_goal_accepted=False,
                  steps=[], matrices=[], profiles={}, scope='Exact source and frontend Checker gate; '
                  'whole-source/capability exits remain separate.')
    env = dict(os.environ, CARGO_TARGET_DIR='/tmp/teaql_cpp_core_baselines_506',
               CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc', TEAQL_LOG_ENDPOINT='off')

    def save():
        args.record.write_bytes(gzip.compress((json.dumps(record,indent=2)+'\n').encode(),
                                             compresslevel=1,mtime=0))

    def run(command, payload=None):
        command = [str(part) for part in command]
        result = subprocess.run(command,cwd=ROOT,env=env,input=payload,text=True,capture_output=True)
        record['steps'].append(dict(command=command,exit_code=result.returncode,
                                    stdout=result.stdout if payload is None else None,
                                    stdout_sha256=hashlib.sha256(result.stdout.encode()).hexdigest(),
                                    stderr=result.stderr))
        save()
        result.check_returncode()
        return result.stdout

    def observe(binary, inputs):
        output = run([binary], ''.join(json.dumps(value)+'\n' for value in inputs))
        lines = output.split('\n')
        if lines and not lines[-1]:
            lines.pop()
        values = [json.loads(line) for line in lines]
        assert len(values)==len(inputs), (binary,len(values),len(inputs))
        return values

    run(['python3','scripts/verify_upstream.py'])
    assert registry_identities(PROJECT/'Cargo.lock') <= registry_identities(ROOT/'.reference/teaql-rs/Cargo.lock')
    run(['cargo','build','--offline','--locked','--manifest-path',PROJECT/'Cargo.toml','--bins'])
    with tempfile.TemporaryDirectory(prefix='teaql_checker_original_506_') as directory:
        view = ReferenceRuntimeView(ROOT,Path(directory)/'runtime',{},run)
        output = run(['cargo','test','--offline','--locked','--manifest-path',
                      view.directory/'Cargo.toml','--lib','--','--list'])
        registry = re.findall(r'^([^ ]+): test$', output,re.M)
        assert len(registry)==273
        selected = [name for name in registry if name.startswith('checker::tests::')]
        assert len(selected)==5
        output = run(['cargo','test','--offline','--locked','--manifest-path',
                      view.directory/'Cargo.toml','--lib','checker::tests::','--',
                      '--nocapture','--test-threads=1'])
        names = re.findall(r'^test (checker::tests::[^ ]+) \.\.\. ok$',output,re.M)
        assert set(names)==set(selected) and '5 passed; 0 failed; 0 ignored;' in output
        record['original_test_registry'] = registry
        record['original_checker_tests'] = names
        record['original_runtime_view'] = view.provenance()
    baseline = json.loads((ROOT/'upstream.lock.json').read_text())['commit']
    matrices = []
    for stem,filename,count in MATRICES:
        path = PARITY/filename
        data = json.loads(gzip.decompress(path.read_bytes()))
        assert data['baseline']==baseline and len(data['cases'])==count
        driver = ROOT/data.get('driver',data.get('original_driver'))
        assert digest(driver)==data.get('driver_sha256',data.get('original_driver_sha256'))
        inputs = [case['input'] for case in data['cases']]
        binary = Path(env['CARGO_TARGET_DIR'])/'debug'/(stem+'_checker_source_506')
        original = observe(binary,inputs)
        assert same(original,[case['expected'] for case in data['cases']]),stem
        matrices.append((stem,inputs,original))
        record['matrices'].append(dict(name=stem,cases=count,corpus=filename,
                                        corpus_sha256=digest(path),driver_sha256=digest(driver),
                                        original_binary_sha256=digest(binary),frontends={}))
        save()
    for profile in ['native']+(['modules_native'] if args.modules else []):
        modules = profile=='modules_native'
        prefix = 'teaql_module_' if modules else 'teaql_'
        build = ROOT/('build_modules_native' if modules else 'build_native_current')
        binaries = build/'modules' if modules else build
        run(['cmake','--preset',profile])
        targets = [prefix+stem+'_probe' for stem,_,_ in MATRICES]
        targets += [prefix+stem for stem in ('checker_tests','check_model_tests','runtime_check_tests',
                                             'original_checker_locale_tests','checker_constructor_tests')]
        run(['cmake','--build',build,'--target',*targets,'--parallel','4'])
        for index,(stem,inputs,original) in enumerate(matrices):
            binary = binaries/(prefix+stem+'_probe')
            outputs = observe(binary,inputs)
            gaps = [dict(index=i,input=inputs[i],original=a,actual=b)
                    for i,(a,b) in enumerate(zip(original,outputs)) if not same(a,b)]
            record['matrices'][index]['frontends'][profile] = dict(binary_sha256=digest(binary),
                                                                  differences=gaps)
            save()
            assert not gaps,(profile,stem,len(gaps))
        selected = [('module_' if modules else '')+name for name in TESTS]
        junit = args.record.with_name(args.record.name+'.'+profile+'.xml')
        run(['ctest','--test-dir',build,'--no-tests=error','--output-on-failure','--output-junit',junit,
             '-R','^('+'|'.join(selected)+')$'])
        checks = ET.parse(junit).getroot().findall('.//testcase')
        assert len(checks)==len(selected) and {row.attrib['name'] for row in checks}==set(selected)
        assert not any(row.find(tag) is not None for row in checks for tag in ('failure','error','skipped'))
        record['profiles'][profile] = dict(passed=len(checks),junit=junit.read_text())
        assert 'no work to do' in run(['cmake','--build',build,'--target',*targets,'--parallel','4'])
        save()
        print(profile,'2716 exact source observations;',len(checks),'checks; final no-work build',flush=True)
    record.update(complete=True,observations_per_frontend=2716,normalization='none',
                  controller_sha256=digest(Path(__file__).resolve()),
                  project_hashes={str(p.relative_to(ROOT)):digest(p) for p in
                                  (PROJECT/'Cargo.toml',PROJECT/'Cargo.lock')})
    save()


if __name__=='__main__':
    main()
