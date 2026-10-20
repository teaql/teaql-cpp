#!/usr/bin/env python3
"""Replay actual public query paths with readonly result schemas and recording caches."""
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
    parser.add_argument('--suite',choices=['schema','typed','dynamic','dynamic_guard','dynamic_loaded','enhanced'],default='schema')
    parser.add_argument('--record',type=Path,required=True)
    parser.add_argument('--cpp',type=Path)
    parser.add_argument('--module',type=Path)
    parser.add_argument('--write-corpus',action='store_true')
    parser.add_argument('--expect-differences',action='store_true')
    parser.add_argument('--check-gates',action='store_true')
    parser.add_argument('--jobs',type=int,default=4)
    args=parser.parse_args()
    if args.record.exists() or args.record.suffix!='.gz':parser.error('Use a fresh .gz receipt')
    if args.check_gates and (args.cpp is None or args.module is None or args.expect_differences):
        parser.error('Installed gates require both matching frontends')
    root=Path(__file__).resolve().parents[1]
    sys.path[:0]=[str(root/'scripts'),str(root/'tests/parity')]
    from compare_values import observe,same
    from resolved_query_schema_inputs_5_0_6 import inputs,typed_inputs,dynamic_inputs,dynamic_guard_inputs,dynamic_loaded_inputs,enhanced_inputs,assert_dynamic_contract,assert_dynamic_loaded_contract,assert_enhanced_contract
    from reference_runtime_view_5_0_6 import ReferenceRuntimeView
    from run_native_integration_epoch_5_0_6 import capture_inputs,references
    digest=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
    assets=['tests/parity/entity_query_probe.cpp','tests/support/entity_decoder_fixture.hpp','tests/support/indexed_query_fixture.hpp','tests/parity/resolved_query_schema_source_5_0_6.rs',
            'tests/parity/resolved_query_schema_helpers_5_0_6.rs',
            'tests/parity/rust_private_save_5_0_6/src/bin/entity_query_capture_5_0_6.rs',
            'scripts/verify_resolved_query_schema_5_0_6.py','tests/parity/resolved_query_schema_inputs_5_0_6.py',
            'teaql_runtime/include/teaql/runtime/detail/data_service/entity_service.hpp',
            'teaql_runtime/include/teaql/runtime/detail/data_service/entity_query.hpp',
            'teaql_core/src/row.cpp',
            'teaql_runtime/include/teaql/runtime/detail/data_service/dynamic_entities.hpp',
            'teaql_runtime/include/teaql/runtime/detail/data_service/enhanced_entities.hpp']
    record=dict(complete=False,source_promoted=False,steps=[],profiles={},inputs=capture_inputs(),references=references(),
                reproduction_sources={p:(root/p).read_text() for p in assets})
    environment=dict(os.environ,CARGO_INCREMENTAL='0',TEAQL_LOG_ENDPOINT='off',CARGO_TARGET_DIR='/tmp/teaql_cpp_sql_compiler_source_506',
                     CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')
    def save():args.record.parent.mkdir(parents=True,exist_ok=True);args.record.write_bytes(gzip.compress((json.dumps(record,indent=2)+'\n').encode(),mtime=0))
    def run(command):
        result=subprocess.run([str(x) for x in command],cwd=root,env=environment,text=True,capture_output=True)
        record['steps'].append(dict(command=[str(x) for x in command],exit_code=result.returncode,stdout=result.stdout,stderr=result.stderr));save()
        result.check_returncode();return result.stdout
    try:
        run([sys.executable,root/'scripts/verify_upstream.py'])
        rows={'schema':inputs,'typed':typed_inputs,'dynamic':dynamic_inputs,'dynamic_guard':dynamic_guard_inputs,'dynamic_loaded':dynamic_loaded_inputs,'enhanced':enhanced_inputs}[args.suite]();count={'schema':2304,'typed':192,'dynamic':288,'dynamic_guard':72,'dynamic_loaded':288,'enhanced':3456}[args.suite];assert len(rows)==count
        with tempfile.TemporaryDirectory(prefix='teaql_resolved_query_schema_506_') as temporary:
            work=Path(temporary);input_path=work/'inputs.json';input_path.write_text(json.dumps(rows)+'\n')
            fixture=root/'tests/parity/resolved_query_schema_source_5_0_6.rs';marker='__TEAQL_QUERY_SCHEMA_INPUTS__';assert fixture.read_text().count(marker)==1
            driver_path=root/'tests/parity/rust_private_save_5_0_6/src/bin/entity_query_capture_5_0_6.rs'
            driver=driver_path.read_text();driver=driver[driver.index('use serde_json'):driver.index('fn main()')]
            driver=driver.replace('use teaql_runtime::{','use crate::{')
            def adapt(before,after,count=1):
                nonlocal driver
                assert driver.count(before)==count,(before,driver.count(before))
                driver=driver.replace(before,after)
            adapt('requests: Mutex<Vec<J>>,','requests: Mutex<Vec<J>>, classifications: Mutex<Vec<J>>, selection_calls: Mutex<Vec<J>>, dynamic_calls: Mutex<Vec<J>>,')
            adapt('''    let mut context = RuntimeModule::new()
        .descriptor(descriptor)
        .into_context()
        .with_user_identifier("fixture");''','''    let mut module=RuntimeModule::new().descriptor(descriptor);
    if input["dynamic_probe"]==true {module=module.behavior("Item",SelectionBehavior(state.clone(),input["selection_behavior"].as_str().unwrap().into()));}
    let mut context=module.into_context().with_user_identifier("fixture");
    if input["mode"]=="dynamic_loaded" || input["mode"].as_str().unwrap().starts_with("enhanced") {context.set_dynamic_fields_provider(Arc::new(IndexedProvider(state.clone(),input["payload"].as_str().unwrap().into())));}
    if input["dynamic_probe"]==true {context.set_request_policy(SelectionPolicy(state.clone(),input["selection_policy"].as_str().unwrap().into(),input["rewrite_intent"]==true));}''')
            adapt('    fail: bool,','    fail: bool, schema_probe: bool, empty: bool, result_kind: String, enhanced_probe: bool,')
            adapt('    async fn query(&self, q: QueryRequest)',                '    fn query_log_intent(&self,query:&SelectQuery)->SqlIntentRedactions {\n'
                '        if self.schema_probe {let mut event=json!({"entity":query.entity,"schema":query.dynamic_property_definitions.as_ref().and_then(|definitions|definitions.data_type("_result")).map(|kind|kind as u8)});if self.enhanced_probe {event["selected"]=json!(query.dynamic_field_selection.is_some());event["comment"]=json!(query.comment);}self.state.classifications.lock().unwrap().push(event);}\n'
                '        SqlIntentRedactions::from_unclassified_query(query)\n    }\n'
                '    async fn query(&self, q: QueryRequest)')
            adapt('rows: vec![CompactRow::from_map([("id".into(), Value::U64(1))].into())],','rows: provider_rows(self),')
            adapt('        fail: input["failure"].as_bool().unwrap(),',
                '        fail: input["failure"].as_bool().unwrap(), schema_probe:input["schema_probe"]==true,empty:input["empty"]==true,result_kind:input["result_kind"].as_str().unwrap_or("u64").into(),enhanced_probe:input["mode"].as_str().unwrap().starts_with("enhanced"),')
            adapt('    let mut outcomes = vec![];',
                '    let query=schema_query(query,input);\n'
                '    let recording=Arc::new(RecordingCache{inner:InMemoryAggregationCache::with_namespace("readonly"),events:Mutex::new(vec![])});\n'
                '    if input["record_cache"]==true {let backend:Arc<dyn crate::AggregationCacheBackend>=recording.clone();context.insert_resource(backend); }\n'
                '    let mut outcomes = vec![];')
            # Resource registration must occur before the repository borrows Context.
            setup='    let recording=Arc::new(RecordingCache{inner:InMemoryAggregationCache::with_namespace("readonly"),events:Mutex::new(vec![])});\n    if input["record_cache"]==true {let backend:Arc<dyn crate::AggregationCacheBackend>=recording.clone();context.insert_resource(backend); }\n'
            adapt(setup,'')
            adapt('    let repo = EntityDataService::for_executor(&context, "Item", &executor);',setup+'    let repo = EntityDataService::for_executor(&context, "Item", &executor);')
            adapt('.map(|v| v.data.len())','.map(|v| (v.data.len(),schema_rows(&v.data)))',2)
            adapt('.map(|v| v.len())','.map(|v| (v.len(),schema_rows(&v)))',2)
            adapt('            "list" => wait(repo.fetch_smart_list(&wrapped))',
                '            "typed" => wait(repo.fetch_entities::<SchemaItem>(&wrapped)).map(|v| (v.data.len(),schema_rows(&v.data.into_iter().map(|item|item.row).collect::<Vec<_>>()))),\n            "list" => wait(repo.fetch_smart_list(&wrapped))')
            adapt('            "typed" => wait(repo.fetch_entities::<SchemaItem>(&wrapped))',
                '            "dynamic_guard" => wait(repo.fetch_entities::<SchemaItem<true>>(&wrapped)).map(|v| (v.data.len(),schema_rows(&v.data.into_iter().map(|item|item.row).collect::<Vec<_>>()))),\n            "typed" => wait(repo.fetch_entities::<SchemaItem>(&wrapped))')
            adapt('            "typed" => wait(repo.fetch_entities::<SchemaItem>(&wrapped))',
                '            "dynamic_loaded" => wait(repo.fetch_entities::<IndexedSchemaItem>(&wrapped)).map(|v| (v.data.len(),indexed_schema(v.data))),\n            "typed" => wait(repo.fetch_entities::<SchemaItem>(&wrapped))')
            adapt('            "typed" => wait(repo.fetch_entities::<SchemaItem>(&wrapped))',
                '            "enhanced" => wait(repo.fetch_enhanced_entities::<IndexedSchemaItem>(&wrapped)).map(|v| (v.data.len(),indexed_schema(v.data))),\n'
                '            "enhanced_aggregates" => wait(repo.fetch_enhanced_entities_with_relation_aggregates::<IndexedSchemaItem>(&wrapped,&[])).map(|v| (v.data.len(),indexed_schema(v.data))),\n'
                '            "enhanced_owned" => wait(repo.fetch_enhanced_entities_with_relation_aggregates_owned::<IndexedSchemaItem>(wrapped,&[])).map(|v| (v.data.len(),indexed_schema(v.data))),\n'
                '            "typed" => wait(repo.fetch_entities::<SchemaItem>(&wrapped))')
            adapt('Ok(Ok(n)) => json!({"rows":n}),','Ok(Ok(n)) => {let mut outcome=json!({"rows":n.0});if input["schema_probe"]==true{outcome["schema"]=n.1;}outcome},')
            adapt('    json!({"outcomes":outcomes,','    let mut output=json!({"outcomes":outcomes,')
            needle='"continuous":context.continuous_page_plan()})'
            adapt(needle,needle+';\n    if input["schema_probe"]==true {output["classifications"]=json!(state.classifications.lock().unwrap().clone());output["cache_events"]=json!(recording.events.lock().unwrap().clone());} output')
            adapt('} output','} if input["dynamic_probe"]==true {output["selection_calls"]=json!(state.selection_calls.lock().unwrap().clone());} if input["mode"]=="dynamic_loaded" || input["mode"].as_str().unwrap().starts_with("enhanced") {output["dynamic_calls"]=json!(state.dynamic_calls.lock().unwrap().clone());} output')
            rendered=fixture.read_text().replace(marker,str(input_path)).replace('__TEAQL_QUERY_CAPTURE_DRIVER__',driver).replace('__TEAQL_QUERY_SCHEMA_HELPERS__',(root/'tests/parity/resolved_query_schema_helpers_5_0_6.rs').read_text())
            observer=work/'observer.rs';observer.write_text(rendered)
            record['rendered_observer']=rendered
            view=ReferenceRuntimeView(root,work/'runtime',{'src/data_service/resolved.rs':observer},run)
            output=run(['cargo','test','--offline','--locked','--manifest-path',view.directory/'Cargo.toml','-j2',
                        '--lib','data_service::resolved::cpp_resolved_query_schema_observer::','--','--nocapture','--test-threads=1'])
            names=re.findall(r'^test (\S+) \.\.\.',output,flags=re.M)
            assert names==['data_service::resolved::cpp_resolved_query_schema_observer::replay_result_schema_and_cache_paths'] and '1 passed; 0 failed' in output
            expected=[json.loads(line.split('TEAQL_QUERY_SCHEMA_JSON ',1)[1]) for line in output.split('\n') if 'TEAQL_QUERY_SCHEMA_JSON ' in line]
            assert len(expected)==len(rows)==count
            if args.suite=='dynamic_loaded':
                assert all('execution_panic' not in outcome for result in expected for outcome in result['outcomes']), 'Indexed fixture must reach actual decode outcomes'
            record['original']=dict(tests=names,observations=expected,runtime_view=view.provenance())
            retained=json.loads(gzip.decompress((root/'tests/parity/entity_query_capture_5_0_6_cases.json.gz').read_bytes()))
            assert retained['baseline']==record['references']['teaql-rs']
            if args.suite=='schema':
                assert same(rows[:768],[case['input'] for case in retained['cases']])
                assert same(expected[:768],[case['expected'] for case in retained['cases']])
            stem={'schema':'resolved_query_schema','typed':'resolved_query_typed_schema','dynamic':'resolved_dynamic_typed','dynamic_guard':'resolved_dynamic_guard','dynamic_loaded':'resolved_dynamic_loaded','enhanced':'resolved_enhanced_typed'}[args.suite]
            if args.suite=='enhanced':record['source_assertions']=assert_enhanced_contract([dict(input=i,expected=e) for i,e in zip(rows,expected)])
            if args.suite=='dynamic_loaded':record['source_assertions']=assert_dynamic_loaded_contract([dict(input=i,expected=e) for i,e in zip(rows,expected)])
            if args.suite=='dynamic':record['source_assertions']=assert_dynamic_contract([dict(input=i,expected=e) for i,e in zip(rows,expected)])
            path=root/f'tests/parity/{stem}_5_0_6_cases.json.gz'
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
                    assert binary.resolve()==build/('modules' if imported else '')/(prefix+'entity_query_probe')
                    targets=['entity_query_probe','context_query_tests','enhanced_aggregate_probe','resolved_dynamic_rows_probe']
                    tests=['resolved_query_schema_rust_parity','entity_query_capture_rust_parity',
                           'resolved_query_typed_schema_rust_parity','resolved_query_schema_context_contracts',
                           'context_query_contracts','query_entry_cursor_rust_parity',
                           'aggregate_membership_rust_parity','resolved_dynamic_rows_rust_parity']
                    contract_path=root/'tests/parity/query_entry_pagination_regression_5_0_6.json'
                    regression=json.loads(contract_path.read_text())
                    assert regression['baseline']==record['references']['teaql-rs']
                    assert digest(root/regression['parent_acceptance'])==regression['parent_acceptance_sha256']
                    if imported: tests=['module_'+x for x in tests]
                    retained=regression['profiles'][profile]
                    tests=list(dict.fromkeys(tests+retained['tests']))
                    build_targets=list(dict.fromkeys([prefix+x for x in targets]+retained['targets']))
                    if args.suite=='dynamic':
                        targets=['entity_query_probe','schema_dynamic_tests','generated_dynamic_field_tests','resolved_dynamic_rows_probe']
                        targets += ['original_runtime_typed_queries','original_runtime_aggregates_cache'] if imported else ['original_runtime_typed_query_tests','original_runtime_aggregate_tests']
                        build_targets=[prefix+x for x in targets]
                        tests=['resolved_dynamic_typed_rust_parity','resolved_dynamic_typed_context_contracts',
                               'resolved_query_schema_rust_parity','resolved_query_typed_schema_rust_parity',
                               'resolved_query_schema_context_contracts','original_runtime_typed_query_contracts',
                               'original_runtime_aggregate_cache_contracts','schema_dynamic_context_contracts',
                               'generated_dynamic_field_contracts','resolved_dynamic_rows_rust_parity']
                        if imported:
                            names={'original_runtime_typed_query_contracts':'original_runtime_typed_queries',
                                   'original_runtime_aggregate_cache_contracts':'original_runtime_aggregates_cache'}
                            tests=['module_'+names.get(x,x) for x in tests]

                    if args.suite in ('dynamic_guard','dynamic_loaded'):
                        build_targets=[prefix+'entity_query_probe']
                        tests=['resolved_dynamic_guard_rust_parity','resolved_dynamic_typed_rust_parity',
                               'resolved_query_typed_schema_rust_parity','resolved_dynamic_typed_context_contracts']
                        if args.suite=='dynamic_loaded':tests.append('resolved_dynamic_loaded_rust_parity')
                        if imported:tests=['module_'+x for x in tests]

                    if args.suite=='enhanced':
                        build_targets=[prefix+x for x in ['entity_query_probe','context_query_tests','enhanced_aggregate_probe']]
                        tests=['resolved_enhanced_typed_rust_parity','resolved_dynamic_loaded_rust_parity',
                               'resolved_dynamic_guard_rust_parity','resolved_dynamic_typed_rust_parity',
                               'resolved_query_typed_schema_rust_parity','resolved_dynamic_typed_context_contracts',
                               'context_query_contracts','enhanced_aggregate_rust_parity','query_entry_cursor_rust_parity',
                               'query_entry_cursor_context_contracts','aggregate_membership_rust_parity','aggregate_membership_context_contracts']
                        if imported:tests=['module_'+x for x in tests]

                    run(['cmake','--preset',profile]);command=['cmake','--build','--preset',profile,'--parallel',str(args.jobs),'--target',*build_targets];run(command)
                    if imported:run([sys.executable,root/'scripts/verify_module_consumers.py','--build-dir',build])
                    junit=work/(profile+'.xml');run(['ctest','--preset',profile,'--output-on-failure','--no-tests=error','--output-junit',junit,'-R','^('+'|'.join(tests)+')$'])
                    tree=ET.parse(junit).getroot();assert sorted(x.attrib['name'] for x in tree.findall('.//testcase'))==sorted(tests)
                    assert not any(tree.findall('.//'+tag) for tag in ['failure','error','skipped'])
                    assert 'ninja: no work to do.' in run(command)
                    gate=dict(tests=tests,junit=junit.read_text(),post_build_no_work=True)
                actual=observe(str(binary.resolve()),rows);differences=[i for i,(a,b) in enumerate(zip(expected,actual)) if not same(a,b)]
                if args.suite=='dynamic' and not args.expect_differences:gate['semantic_assertions']=assert_dynamic_contract([dict(input=i,expected=e) for i,e in zip(rows,actual)])
                if args.suite=='dynamic_loaded' and not args.expect_differences:gate['semantic_assertions']=assert_dynamic_loaded_contract([dict(input=i,expected=e) for i,e in zip(rows,actual)])
                if args.suite=='enhanced' and not args.expect_differences:gate['semantic_assertions']=assert_enhanced_contract([dict(input=i,expected=e) for i,e in zip(rows,actual)])
                record['profiles'][profile]=dict(observations=actual,differing_indices=differences,binary_sha256=digest(binary),gate=gate);save()
                assert bool(differences)==args.expect_differences,(profile,len(differences))
            view.verify()
        assert capture_inputs()==record['inputs']|record.get('generated_inputs',{}) and references()==record['references']
        record.update(complete=True,unchanged_inputs=True,original_tests=0,added_caller_tests=1,
            all_installed_frontends=not args.expect_differences and set(record['profiles'])=={'native','modules_native'});save()
        print(f'Actual resolved query {args.suite}:{count}; '+', '.join(f'{p}:{len(v["differing_indices"])} differences' for p,v in record['profiles'].items()),flush=True)
    except BaseException as error:
        record['controller_error']=dict(type=type(error).__name__,message=str(error));save();raise


if __name__=='__main__':main()
