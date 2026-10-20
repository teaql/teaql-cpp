#!/usr/bin/env python3
"""Replay original resolved streams with readonly schemas and cursor close boundaries."""
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


def render(root, input_path):
    path=root/'tests/parity/rust_private_save_5_0_6/src/bin/entity_stream_5_0_6.rs'
    driver=path.read_text();driver=driver[driver.index('use futures_core'):driver.index('fn main()')]
    driver=driver.replace('use teaql_runtime::{','use crate::{')
    def adapt(before,after,count=1):
        nonlocal driver
        assert driver.count(before)==count,(before,driver.count(before))
        driver=driver.replace(before,after)
    adapt('    last_first: bool,','    last_first: bool,\n    schema_probe:bool, empty:bool, result_kind:String,',2)
    adapt('''            rows: vec![CompactRow::from_map(
                [("id".into(), Value::U64(index as u64 + 1))].into(),
            )],''','''            rows: provider_rows(index,self.schema_probe,self.empty,&self.result_kind),''')
    adapt('            last_first: self.last_first,','            last_first: self.last_first, schema_probe:self.schema_probe,empty:self.empty,result_kind:self.result_kind.clone(),')
    adapt('        last_first: input["last_first"].as_bool().unwrap(),','        last_first: input["last_first"].as_bool().unwrap(), schema_probe:input["schema_probe"]==true,empty:input["empty"]==true,result_kind:input["result_kind"].as_str().unwrap_or("u64").into(),')
    adapt('    let wrapped = std::panic::catch_unwind', '''    if input["schema_probe"]==true && input["schema_kind"]!="none" {
        query=query.with_dynamic_property_definitions(teaql_core::dynamic_properties::DynamicPropertyDefinitions::new(
            [("_result".into(),if input["schema_kind"]=="text" {DataType::Text}else{DataType::U64})]).unwrap());
    }
    let wrapped = std::panic::catch_unwind''')
    adapt('''                loop {
                    match stream.as_mut().poll_next(&mut cx) {''','''                let consumption=input["consume"].as_str().unwrap_or("all");
                if consumption=="pending" {
                    assert!(matches!(stream.as_mut().poll_next(&mut cx),Poll::Pending));
                } else if consumption!="none" {
                loop {
                    match stream.as_mut().poll_next(&mut cx) {''')
    adapt('''                            output["items"].as_array_mut().unwrap().push(json!({
                            "index": c.chunk_index, "last": c.is_last,
                            "id": c.rows[0].get("id").unwrap().try_u64().unwrap()}))''','''                            let mut item=json!({"index":c.chunk_index,"last":c.is_last,
                                "id":c.rows.first().and_then(|r|r.get("id")).and_then(Value::try_u64)});
                            if input["schema_probe"]==true {item["schema"]=schema_rows(&c.rows);}
                            output["items"].as_array_mut().unwrap().push(item)''')
    adapt('''                    }
                }
            }
        },''','''                    }
                    if consumption=="one" && !output["items"].as_array().unwrap().is_empty() {break;}
                }
                }
            }
        },''')
    template=(root/'tests/parity/resolved_stream_schema_source_5_0_6.rs').read_text()
    assert template.count('__TEAQL_STREAM_DRIVER__')==template.count('__TEAQL_STREAM_INPUTS__')==1
    return template.replace('__TEAQL_STREAM_DRIVER__',driver).replace('__TEAQL_STREAM_INPUTS__',str(input_path))


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record',type=Path,required=True)
    parser.add_argument('--cpp',type=Path)
    parser.add_argument('--module',type=Path)
    parser.add_argument('--write-corpus',action='store_true')
    parser.add_argument('--expect-differences',action='store_true')
    parser.add_argument('--check-gates',action='store_true')
    parser.add_argument('--jobs',type=int,default=4)
    args=parser.parse_args()
    if args.record.exists() or args.record.suffix!='.gz':parser.error('Use a fresh .gz receipt')
    if args.jobs<1:parser.error('jobs must be positive')
    if args.check_gates and (args.cpp is None or args.module is None or args.expect_differences):parser.error('Installed gates require both matching frontends')
    root=Path(__file__).resolve().parents[1];sys.path[:0]=[str(root/'scripts'),str(root/'tests/parity')]
    from compare_values import observe,same
    from resolved_stream_schema_inputs_5_0_6 import inputs
    from reference_runtime_view_5_0_6 import ReferenceRuntimeView
    from run_native_integration_epoch_5_0_6 import capture_inputs,references
    digest=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
    assets=['scripts/verify_resolved_stream_schema_5_0_6.py','tests/parity/entity_stream_probe.cpp',
            'tests/parity/resolved_stream_schema_source_5_0_6.rs','tests/parity/resolved_stream_schema_inputs_5_0_6.py',
            'tests/parity/rust_private_save_5_0_6/src/bin/entity_stream_5_0_6.rs',
            'teaql_runtime/include/teaql/runtime/detail/data_service/entity_stream.hpp']
    record=dict(complete=False,source_promoted=False,steps=[],profiles={},inputs=capture_inputs(),references=references(),reproduction_sources={p:(root/p).read_text() for p in assets})
    environment=dict(os.environ,CARGO_INCREMENTAL='0',TEAQL_LOG_ENDPOINT='off',CARGO_TARGET_DIR='/tmp/teaql_cpp_sql_compiler_source_506',CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')
    def save():args.record.parent.mkdir(parents=True,exist_ok=True);args.record.write_bytes(gzip.compress((json.dumps(record,indent=2)+'\n').encode(),mtime=0))
    def run(command):
        result=subprocess.run([str(x) for x in command],cwd=root,env=environment,text=True,capture_output=True)
        record['steps'].append(dict(command=[str(x) for x in command],exit_code=result.returncode,stdout=result.stdout,stderr=result.stderr));save();result.check_returncode();return result.stdout
    try:
        run([sys.executable,root/'scripts/verify_upstream.py']);rows=inputs();assert len(rows)==769
        with tempfile.TemporaryDirectory(prefix='teaql_stream_schema_506_') as temporary:
            work=Path(temporary);input_path=work/'inputs.json';input_path.write_text(json.dumps(rows)+'\n')
            rendered=render(root,input_path);observer=work/'observer.rs';observer.write_text(rendered);record['rendered_observer']=rendered
            view=ReferenceRuntimeView(root,work/'runtime',{'src/data_service/resolved.rs':observer},run)
            output=run(['cargo','test','--offline','--locked','--manifest-path',view.directory/'Cargo.toml','-j2','--lib','data_service::resolved::cpp_resolved_stream_schema_observer::','--','--nocapture','--test-threads=1'])
            names=re.findall(r'^test (\S+) \.\.\.',output,flags=re.M);assert names==['data_service::resolved::cpp_resolved_stream_schema_observer::replay_readonly_cursor_boundaries'] and '1 passed; 0 failed' in output
            expected=[json.loads(line.split('TEAQL_STREAM_SCHEMA_JSON ',1)[1]) for line in output.split('\n') if 'TEAQL_STREAM_SCHEMA_JSON ' in line];assert len(expected)==len(rows)
            record['original']=dict(tests=names,observations=expected,runtime_view=view.provenance())
            retained=json.loads(gzip.decompress((root/'tests/parity/entity_stream_5_0_6_cases.json.gz').read_bytes()));assert retained['baseline']==record['references']['teaql-rs']
            assert same(rows[:193],[case['input'] for case in retained['cases']]);assert same(expected[:193],[case['expected'] for case in retained['cases']])
            for entry,result in zip(rows[193:],expected[193:]):
                assert result['closes']==1 and result['pending']==(0 if entry['consume']=='none' else 1)
                successful=[item for item in result['items'] if 'schema' in item]
                if entry['consume'] in ['none','pending']:assert result['items']==[]
                if entry['empty']:assert all(item['schema']==[] and item['id'] is None for item in successful)
                elif entry['schema_kind']!='none':assert all(len(item['schema'])==2 and all(not row['equals_plain'] for row in item['schema']) for item in successful)
            corpus=dict(baseline=record['references']['teaql-rs'],cases=[dict(input=i,expected=e) for i,e in zip(rows,expected)])
            path=root/'tests/parity/resolved_stream_schema_5_0_6_cases.json.gz'
            if args.write_corpus:
                assert not path.exists();path.write_bytes(gzip.compress((json.dumps(corpus,indent=2)+'\n').encode(),mtime=0));record['generated_inputs']={'runtime:'+str(path.relative_to(root)):digest(path)}
            else:assert same(json.loads(gzip.decompress(path.read_bytes())),corpus)
            for profile,binary in [('native',args.cpp),('modules_native',args.module)]:
                if binary is None:continue
                gate={}
                if args.check_gates:
                    imported=profile=='modules_native';prefix='teaql_module_' if imported else 'teaql_';build=root/('build_modules_native' if imported else 'build_native_current')
                    assert binary.resolve()==build/('modules' if imported else '')/(prefix+'entity_stream_probe')
                    targets=['entity_stream_probe','stream_shapes_probe','sql_dynamic_stream_source_probe','entity_query_probe']
                    tests=['resolved_stream_schema_rust_parity','resolved_stream_schema_context_contracts','entity_stream_rust_parity','stream_shapes_rust_parity','sql_dynamic_stream_source_rust_parity',
                           'resolved_query_schema_rust_parity','resolved_query_typed_schema_rust_parity','resolved_query_schema_context_contracts']
                    if imported:tests=['module_'+x for x in tests]
                    else:
                        targets+=['generated_inherited_stream_probe','entity_stream_tests'];tests+=['generated_inherited_stream_rust_parity','entity_stream_contracts']
                    run(['cmake','--preset',profile]);command=['cmake','--build','--preset',profile,'--parallel',str(args.jobs),'--target',*[prefix+x for x in targets]];run(command)
                    if imported:run([sys.executable,root/'scripts/verify_module_consumers.py','--build-dir',build])
                    junit=work/(profile+'.xml');run(['ctest','--preset',profile,'--output-on-failure','--no-tests=error','--output-junit',junit,'-R','^('+'|'.join(tests)+')$'])
                    tree=ET.parse(junit).getroot();assert sorted(x.attrib['name'] for x in tree.findall('.//testcase'))==sorted(tests)
                    assert not any(tree.findall('.//'+tag) for tag in ['failure','error','skipped']);assert 'ninja: no work to do.' in run(command)
                    gate=dict(tests=tests,junit=junit.read_text(),post_build_no_work=True)
                actual=observe(str(binary.resolve()),rows);differences=[i for i,(a,b) in enumerate(zip(expected,actual)) if not same(a,b)]
                record['profiles'][profile]=dict(observations=actual,differing_indices=differences,binary_sha256=digest(binary),gate=gate);save();assert bool(differences)==args.expect_differences,(profile,len(differences))
            view.verify()
        assert capture_inputs()==record['inputs']|record.get('generated_inputs',{}) and references()==record['references']
        record.update(complete=True,unchanged_inputs=True,original_tests=0,added_caller_tests=1,all_installed_frontends=not args.expect_differences and set(record['profiles'])=={'native','modules_native'});save()
        print('Actual resolved streams:769; '+', '.join(f'{p}:{len(v["differing_indices"])} differences' for p,v in record['profiles'].items()),flush=True)
    except BaseException as error:
        record['controller_error']=dict(type=type(error).__name__,message=str(error));save();raise


if __name__=='__main__':main()
