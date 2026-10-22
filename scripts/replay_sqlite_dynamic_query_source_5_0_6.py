#!/usr/bin/env python3
"""Replay selected unchanged SQLite source tests with exact reference registry dependencies."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
from verify_constant_closure_5_0_6 import registry_identities

ROOT=Path(__file__).resolve().parents[1]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record',type=Path,required=True)
    parser.add_argument('--source', choices=['dynamic_field_query', 'ledger_allocated_trace'],
                        default='dynamic_field_query')
    args=parser.parse_args()
    if args.record.exists():
        parser.error('Preserve historical evidence')
    reference=ROOT/'.reference/teaql-rs'
    source_name=args.source
    expected_count={'dynamic_field_query':23,'ledger_allocated_trace':15}[source_name]
    source=reference/'teaql-provider-sqlite/tests'/f'{source_name}.rs'
    expected=re.findall(r'#\[test\]\s*fn (\w+)\(',source.read_text())
    assert len(expected)==expected_count
    record=dict(complete=False,source=str(source.relative_to(ROOT)),source_sha256=digest(source),
                reference_lock_sha256=digest(reference/'Cargo.lock'),expected_tests=expected,
                controller=Path(__file__).read_text(),steps=[],source_promoted=False)
    environment=dict(os.environ,CARGO_INCREMENTAL='0',CARGO_PROFILE_DEV_DEBUG='0',
                     CARGO_PROFILE_TEST_DEBUG='0',TEAQL_LOG_ENDPOINT='off',
                     CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')

    run_directory=ROOT

    def save():
        args.record.write_bytes(gzip.compress((json.dumps(record,indent=2)+'\n').encode(),mtime=0))

    def run(command):
        result=subprocess.run(list(map(str,command)),cwd=run_directory,env=environment,text=True,capture_output=True)
        record['steps'].append(dict(command=list(map(str,command)),exit_code=result.returncode,stdout=result.stdout,stderr=result.stderr))
        save();result.check_returncode();return result.stdout

    try:
        run(['python3','scripts/verify_upstream.py'])
        with tempfile.TemporaryDirectory(prefix=f'teaql_sqlite_{source_name}_source_') as temporary:
            work=Path(temporary);run_directory=work;environment['CARGO_TARGET_DIR']=str(work/'target')
            environment['TMPDIR']=str(work)  # Original persistent DB fixture belongs to this probe.
            shutil.copyfile(source,work/f'{source_name}.rs')
            manifest=['[package]',f'name="teaql_{source_name}_source_506"','version="0.0.0"','edition="2024"',
                      '[workspace]','[[test]]',f'name="{source_name}"',f'path="{source_name}.rs"','[dependencies]',
                      'futures-executor="=0.3.32"','futures-util="=0.3.32"',
                      'rusqlite={version="=0.40.1",features=["bundled","column_decltype","functions"]}']
            packages=['teaql-core','teaql-runtime','teaql-data-service','teaql-sql','teaql-provider-sqlite','teaql-macros']
            for package in packages:
                manifest.append(package+'={path='+json.dumps(str(reference/package))+'}')
            manifest.append('[patch.crates-io]')
            for package in packages:
                manifest.append(package+'={path='+json.dumps(str(reference/package))+'}')
            (work/'Cargo.toml').write_text('\n'.join(manifest)+'\n')
            shutil.copyfile(reference/'Cargo.lock',work/'Cargo.lock')
            run(['cargo','metadata','--offline','--format-version','1','--manifest-path',work/'Cargo.toml'])
            assert registry_identities(work/'Cargo.lock') <= registry_identities(reference/'Cargo.lock')
            record['resolved_lock']=(work/'Cargo.lock').read_text()
            command=['cargo','test','--offline','--locked','--manifest-path',work/'Cargo.toml','-j2','--test',source_name]
            listed=run([*command,'--','--list'])
            names=re.findall(r'^(\w+): test$',listed,re.M)
            assert sorted(names)==sorted(expected)
            result=run([*command,'--','--test-threads=1'])
            passed=re.findall(r'^test (\w+) \.\.\. ok$',result,re.M)
            assert sorted(passed)==sorted(expected) and f'{expected_count} passed; 0 failed; 0 ignored' in result
            assert digest(work/f'{source_name}.rs')==record['source_sha256']==digest(source)
            record['passed_tests']=passed
            record['owned_bytes_before_cleanup']=sum(p.stat().st_size for p in work.rglob('*') if p.is_file())
        run_directory=ROOT
        record['temporary_directory_removed']=not work.exists()
        run(['python3','scripts/verify_upstream.py'])
        record['complete']=True;save()
        print(f'Original SQLite {source_name}: {expected_count}/{expected_count} tests; owned build removed')
    except BaseException as error:
        record['error']=repr(error);save();raise


if __name__=='__main__':
    main()
