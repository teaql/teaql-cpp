#!/usr/bin/env python3
"""Compare original release overflow policy with isolated C++ NDEBUG mutation templates."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys
import tempfile
from reference_runtime_view_5_0_6 import ReferenceRuntimeView

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tests/parity'))
from compare_values import observe, same


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    if args.record.exists():
        parser.error('Preserve historical receipts')
    prior_path = ROOT/'docs/parity/resolved_mutation_privacy_source_5_0_6.json.gz'
    prior = json.loads(gzip.decompress(prior_path.read_bytes()))
    corpus = json.loads(gzip.decompress((ROOT/'tests/parity/resolved_mutation_privacy_5_0_6_cases.json.gz').read_bytes()))
    selected = [x for x in corpus['cases'][3518:] if x['expected']['outcome'].get('panic')]
    assert prior['complete'] and len(selected) == 60
    rows = [x['input'] for x in selected]
    for case in selected:
        row = json.loads(json.dumps(case['input']))
        if row.get('version') == 2**63-1:
            row['version'] -= 1
        row['versions'] = [v-1 if v == 2**63-1 else v for v in row.get('versions', [])]
        rows.append(row)
    record = dict(complete=False, steps=[], cases=rows, prior_sha256=digest(prior_path),
                  source_promoted=False, scope='Original Rust release policy versus C++ probe-only NDEBUG; not a full Release runtime build')
    environment = dict(os.environ, CARGO_INCREMENTAL='0', TEAQL_LOG_ENDPOINT='off',
                       CARGO_PROFILE_RELEASE_OPT_LEVEL='0', CARGO_PROFILE_RELEASE_DEBUG='0',
                       CARGO_PROFILE_RELEASE_DEBUG_ASSERTIONS='false', CARGO_PROFILE_RELEASE_OVERFLOW_CHECKS='false',
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')

    def save():
        args.record.write_bytes(gzip.compress((json.dumps(record,indent=2)+'\n').encode(),mtime=0))

    def run(command, cwd=ROOT):
        result = subprocess.run(list(map(str,command)),cwd=cwd,env=environment,text=True,capture_output=True)
        record['steps'].append(dict(command=list(map(str,command)),cwd=str(cwd),exit_code=result.returncode,stdout=result.stdout,stderr=result.stderr))
        save(); result.check_returncode()
        return result.stdout

    try:
        run(['python3','scripts/verify_upstream.py'])
        with tempfile.TemporaryDirectory(prefix='teaql_release_versions_') as temporary:
            work=Path(temporary); environment['CARGO_TARGET_DIR']=str(work/'target')
            inputs=work/'inputs.json';inputs.write_text(json.dumps(rows)+'\n')
            rendered,count=re.subn(r'include_str!\("[^"]*/inputs.json"\)',lambda _: 'include_str!('+json.dumps(str(inputs))+')',prior['observer_rendering']['rendered'])
            assert count==1
            observer=work/'observer.rs';observer.write_text(rendered)
            view=ReferenceRuntimeView(ROOT,work/'runtime',{'src/data_service/resolved.rs':observer},run)
            output=run(['cargo','test','--release','--offline','--locked','--manifest-path',view.directory/'Cargo.toml','-j2','--lib','data_service::resolved::cpp_resolved_mutation_execution_observer::','--','--nocapture','--test-threads=1'])
            assert '1 passed; 0 failed' in output
            expected=[json.loads(line.split('TEAQL_RESOLVED_EXECUTION_JSON ',1)[1]) for line in output.splitlines() if 'TEAQL_RESOLVED_EXECUTION_JSON ' in line]
            assert len(expected)==120
            record['original']=dict(observations=expected,provenance=view.provenance())
            command=next(x for x in json.loads((ROOT/'build_native_current/compile_commands.json').read_text()) if x['file'].endswith('/tests/parity/runtime_mutation_probe.cpp'))
            compile_args=shlex.split(command['command']); obj=work/'probe.o';compile_args[compile_args.index('-o')+1]=str(obj)
            compile_args += ['-DNDEBUG','-O0','-g0']
            run(compile_args,cwd=Path(command['directory']))
            link=run(['ninja','-C','build_native_current','-t','commands','teaql_runtime_mutation_probe']).splitlines()[-1]
            tokens=shlex.split(link);assert tokens[:2]==[':', '&&'] and tokens[-2:]==['&&', ':']
            tokens=tokens[2:-2];archive_paths=[ROOT/'build_native_current'/x for x in tokens if x.endswith('.a')]
            symbols=run(['nm','-C',*archive_paths])
            assert 'mutation_next_version' not in symbols and 'EntityMutationOperation<' not in symbols
            tokens=[str(obj) if x.endswith('/runtime_mutation_probe.cpp.o') else x for x in tokens]
            binary=work/'probe';tokens[tokens.index('-o')+1]=str(binary)
            run(tokens,cwd=ROOT/'build_native_current')
            actual=observe(str(binary),rows)
            record['candidate']=dict(observations=actual,binary_sha256=digest(binary),archive_sha256={str(p.relative_to(ROOT)):digest(p) for p in archive_paths},probe_sha256=digest(ROOT/'tests/parity/runtime_mutation_probe.cpp'))
            record['differences']=[i for i,(a,b) in enumerate(zip(expected,actual)) if not same(a,b)]
            assert len(actual)==120 and not record['differences'],record['differences']
            assert all(not x['outcome'].get('panic') for x in expected)
            view.verify()
            record['owned_bytes_before_cleanup']=sum(p.stat().st_size for p in work.rglob('*') if p.is_file())
        record['temporary_directory_removed']=not work.exists()
        record['complete']=True;save()
        print('Release version policy: 120/120 source comparisons; owned build removed')
    except BaseException as error:
        record['error']=repr(error);save();raise


if __name__=='__main__':
    main()
