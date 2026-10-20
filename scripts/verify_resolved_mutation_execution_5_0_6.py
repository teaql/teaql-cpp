#!/usr/bin/env python3
"""Replay complete original resolved mutation execution and event delivery with trusted intent."""
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
    parser.add_argument('--suite',choices=['execution','privacy'],default='execution')
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
    if args.suite=='privacy':
        from resolved_mutation_privacy_inputs_5_0_6 import inputs
    else:
        from resolved_mutation_execution_inputs_5_0_6 import inputs
    expected_count=5486 if args.suite=='privacy' else 3518
    corpus_name='resolved_mutation_privacy' if args.suite=='privacy' else 'resolved_mutation_execution'

    from reference_runtime_view_5_0_6 import ReferenceRuntimeView
    from run_native_integration_epoch_5_0_6 import capture_inputs,references
    digest=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
    assets=['tests/parity/runtime_mutation_probe.cpp',
            'tests/parity/build_runtime_mutation_oracle.py',
            'tests/parity/rust_private_save_5_0_6/src/bin/runtime_mutation_5_0_6.rs',
            'tests/parity/resolved_mutation_execution_source_5_0_6.rs',
            'scripts/verify_resolved_mutation_execution_5_0_6.py',
            'tests/parity/resolved_mutation_execution_inputs_5_0_6.py',
            'teaql_runtime/include/teaql/runtime/detail/data_service/entity_service.hpp',
            'teaql_runtime/include/teaql/runtime/detail/data_service/entity_mutation.hpp']
    if args.suite=='privacy':
        assets += ['tests/parity/resolved_mutation_privacy_inputs_5_0_6.py',
                   'tests/parity/resolved_mutation_privacy_observer_5_0_6.rs']
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
        rows=inputs();assert len(rows)==expected_count
        with tempfile.TemporaryDirectory(prefix='teaql_resolved_mutation_execution_506_') as temporary:
            work=Path(temporary);input_path=work/'inputs.json';input_path.write_text(json.dumps(rows)+'\n')
            fixture=root/'tests/parity/resolved_mutation_execution_source_5_0_6.rs';marker='__TEAQL_RESOLVED_EXECUTION_INPUTS__';assert fixture.read_text().count(marker)==1
            import ast
            bridge_path=root/'tests/parity/build_runtime_mutation_oracle.py'
            bridge=next(ast.literal_eval(node.value) for node in ast.parse(bridge_path.read_text()).body
                        if isinstance(node,ast.Assign) and any(isinstance(target,ast.Name) and target.id=='BRIDGE' for target in node.targets))
            bridge=bridge.replace('mutation: __CppMutation, lineage: Vec<teaql_core::TraceNode>)',
                                  'mutation: __CppMutation, lineage: Vec<teaql_core::TraceNode>, bound_intent: bool)')
            needle='    let runtime = RuntimeDataService::new(&service.data_service.metadata, executor);'
            assert bridge.count(needle)==1
            bridge=bridge.replace(needle, '    let service = if bound_intent { service.with_mutation_intent(teaql_core::MutationIntent::new("verify event execution").unwrap()) } else { service };\n'+needle)
            driver_path=root/'tests/parity/rust_private_save_5_0_6/src/bin/runtime_mutation_5_0_6.rs'
            driver=driver_path.read_text();driver=driver[driver.index('use std::'):]
            driver=driver[:driver.index('fn main()')]
            driver=driver.replace('use teaql_runtime::{','use crate::{').replace(',__CppMutation,__cpp_mutation','')
            driver=driver.replace('../../../rust_values/src/fixture_value.rs',str(root/'tests/parity/rust_values/src/fixture_value.rs'))
            needle='command(&j),trace(&j["lineage"])'
            assert driver.count(needle)==1
            driver=driver.replace(needle,needle+',j["bound_intent"]==true')
            if args.suite=='privacy':
                needle='bound_intent: bool)'
                assert bridge.count(needle)==1
                bridge=bridge.replace(needle,'bound_intent: bool, privacy_kind: Option<&str>, replace_intent: bool)')
                needle='    let runtime = RuntimeDataService::new(&service.data_service.metadata, executor);'
                assert bridge.count(needle)==1
                bridge=bridge.replace(needle,'    let service = apply_privacy(service,privacy_kind,replace_intent);\n'+needle)
                needle='state:Arc<State>,mode:String'
                # Extend only observer state and observed driver surfaces.
                needle='struct State{notes:'
                assert driver.count(needle)==1
                driver=driver.replace(needle,'struct State{privacy_probe:bool,event_privacy:Mutex<Vec<J>>,notes:')
                needle='Arc::new(State::default())'
                assert driver.count(needle)==1
                driver=driver.replace(needle,'Arc::new(State{privacy_probe:j["privacy_probe"]==true,..State::default()})')
                needle='self.state.note("event");'
                assert driver.count(needle)==1
                driver=driver.replace(needle,needle+'if self.state.privacy_probe{self.state.event_privacy.lock().unwrap().push(privacy_event_out(e));}')
                needle='self.state.requests.lock().unwrap().push(request_out(&r));'
                assert driver.count(needle)==1
                driver=driver.replace(needle,'let mut observed=request_out(&r);if self.state.privacy_probe{observed["privacy"]=json!(r.diagnostic_redactions().map(SqlIntentRedactions::is_empty));observed["intent"]=json!(r.comment());}self.state.requests.lock().unwrap().push(observed);')
                needle='j["bound_intent"]==true'
                assert driver.count(needle)==1
                driver=driver.replace(needle,needle+',j["privacy"].as_str(),j["replace_intent"]==true')
                needle='let output=json!('
                assert driver.count(needle)==1
                driver=driver.replace(needle,'let mut output=json!(')
                needle=';output}'
                assert driver.count(needle)==1
                driver=driver.replace(needle,';if state.privacy_probe{output["event_privacy"]=json!(*state.event_privacy.lock().unwrap());}output}')
                driver += '\n'+(root/'tests/parity/resolved_mutation_privacy_observer_5_0_6.rs').read_text()
            rendered=fixture.read_text().replace(marker,str(input_path)).replace('__TEAQL_MUTATION_ACCESS_BRIDGE__',bridge).replace('__TEAQL_MUTATION_RETAINED_DRIVER__',driver)
            observer=work/'observer.rs';observer.write_text(rendered)
            record['observer_rendering']=dict(driver_sha256=digest(driver_path),bridge_sha256=digest(bridge_path),rendered=rendered,
                scope='Access-only forwarding plus trusted mutation-intent binding; retain driver application SPI bodies and actual original runtime.')
            view=ReferenceRuntimeView(root,work/'runtime',{'src/data_service/resolved.rs':observer},run)
            output=run(['cargo','test','--offline','--locked','--manifest-path',view.directory/'Cargo.toml','-j2',
                        '--lib','data_service::resolved::cpp_resolved_mutation_execution_observer::','--','--nocapture','--test-threads=1'])
            names=re.findall(r'^test (\S+) \.\.\.',output,flags=re.M)
            assert names==['data_service::resolved::cpp_resolved_mutation_execution_observer::replay_bound_mutations_and_events'] and '1 passed; 0 failed' in output
            expected=[json.loads(line.split('TEAQL_RESOLVED_EXECUTION_JSON ',1)[1]) for line in output.split('\n') if 'TEAQL_RESOLVED_EXECUTION_JSON ' in line]
            assert len(expected)==len(rows)==expected_count
            record['original']=dict(tests=names,observations=expected,runtime_view=view.provenance())
            retained=json.loads(gzip.decompress((root/'tests/parity/runtime_mutation_5_0_6_cases.json.gz').read_bytes()))
            assert retained['baseline']==record['references']['teaql-rs']
            assert same(rows[:1502],[case['input'] for case in retained['cases']])
            assert same(expected[:1502],[case['expected'] for case in retained['cases']])
            if args.suite=='privacy':
                previous=json.loads(gzip.decompress((root/'tests/parity/resolved_mutation_execution_5_0_6_cases.json.gz').read_bytes()))
                assert same(rows[:3518],[case['input'] for case in previous['cases']])
                assert same(expected[:3518],[case['expected'] for case in previous['cases']])
            path=root/('tests/parity/'+corpus_name+'_5_0_6_cases.json.gz')
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
                    assert binary.resolve()==build/('modules' if imported else '')/(prefix+'runtime_mutation_probe')
                    targets=['runtime_mutation_probe','context_query_tests','entity_service_tests','schema_runtime_tests','resolved_select_preparation_probe','resolved_mutation_preparation_probe']
                    tests=['resolved_mutation_execution_rust_parity','context_query_contracts','entity_service_contracts','schema_runtime_contracts','resolved_select_preparation_rust_parity','resolved_mutation_preparation_rust_parity']
                    if args.suite=='privacy': tests.append('resolved_mutation_privacy_rust_parity')
                    if imported:
                        targets.remove('entity_service_tests')
                        tests.remove('entity_service_contracts')
                        tests=['module_'+x for x in tests]
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
        print(f'Actual resolved mutation {args.suite}:{expected_count}; '+', '.join(f'{p}:{len(v["differing_indices"])} differences' for p,v in record['profiles'].items()),flush=True)
    except BaseException as error:
        record['controller_error']=dict(type=type(error).__name__,message=str(error));save();raise


if __name__=='__main__':main()
