#!/usr/bin/env python3
"""Build and run the native relation review union without promoting source acceptance."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
TARGETS = '''relation_model_tests relation_model_probe original_cache_relation_tests
relation_execution_probe relation_execution_tests relation_hydration_tests
relation_enhancement_tests relation_hydration_replay_probe relation_planning_extensions_probe
hydration_flat_facets_probe hydration_owner_facets_probe hydration_owner_facets_async_probe
graph_relation_facets_probe hydration_sibling_keys_probe loaded_row_graph_probe
relation_enhancement_complete_probe dynamic_relation_tests provider_relation_membership_tests'''.split()
TESTS = '''relation_model_contracts relation_model_rust_parity original_runtime_cache_relation_contracts
relation_execution_rust_parity relation_probe_source_boundaries relation_execution_contracts
relation_hydration_contracts relation_enhancement_contracts relation_hydration_rust_parity
relation_hydration_intent_rust_parity relation_planning_extensions_contracts
relation_planning_extensions_rust_parity hydration_flat_facets_rust_parity
hydration_flat_facets_context_close hydration_owner_facets_rust_parity
hydration_owner_facets_async_rust_parity hydration_owner_facets_async_context_close
graph_relation_facets_rust_parity graph_relation_facets_context_contracts
hydration_sibling_keys_rust_parity loaded_row_graph_rust_parity loaded_row_graph_context_contracts
relation_enhancement_complete_rust_parity dynamic_relation_contracts
relation_loaded_state_rust_parity provider_relation_membership_contracts'''.split()


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inputs():
    paths = set()
    for base in ['teaql_runtime/include/teaql/runtime/detail/data_service', 'teaql_runtime/src/data_service']:
        paths.update(p for p in (ROOT/base).rglob('*') if p.is_file())
    paths.add(ROOT/'tests/CMakeLists.txt')
    paths.add(ROOT/'.reference/teaql-rs/teaql-runtime/src/data_service/relation.rs')
    return {str(p.relative_to(ROOT)): digest(p) for p in sorted(paths)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        raise RuntimeError('Preserve prior gate receipts')
    record = dict(complete=False, source_promoted=False, steps=[], source_sha256=inputs(),
                  targets=TARGETS, tests=TESTS, controller=Path(__file__).read_text(),
                  free_bytes=os.statvfs(ROOT).f_bavail*os.statvfs(ROOT).f_frsize)

    def save():
        args.output.write_bytes(gzip.compress((json.dumps(record,indent=2)+'\n').encode(),mtime=0))

    def run(command):
        print('Running:', ' '.join(map(str,command)), flush=True)
        result = subprocess.run(command,cwd=ROOT,env=dict(os.environ,TEAQL_LOG_ENDPOINT='off'),
                                text=True,capture_output=True)
        record['steps'].append(dict(command=list(map(str,command)),exit_code=result.returncode,
                                    stdout=result.stdout,stderr=result.stderr))
        save()
        print('Exit:',result.returncode,flush=True)
        result.check_returncode()
        return result.stdout

    try:
        run(['python3','scripts/verify_upstream.py'])
        run(['cmake','--build','--preset','native','--target',*['teaql_'+s for s in TARGETS],'-j','2'])
        pattern='^('+'|'.join(map(re.escape,TESTS))+')$'
        selection=json.loads(run(['ctest','--test-dir','build_native_current','--show-only=json-v1','-R',pattern]))
        assert sorted(t['name'] for t in selection['tests'])==sorted(TESTS)
        xml=ROOT/'build_native_current/relation_native_closure.junit.xml'
        run(['ctest','--test-dir','build_native_current','-R',pattern,'--output-on-failure','--output-junit',str(xml)])
        suite=ET.fromstring(xml.read_text())
        cases=list(suite.iter('testcase'))
        assert len(cases)==len(TESTS) and all(c.find('failure') is None and c.find('skipped') is None for c in cases)
        record['junit']=xml.read_text()
        record['junit_path']=str(xml)
        assert inputs()==record['source_sha256'],'Review inputs changed during the gate'
        record['complete']=True
        save()
        print(f'Native relation closure: {len(TESTS)}/{len(TESTS)} passed; no source promotion',flush=True)
    except BaseException as error:
        record['error']=repr(error)
        save()
        raise


if __name__ == '__main__':
    main()
