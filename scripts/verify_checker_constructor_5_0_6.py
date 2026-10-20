#!/usr/bin/env python3
"""Verify staged TypedEntityChecker construction, erasure and Context retention."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
import xml.etree.ElementTree as ET

from verify_constant_closure_5_0_6 import registry_identities

ROOT = Path(__file__).resolve().parents[1]
PROJECT = ROOT/'tests/parity/rust_checker_source_5_0_6'
TESTS = ('checker_constructor_contracts','checker_context_contracts','checker_reentrant_close',
         'checker_registration_reentrant_close','checker_typed_direct_reentrant_close')


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record',type=Path,required=True)
    parser.add_argument('--sanitize',action='store_true')
    args = parser.parse_args()
    args.record = args.record.resolve()
    if args.record.exists():
        parser.error('Use a fresh receipt path')
    record = dict(complete=False,full_goal_accepted=False,steps=[],profiles={},
                  scope='Shared constructor/erasure repair; complete Checker source and generator exits remain separate.')
    env = dict(os.environ,CARGO_TARGET_DIR='/tmp/teaql_cpp_core_baselines_506',
               CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc',TEAQL_LOG_ENDPOINT='off',
               ASAN_OPTIONS='detect_leaks=0:halt_on_error=1',
               UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')

    def save():
        args.record.write_bytes(gzip.compress((json.dumps(record,indent=2)+'\n').encode(),
                                             compresslevel=1,mtime=0))

    def run(command, succeeds=True):
        command = [str(part) for part in command]
        result = subprocess.run(command,cwd=ROOT,env=env,text=True,capture_output=True)
        record['steps'].append(dict(command=command,exit_code=result.returncode,
                                    stdout=result.stdout,stderr=result.stderr))
        save()
        assert (result.returncode==0)==succeeds,(command,result.returncode,result.stderr[-3000:])
        return result.stdout if succeeds else result.stderr

    run(['python3','scripts/verify_upstream.py'])
    assert registry_identities(PROJECT/'Cargo.lock') <= registry_identities(ROOT/'.reference/teaql-rs/Cargo.lock')
    source = run(['cargo','run','--offline','--locked','--manifest-path',PROJECT/'Cargo.toml',
                  '--bin','checker_constructor_source_506'])
    assert source=='opaque_alias:true\nconstructors:5\nchecker_name:Entity\n'
    record['original_constructor_output'] = source
    cxx = ROOT/'.tools/toolchains/llvm-23.1.3/bin/clang++'
    command = [cxx,'-std=c++20','-fsyntax-only']+['-I'+str(p) for p in ROOT.glob('teaql_*/include')]
    fixture = ROOT/'tests/parity/checker_constructor_contracts.cpp'
    run([*command,fixture])
    negatives = ('metadata','logic','clone','constructor')
    for feature in negatives:
        error = run(['cargo','check','--offline','--locked','--manifest-path',PROJECT/'Cargo.toml',
                     '--bin','checker_constructor_source_506','--features','reject_'+feature],False)
        assert ('TeaqlEntity' if feature=='constructor' else 'Checker') in error
        error = run([*command,'-DTEAQL_REJECT_'+feature.upper(),fixture],False)
        assert 'TypedEntityChecker' in error
        if feature!='constructor':
            assert 'constraints not satisfied' in error
    record['compiler_controls'] = dict(original_positive=1,native_positive=1,
                                       original_negative=len(negatives),native_negative=len(negatives))
    for profile in ['native','modules_native']+(['sanitize'] if args.sanitize else []):
        modules = profile=='modules_native'
        prefix = 'teaql_module_' if modules else 'teaql_'
        build = ROOT/('build_modules_native' if modules else
                      'build_native_current' if profile=='native' else 'build_sanitize_current')
        binaries = build/'modules' if modules else build
        run(['cmake','--preset',profile])
        targets = [prefix+'checker_constructor_tests',prefix+'checker_tests']
        run(['cmake','--build',build,'--target',*targets,'--parallel','4'])
        output = run([binaries/(prefix+'checker_constructor_tests')])
        assert output==source
        selected = [('module_' if modules else '')+name for name in TESTS]
        junit = args.record.with_name(args.record.name+'.'+profile+'.xml')
        run(['ctest','--test-dir',build,'--no-tests=error','--output-on-failure','--output-junit',junit,
             '-R','^('+'|'.join(selected)+')$'])
        checks = ET.parse(junit).getroot().findall('.//testcase')
        assert len(checks)==len(selected) and {row.attrib['name'] for row in checks}==set(selected)
        assert not any(row.find(tag) is not None for row in checks for tag in ('failure','error','skipped'))
        assert 'no work to do' in run(['cmake','--build',build,'--target',*targets,'--parallel','4'])
        record['profiles'][profile] = dict(passed=len(checks),junit=junit.read_text(),
                                          constructor_binary_sha256=digest(binaries/(prefix+'checker_constructor_tests')))
        save()
        print(profile,'source constructor output matches; 5 ownership/dispatch checks',flush=True)
    run(['python3','scripts/verify_generated_type_errors.py','--build-dir','build_native_current',
         '--cases','checker_customization_positive','checker_wrong_logic','checker_wrong_adapter',
         'checker_wrong_entity','checker_wrong_callback','--jobs','2'])
    run(['cmake','--build',ROOT/'build_native_current','--target',
         'teaql_generated_checker_flow_probe','--parallel','4'])
    for comparator in ('compare_generated_checker_flow.py','compare_generated_checker_commit.py'):
        run(['python3',ROOT/'tests/parity'/comparator,'--cpp',
             ROOT/'build_native_current/teaql_generated_checker_flow_probe'])
    assert 'no work to do' in run(['cmake','--build',ROOT/'build_native_current','--target',
                                  'teaql_generated_checker_flow_probe','--parallel','4'])
    record['retained_generated_integration'] = dict(flow=1620,governed_sqlite_save=540,
                                                   scope='Current native regression against retained source corpora; no new generator/source replay.')
    paths = [ROOT/'teaql_runtime/include/teaql/runtime/checker.hpp',fixture,
             ROOT/'tests/parity/checker_constructor_source_5_0_6.rs',
             ROOT/'scripts/verify_generated_type_errors.py',Path(__file__).resolve()]
    record.update(complete=True,sanitizer_leak_scanning=False,
                  sources={str(p.relative_to(ROOT)):digest(p) for p in paths})
    save()


if __name__=='__main__':
    main()
