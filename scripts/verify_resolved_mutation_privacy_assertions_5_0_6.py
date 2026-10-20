#!/usr/bin/env python3
"""Join unchanged source/ELF evidence with explicit privacy assertions and fresh CTest."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--record',type=Path,required=True)
    args=parser.parse_args()
    if args.record.exists() or args.record.suffix!='.gz':parser.error('Use a fresh .gz receipt')
    root=Path(__file__).resolve().parents[1]
    sys.path[:0]=[str(root/'scripts'),str(root/'tests/parity')]
    from run_native_integration_epoch_5_0_6 import capture_inputs,references
    from compare_values import same
    from resolved_mutation_privacy_inputs_5_0_6 import inputs,assert_privacy_contract
    digest=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
    parent_path=root/'docs/parity/resolved_mutation_privacy_installed_5_0_6.json.gz'
    parent=json.loads(gzip.decompress(parent_path.read_bytes()))
    assert parent['complete'] and parent['unchanged_inputs'] and parent['all_installed_frontends']
    current=capture_inputs()
    differences={key for key in current.keys()|parent['inputs'].keys() if current.get(key)!=parent['inputs'].get(key)}
    allowed={'runtime:tests/parity/resolved_mutation_privacy_inputs_5_0_6.py',
             'runtime:tests/parity/compare_resolved_mutation_privacy_5_0_6.py',
             'runtime:scripts/verify_resolved_mutation_privacy_assertions_5_0_6.py'}
    assert differences<=allowed,differences-allowed
    record=dict(complete=False,steps=[],profiles={},inputs=current,references=references(),
                parent=dict(path=str(parent_path.relative_to(root)),sha256=digest(parent_path)),
                allowed_observer_changes=sorted(differences),controller=Path(__file__).read_text(),
                scope='Reuse exact unchanged original runtime and binary evidence; no fresh Rust execution or production/capability promotion')
    assert record['references']==parent['references']
    def save():
        args.record.parent.mkdir(parents=True,exist_ok=True)
        args.record.write_bytes(gzip.compress((json.dumps(record,indent=2)+'\n').encode(),mtime=0))
    def run(command):
        result=subprocess.run([str(x) for x in command],cwd=root,text=True,capture_output=True)
        record['steps'].append(dict(command=[str(x) for x in command],exit_code=result.returncode,stdout=result.stdout,stderr=result.stderr))
        save();result.check_returncode();return result.stdout
    try:
        corpus=root/'tests/parity/resolved_mutation_privacy_5_0_6_cases.json.gz'
        cases=json.loads(gzip.decompress(corpus.read_bytes()))['cases']
        assert same(inputs(),[case['input'] for case in cases])
        assert same(parent['original']['observations'],[case['expected'] for case in cases])
        record['source_assertions']=assert_privacy_contract(cases)
        with tempfile.TemporaryDirectory(prefix='teaql_mutation_privacy_assertions_') as directory:
            for profile,binary in [('native',root/'build_native_current/teaql_runtime_mutation_probe'),
                                   ('modules_native',root/'build_modules_native/modules/teaql_module_runtime_mutation_probe')]:
                previous=parent['profiles'][profile]
                assert digest(binary)==previous['binary_sha256']
                assert same(previous['observations'],[case['expected'] for case in cases])
                assertions=assert_privacy_contract([dict(input=case['input'],expected=observed)
                    for case,observed in zip(cases,previous['observations'])])
                commands=[step['command'] for step in parent['steps'] if step['command'][:4]==['cmake','--build','--preset',profile]]
                assert commands and all(command==commands[0] for command in commands)
                assert 'ninja: no work to do.' in run(commands[0])
                junit=Path(directory)/(profile+'.xml');tests=previous['gate']['tests']
                run(['ctest','--preset',profile,'--output-on-failure','--no-tests=error','--output-junit',junit,'-R','^('+'|'.join(tests)+')$'])
                tree=ET.parse(junit).getroot()
                assert sorted(x.attrib['name'] for x in tree.findall('.//testcase'))==sorted(tests)
                assert not any(tree.findall('.//'+tag) for tag in ['failure','error','skipped'])
                assert digest(binary)==previous['binary_sha256']
                record['profiles'][profile]=dict(assertions=assertions,tests=tests,junit=junit.read_text(),binary_sha256=digest(binary),no_work=True)
        assert capture_inputs()==current and references()==record['references']
        record.update(complete=True,unchanged_inputs=True,source_promoted=False);save()
        print('Privacy assertions and fresh native7/module6 pass; source/ELF and corpus unchanged')
    except BaseException as error:
        record['controller_error']=dict(type=type(error).__name__,message=str(error));save();raise


if __name__=='__main__':main()
