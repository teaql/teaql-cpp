#!/usr/bin/env python3
"""Qualify full generated batches and explicit Context ownership adaptations."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--profile',choices=['native','modules_native','sanitize'],required=True)
    parser.add_argument('--record',type=Path,required=True)
    args=parser.parse_args()
    if args.record.exists():
        parser.error('Use a fresh receipt')
    module=args.profile=='modules_native'
    record=dict(scope='Typed small/large batch and lifetime/source checkpoint; whole G15 remains separate',
                complete=False,profile=args.profile,steps=[])

    def save():
        args.record.write_bytes(gzip.compress((json.dumps(record,indent=2)+'\n').encode(),mtime=0))

    def run(command,environment=None):
        result=subprocess.run([str(x) for x in command],cwd=ROOT,env=environment,text=True,capture_output=True)
        record['steps'].append(dict(command=[str(x) for x in command],exit_code=result.returncode,
                                    stdout=result.stdout,stderr=result.stderr))
        save()
        print('exit',result.returncode,' '.join(str(x) for x in command[:5]),flush=True)
        result.check_returncode()
        return result.stdout

    run(['python3','scripts/verify_upstream.py'])
    run(['cmake','--preset',args.profile])
    stems=['generated_load_state_bulk_probe','generated_member_alias_probe','context_reference_tests']
    names=['generated_load_state_bulk_contracts','generated_member_alias_contracts','context_reference_contracts']
    if args.profile!='sanitize':
        stems.append('runtime_expression_tests')
        names.extend(['generated_load_state_bulk_rust_parity','generated_member_alias_rust_parity',
                      'runtime_entity_expression_contracts'])
    target_prefix='teaql_module_' if module else 'teaql_'
    targets=[target_prefix+stem for stem in stems]
    run(['cmake','--build','--preset',args.profile,'--parallel','2','--target',*targets])
    environment=dict(os.environ)
    if args.profile=='sanitize':
        environment.update(ASAN_OPTIONS='detect_leaks=0:abort_on_error=1',UBSAN_OPTIONS='halt_on_error=1')
        record['sanitizer_environment']={k:environment[k] for k in ['ASAN_OPTIONS','UBSAN_OPTIONS']}
    prefix='module_' if module else ''
    pattern='^('+'|'.join(prefix+name for name in names)+')$'
    output=run(['ctest','--preset',args.profile,'--output-on-failure','-R',pattern],environment)
    assert re.search(rf'100% tests passed out of {len(names)}\b',output),output
    record['passed']=len(names)
    rust_env=dict(os.environ,CARGO_TARGET_DIR='/tmp/teaql_cpp_field_layout_rust_506',
                  CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')
    run(['cargo','build','--offline','--locked','--manifest-path','tests/parity/rust_field_layout/Cargo.toml',
         '--bin','generated_load_state_bulk_5_0_6'],rust_env)
    build=ROOT/{'native':'build_native_current','modules_native':'build_modules_native',
                'sanitize':'build_sanitize_current'}[args.profile]
    binary=build/('modules' if module else '')/(target_prefix+'generated_load_state_bulk_probe')
    comparison=args.record.with_name(args.record.name.replace('.json.gz','_source.json.gz'))
    run(['python3','tests/parity/compare_generated_load_state_bulk_5_0_6.py','--cpp',binary,
         '--rust','/tmp/teaql_cpp_field_layout_rust_506/debug/generated_load_state_bulk_5_0_6',
         '--record',comparison],environment)
    record['actual_source']=dict(path=str(comparison),sha256=digest(comparison),batches=12,rows=40404)
    output=run(['cmake','--build','--preset',args.profile,'--parallel','2','--target',*targets])
    assert 'no work to do' in output
    paths=[ROOT/'tests/parity/generated_load_state_bulk_probe.cpp',
           ROOT/'tests/parity/rust_field_layout/src/bin/generated_load_state_bulk_5_0_6.rs',
           ROOT/'tests/parity/generated_load_state_bulk_5_0_6_cases.json.gz',
           ROOT/'tests/parity/compare_generated_load_state_bulk_5_0_6.py',
           Path(__file__).resolve(),binary]
    record['hashes']={str(path):digest(path) for path in paths}
    record['complete']=True
    save()
    print('Qualified',args.profile,len(names),'checks;12 full batches/40,404 rows',flush=True)


if __name__=='__main__':
    main()
