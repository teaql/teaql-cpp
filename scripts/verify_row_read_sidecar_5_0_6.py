#!/usr/bin/env python3
"""Compare complete read-result sidecar transitions with locked original methods."""
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
ROOT = Path(__file__).resolve().parents[1]
PARITY = ROOT / 'tests/parity'
PROJECT = PARITY / 'rust_query_full_state_5_0_6'
sys.path.insert(0, str(PARITY))
from compare_values import same


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, default=ROOT / 'build_native_current')
    parser.add_argument('--module-build-dir', type=Path)
    parser.add_argument('--record-corpus', action='store_true')
    parser.add_argument('--expect-gap', action='store_true')
    parser.add_argument('--record', type=Path, required=True)
    args = parser.parse_args()
    corpus_path = PARITY / 'row_read_sidecar_5_0_6_cases.json.gz'
    if args.record.exists() or (args.record_corpus and corpus_path.exists()):
        parser.error('Use fresh evidence paths; never overwrite source observations')
    record = dict(complete=False, full_goal_accepted=False, whole_source_accepted=False,
                  steps=[], scope='Nine native/relation/dynamic sidecar states, checked and '
                  'unchecked merge, loaded/unloaded lists, take/clear, Clone/equality and '
                  'complete Debug; C++ additionally retains actual merged rows across '
                  'source UserContext deletion using explicit Global copies.')
    def save():
        args.record.write_bytes(gzip.compress((json.dumps(record, indent=2)+'\n').encode(),
                                             compresslevel=1, mtime=0))
    environment = dict(os.environ, CARGO_TARGET_DIR='/tmp/teaql_cpp_core_baselines_506',
                       CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc',
                       TEAQL_LOG_ENDPOINT='off')
    def run(command, payload=None):
        q = subprocess.run([str(p) for p in command], cwd=ROOT, env=environment,
                           input=payload, text=True, capture_output=True)
        record['steps'].append(dict(command=[str(p) for p in command], exit_code=q.returncode,
                                    stdout=q.stdout, stderr=q.stderr))
        if payload is None or q.returncode:
            save()
        q.check_returncode()
        return q.stdout
    run(['python3', ROOT / 'scripts/verify_upstream.py'])
    baseline = json.loads((ROOT / 'upstream.lock.json').read_text())['commit']
    assert registry_identities(PROJECT/'Cargo.lock') <= registry_identities(ROOT/'.reference/teaql-rs/Cargo.lock')
    run(['cargo', 'build', '--offline', '--locked', '--manifest-path', PROJECT/'Cargo.toml',
         '--example', 'row_read_sidecar'])
    inputs = [dict(left=l, right=r, checked=c, loaded=b, name=n)
              for l,r,c,b,n in itertools.product(range(9),range(9),(False,True),(False,True),
                                                ('lines','雪\0lines','l'*256))]
    record['inputs'] = inputs
    payload = ''.join(json.dumps(v)+'\n' for v in inputs)
    expected = None
    if not args.record_corpus:
        corpus = json.loads(gzip.decompress(corpus_path.read_bytes()))
        assert corpus['baseline'] == baseline and [c['input'] for c in corpus['cases']] == inputs
        expected = [c['expected'] for c in corpus['cases']]
    executables = [('original', Path(environment['CARGO_TARGET_DIR'])/'debug/examples/row_read_sidecar'),
                   ('native', args.build_dir.resolve()/'teaql_row_read_sidecar_probe')]
    if args.module_build_dir:
        executables.append(('modules', args.module_build_dir.resolve()/'modules/teaql_module_row_read_sidecar_probe'))
    gaps=[]
    for label, executable in executables:
        output=run([executable],payload)
        assert output.endswith('\n')
        actual=[json.loads(line) for line in output.split('\n')[:-1]]
        assert len(actual)==len(inputs)
        if expected is None:
            expected=actual
        for i,(a,b) in enumerate(zip(expected,actual)):
            assert b['equal_clone'] and b['cleared_equals_pristine'] and b['initial_unchanged'] and b['right_unchanged']
            if not same(a,b):
                gaps.append(dict(frontend=label,index=i,input=inputs[i],expected=a,actual=b))
        print(label,len(actual),'observed',flush=True)
    corpus=dict(baseline=baseline,cases=[dict(input=i,expected=o) for i,o in zip(inputs,expected)])
    compressed=gzip.compress((json.dumps(corpus,separators=(',',':'))+'\n').encode(),mtime=0)
    if args.record_corpus:corpus_path.write_bytes(compressed)
    else:assert corpus_path.read_bytes()==compressed,'Original source corpus bytes changed'
    paths=['teaql_core/src/row.cpp','teaql_core/include/teaql/core/row.hpp',
           'teaql_core/include/teaql/core/dynamic_fields_debug.hpp',
           'tests/parity/row_read_sidecar_probe.cpp',
           'tests/parity/rust_query_full_state_5_0_6/examples/row_read_sidecar.rs',
           'tests/parity/rust_query_full_state_5_0_6/Cargo.toml',
           'tests/parity/rust_query_full_state_5_0_6/Cargo.lock',
           '.reference/teaql-rs/teaql-core/src/query.rs',
           '.reference/teaql-rs/teaql-core/src/dynamic_fields.rs',
           '.reference/teaql-rs/Cargo.lock','scripts/verify_row_read_sidecar_5_0_6.py']
    record.update(complete=not gaps,differences=gaps,observations=len(inputs),
                  corpus_sha256=hashlib.sha256(corpus_path.read_bytes()).hexdigest(),
                  sources={p:dict(sha256=hashlib.sha256((ROOT/p).read_bytes()).hexdigest(),
                                  text=(ROOT/p).read_text()) for p in paths})
    save()
    if args.expect_gap:
        assert gaps and all(g['frontend']=='native' for g in gaps)
        print('Preserved',len(gaps),'original/native mismatches',flush=True)
    else:
        assert not gaps,f'{len(gaps)} row sidecar mismatches'
        print(len(inputs),'exact source outcomes per frontend',flush=True)


if __name__=='__main__':main()
