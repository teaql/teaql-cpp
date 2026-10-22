#!/usr/bin/env python3
"""Run all original SQLite facet tests with exact reference registry dependencies."""
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
    args=parser.parse_args()
    if args.record.exists():
        parser.error('Preserve historical evidence')
    reference=ROOT/'.reference/teaql-rs'
    source=reference/'teaql-provider-sqlite/tests/facet_trace.rs'
    expected=re.findall(r'#\[test\]\s*fn (\w+)\(',source.read_text())
    assert len(expected)==10
    record=dict(complete=False,source=str(source.relative_to(ROOT)),source_sha256=digest(source),
                reference_lock_sha256=digest(reference/'Cargo.lock'),expected_tests=expected,
                controller=Path(__file__).read_text(),steps=[],source_promoted=False)
    environment=dict(os.environ,CARGO_INCREMENTAL='0',CARGO_PROFILE_DEV_DEBUG='0',
                     CARGO_PROFILE_TEST_DEBUG='0',TEAQL_LOG_ENDPOINT='off',
                     CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')

    def save():
        args.record.write_bytes(gzip.compress((json.dumps(record,indent=2)+'\n').encode(),mtime=0))

    def run(command):
        result=subprocess.run(list(map(str,command)),cwd=ROOT,env=environment,text=True,capture_output=True)
        record['steps'].append(dict(command=list(map(str,command)),exit_code=result.returncode,stdout=result.stdout,stderr=result.stderr))
        save();result.check_returncode();return result.stdout

    try:
        run(['python3','scripts/verify_upstream.py'])
        with tempfile.TemporaryDirectory(prefix='teaql_sqlite_facet_source_') as temporary:
            work=Path(temporary);environment['CARGO_TARGET_DIR']=str(work/'target')
            shutil.copyfile(source,work/'facet_trace.rs')
            manifest=['[package]','name="teaql_facet_source_506"','version="0.0.0"','edition="2024"',
                      '[workspace]','[[test]]','name="facet_trace"','path="facet_trace.rs"','[dependencies]',
                      'futures-executor="=0.3.32"','futures-util="=0.3.32"',
                      'rusqlite={version="=0.40.1",features=["bundled","column_decltype","functions"]}']
            packages=['teaql-core','teaql-runtime','teaql-data-service','teaql-sql','teaql-provider-sqlite']
            for package in packages:
                manifest.append(package+'={path='+json.dumps(str(reference/package))+'}')
            manifest.append('[patch.crates-io]')
            for package in packages+['teaql-macros']:
                manifest.append(package+'={path='+json.dumps(str(reference/package))+'}')
            (work/'Cargo.toml').write_text('\n'.join(manifest)+'\n')
            shutil.copyfile(reference/'Cargo.lock',work/'Cargo.lock')
            run(['cargo','metadata','--offline','--format-version','1','--manifest-path',work/'Cargo.toml'])
            assert registry_identities(work/'Cargo.lock') <= registry_identities(reference/'Cargo.lock')
            record['resolved_lock']=(work/'Cargo.lock').read_text()
            command=['cargo','test','--offline','--locked','--manifest-path',work/'Cargo.toml','-j2','--test','facet_trace']
            listed=run([*command,'--','--list'])
            names=re.findall(r'^(\w+): test$',listed,re.M)
            assert sorted(names)==sorted(expected)
            result=run([*command,'--','--test-threads=1'])
            passed=re.findall(r'^test (\w+) \.\.\. ok$',result,re.M)
            assert sorted(passed)==sorted(expected) and '10 passed; 0 failed; 0 ignored' in result
            assert digest(work/'facet_trace.rs')==record['source_sha256']==digest(source)
            record['passed_tests']=passed
            record['owned_bytes_before_cleanup']=sum(p.stat().st_size for p in work.rglob('*') if p.is_file())
        record['temporary_directory_removed']=not work.exists()
        run(['python3','scripts/verify_upstream.py'])
        record['complete']=True;save()
        print('Original SQLite facet source: 10/10 tests; owned build removed')
    except BaseException as error:
        record['error']=repr(error);save();raise


if __name__=='__main__':
    main()
