#!/usr/bin/env python3
"""Replay distinct query entry, aggregate, pagination and dynamic-selection protocols."""
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


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record',type=Path,required=True)
    parser.add_argument('--cpp',type=Path)
    parser.add_argument('--module',type=Path)
    parser.add_argument('--write-corpus',action='store_true')
    parser.add_argument('--expect-differences',action='store_true')
    parser.add_argument('--check-gates',action='store_true')
    args=parser.parse_args()
    if args.record.exists() or args.record.suffix!='.gz':parser.error('Use a fresh .gz receipt')
    if args.check_gates and (args.cpp is None or args.module is None or args.expect_differences):
        parser.error('Installed gates require both matching frontends')
    root=Path(__file__).resolve().parents[1]
    sys.path[:0]=[str(root/'scripts'),str(root/'tests/parity')]
    from compare_values import observe,same
    from query_entry_protocol_inputs_5_0_6 import inputs
    from reference_runtime_view_5_0_6 import ReferenceRuntimeView
    from run_native_integration_epoch_5_0_6 import capture_inputs,references
    digest=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
    assets=['tests/parity/enhanced_aggregate_probe.cpp','tests/parity/enhanced_aggregate_source_5_0_6.rs',
            'teaql_runtime/include/teaql/runtime/detail/data_service/declarations.hpp',
            'teaql_runtime/include/teaql/runtime/detail/data_service/enhanced_entities.hpp',
            'teaql_runtime/include/teaql/runtime/detail/data_service/entity_query.hpp',
            'teaql_runtime/include/teaql/runtime/detail/data_service/graph_saver.hpp',
            'scripts/verify_query_entry_protocol_5_0_6.py','tests/parity/query_entry_protocol_inputs_5_0_6.py']
    record=dict(complete=False,source_promoted=False,steps=[],profiles={},inputs=capture_inputs(),references=references(),
                reproduction_sources={p:(root/p).read_text() for p in assets})
    environment=dict(os.environ,TEAQL_LOG_ENDPOINT='off',CARGO_TARGET_DIR='/tmp/teaql_cpp_sql_compiler_source_506',
                     CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')
    def save():args.record.parent.mkdir(parents=True,exist_ok=True);args.record.write_bytes(gzip.compress((json.dumps(record,indent=2)+'\n').encode(),mtime=0))
    def run(command):
        result=subprocess.run([str(x) for x in command],cwd=root,env=environment,text=True,capture_output=True)
        record['steps'].append(dict(command=[str(x) for x in command],exit_code=result.returncode,stdout=result.stdout,stderr=result.stderr));save()
        result.check_returncode();return result.stdout
    try:
        run([sys.executable,root/'scripts/verify_upstream.py'])
        rows=inputs();assert len(rows)==6080
        with tempfile.TemporaryDirectory(prefix='teaql_query_entry_protocol_506_') as temporary:
            work=Path(temporary);input_path=work/'inputs.json';input_path.write_text(json.dumps(rows)+'\n')
            fixture=root/'tests/parity/enhanced_aggregate_source_5_0_6.rs';marker='__TEAQL_ENHANCED_AGGREGATE_INPUTS__';assert fixture.read_text().count(marker)==1
            observer=work/'observer.rs';observer.write_text(fixture.read_text().replace(marker,str(input_path)))
            view=ReferenceRuntimeView(root,work/'runtime',{'src/data_service/resolved.rs':observer},run)
            output=run(['cargo','test','--offline','--locked','--manifest-path',view.directory/'Cargo.toml','-j2',
                        '--lib','data_service::resolved::cpp_enhanced_aggregate_observer::','--','--nocapture','--test-threads=1'])
            names=re.findall(r'^test (\S+) \.\.\.',output,flags=re.M)
            assert names==['data_service::resolved::cpp_enhanced_aggregate_observer::replay_query_owned_and_separate_aggregates'] and '1 passed; 0 failed' in output
            expected=[json.loads(line.split('TEAQL_ENHANCED_AGGREGATE_JSON ',1)[1]) for line in output.split('\n') if 'TEAQL_ENHANCED_AGGREGATE_JSON ' in line]
            assert len(expected)==len(rows)==6080
            record['original']=dict(tests=names,observations=expected,runtime_view=view.provenance())
            legacy=json.loads(gzip.decompress((root/'tests/parity/enhanced_aggregate_5_0_6_cases.json.gz').read_bytes()))
            assert legacy['baseline']==record['references']['teaql-rs']
            assert same(rows[:320],[case['input'] for case in legacy['cases']])
            assert same(expected[:320],[case['expected'] for case in legacy['cases']])
            path=root/'tests/parity/query_entry_protocol_5_0_6_cases.json.gz'
            corpus=dict(baseline=record['references']['teaql-rs'],cases=[dict(input=i,expected=e) for i,e in zip(rows,expected)])
            if args.write_corpus:
                assert not path.exists();path.write_bytes(gzip.compress((json.dumps(corpus,indent=2)+'\n').encode(),mtime=0))
                record['generated_inputs']={'runtime:'+str(path.relative_to(root)):digest(path)}
            else:assert same(json.loads(gzip.decompress(path.read_bytes())),corpus)
            for profile,binary in [('native',args.cpp),('modules_native',args.module)]:
                if binary is None:continue
                gate={}
                if args.check_gates:
                    imported=profile=='modules_native';prefix='teaql_module_' if imported else 'teaql_';build=root/('build_modules_native' if imported else 'build_native_current')
                    assert binary.resolve()==build/('modules' if imported else '')/(prefix+'enhanced_aggregate_probe')
                    targets=['enhanced_aggregate_probe','loaded_row_graph_probe','context_query_tests']
                    tests=['query_entry_protocol_rust_parity','enhanced_aggregate_rust_parity','loaded_graph_protocol_rust_parity','loaded_row_graph_rust_parity','loaded_row_graph_context_contracts','context_query_contracts']
                    if imported:
                        tests=['module_'+x for x in tests]
                    else:
                        targets += ['original_runtime_aggregate_tests','enhanced_entities_tests','typed_aggregate_tests','typed_aggregate_scope_probe','enhanced_entities_probe','generated_query_expression']
                        tests += ['original_runtime_aggregate_cache_contracts','enhanced_entities_contracts','typed_aggregate_context_contracts','typed_aggregate_scope_rust_parity','enhanced_entities_rust_parity','example_generated_query_expression']
                    run(['cmake','--preset',profile]);command=['cmake','--build','--preset',profile,'--parallel','2','--target',*[prefix+x for x in targets]];run(command)
                    if imported:run([sys.executable,root/'scripts/verify_module_consumers.py','--build-dir',build])
                    junit=work/(profile+'.xml');run(['ctest','--preset',profile,'--output-on-failure','--no-tests=error','--output-junit',junit,'-R','^('+'|'.join(tests)+')$'])
                    tree=ET.parse(junit).getroot();assert sorted(x.attrib['name'] for x in tree.findall('.//testcase'))==sorted(tests)
                    assert not any(tree.findall('.//'+tag) for tag in ['failure','error','skipped'])
                    assert 'ninja: no work to do.' in run(command)
                    gate=dict(tests=tests,junit=junit.read_text(),post_build_no_work=True)
                actual=observe(str(binary.resolve()),rows);differences=[i for i,(a,b) in enumerate(zip(expected,actual)) if not same(a,b)]
                record['profiles'][profile]=dict(observations=actual,differing_indices=differences,binary_sha256=digest(binary),gate=gate);save()
                assert bool(differences)==args.expect_differences,(profile,len(differences))
            view.verify()
        assert capture_inputs()==record['inputs']|record.get('generated_inputs',{}) and references()==record['references']
        record.update(complete=True,unchanged_inputs=True,original_tests=0,added_caller_tests=1,
            all_installed_frontends=not args.expect_differences and set(record['profiles'])=={'native','modules_native'});save()
        print('Actual query entry protocols:6080; '+', '.join(f'{p}:{len(v["differing_indices"])} differences' for p,v in record['profiles'].items()),flush=True)
    except BaseException as error:
        record['controller_error']=dict(type=type(error).__name__,message=str(error));save();raise


if __name__=='__main__':main()
