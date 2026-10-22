#!/usr/bin/env python3
"""Build and run the native resolved-service review union without promoting source acceptance."""
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
TARGETS = ['enhanced_aggregate_probe', 'entity_query_probe', 'entity_stream_probe', 'loaded_row_graph_probe', 'original_cache_relation_tests', 'relation_enhancement_complete_probe', 'request_trace_scope_tests', 'resolved_cache_intent_probe', 'resolved_dynamic_rows_probe', 'resolved_mutation_preparation_probe', 'resolved_public_aggregate_scope_probe', 'resolved_scope_privacy_probe', 'resolved_scope_tree_probe', 'resolved_select_preparation_probe', 'runtime_mutation_probe', 'runtime_service_types_probe']
TESTS = ['original_runtime_cache_relation_contracts', 'resolved_public_aggregate_scope_rust_parity', 'resolved_scope_tree_rust_parity', 'resolved_scope_privacy_rust_parity', 'resolved_cache_intent_rust_parity', 'relation_enhancement_complete_rust_parity', 'loaded_row_graph_rust_parity', 'loaded_row_graph_context_contracts', 'request_trace_scope_contracts', 'entity_query_capture_rust_parity', 'runtime_service_types_rust_parity', 'enhanced_aggregate_rust_parity', 'query_entry_cursor_rust_parity', 'query_entry_cursor_context_contracts', 'aggregate_membership_rust_parity', 'aggregate_membership_context_contracts', 'resolved_select_preparation_rust_parity', 'resolved_mutation_preparation_rust_parity', 'resolved_mutation_execution_rust_parity', 'resolved_mutation_privacy_rust_parity', 'resolved_dynamic_rows_rust_parity', 'resolved_query_schema_rust_parity', 'resolved_query_typed_schema_rust_parity', 'resolved_query_schema_context_contracts', 'resolved_stream_schema_rust_parity', 'resolved_stream_schema_context_contracts', 'resolved_enhanced_typed_rust_parity', 'resolved_dynamic_loaded_rust_parity', 'resolved_dynamic_guard_rust_parity', 'resolved_dynamic_typed_rust_parity', 'resolved_dynamic_typed_context_contracts']


# Keep later private-entry and representation repairs in the joined gate.
TARGETS += ['resolved_private_aggregate_scope_probe', 'id_set_page_probe',
            'resolved_cache_execution_probe', 'resolved_flat_predecode_probe',
            'generated_relation_analysis_inputs_probe',
            'generated_matching_statistics_inputs_probe']
TESTS += ['resolved_private_aggregate_scope_rust_parity',
          'resolved_id_set_intent_rust_parity', 'resolved_cache_execution_rust_parity',
          'resolved_flat_predecode_rust_parity', 'relation_analysis_inputs_rust_parity',
          'matching_statistics_inputs_rust_parity']


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inputs():
    paths = set()
    for base in ['teaql_runtime/include/teaql/runtime/detail/data_service', 'teaql_runtime/src/data_service']:
        paths.update(p for p in (ROOT/base).rglob('*') if p.is_file())
    for pattern in ('*.cpp', '*.py', '*_source_5_0_6.rs'):
        paths.update((ROOT/'tests/parity').glob(pattern))
    paths.add(ROOT/'teaql_runtime/tests/request_trace_scope_tests.cpp')
    paths.add(ROOT/'tests/CMakeLists.txt')
    paths.add(ROOT/'teaql_core/include/teaql/core/entity.hpp')
    paths.add(Path(__file__).resolve())
    for base in ('examples/generated', 'tests/generated_relations'):
        paths.update(p for p in (ROOT/base).rglob('*') if p.is_file())
    paths.add(ROOT/'.reference/teaql-rs/teaql-runtime/src/data_service/resolved.rs')
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
        xml=ROOT/'build_native_current/resolved_native_closure.junit.xml'
        run(['ctest','--test-dir','build_native_current','-R',pattern,'--output-on-failure','--output-junit',str(xml)])
        suite=ET.fromstring(xml.read_text())
        cases=list(suite.iter('testcase'))
        assert len(cases)==len(TESTS) and all(c.find('failure') is None and c.find('skipped') is None for c in cases)
        record['junit']=xml.read_text()
        record['junit_path']=str(xml)
        assert inputs()==record['source_sha256'],'Review inputs changed during the gate'
        record['complete']=True
        save()
        print(f'Native resolved-service closure: {len(TESTS)}/{len(TESTS)} passed; no source promotion',flush=True)
    except BaseException as error:
        record['error']=repr(error)
        save()
        raise


if __name__ == '__main__':
    main()
