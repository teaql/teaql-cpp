#!/usr/bin/env python3
"""Observe explicit-executor graph fallback with selected dynamic fields."""
import copy
import gzip
import json
import os
from pathlib import Path
import subprocess
import tempfile

from reference_runtime_view_5_0_6 import ReferenceRuntimeView

ROOT = Path(__file__).resolve().parents[1]
RECORD = ROOT / 'docs/parity/explicit_saved_dynamic_original_5_0_6.json.gz'


def main():
    if RECORD.exists():
        raise SystemExit('Preserve existing observation receipt')
    base = json.loads(gzip.decompress((ROOT / 'tests/parity/audited_save_5_0_6_cases.json.gz').read_bytes()))['cases'][0]['input']
    inputs = []
    for runtime in (False, True):
        for dynamic in (False, True):
            case = copy.deepcopy(base)
            case.update(runtime=runtime, dynamic=dynamic, reason='review saved dynamic result', checker='none', rows=[copy.deepcopy(base['values'])])
            inputs.append(case)
    receipt = {'complete': False, 'inputs': inputs, 'steps': [], 'controller': Path(__file__).read_text()}
    env = dict(os.environ, CARGO_INCREMENTAL='0', CARGO_PROFILE_DEV_DEBUG='0',
               CARGO_PROFILE_TEST_DEBUG='0', TEAQL_LOG_ENDPOINT='off',
               CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')

    def run(command):
        command = list(map(str, command))
        result = subprocess.run(command, cwd=ROOT, env=env, text=True, capture_output=True)
        receipt['steps'].append(dict(command=command, exit_code=result.returncode,
                                     stdout=result.stdout, stderr=result.stderr))
        result.check_returncode()
        return result.stdout

    try:
        run(['python3', 'scripts/verify_upstream.py'])
        with tempfile.TemporaryDirectory(prefix='teaql_explicit_saved_dynamic_') as temporary:
            work = Path(temporary)
            env['CARGO_TARGET_DIR'] = str(work / 'target')
            driver = (ROOT / 'tests/parity/rust_field_layout/src/bin/audited_save_5_0_6.rs').read_text()
            driver = driver[driver.index('use std::'):driver.index('fn main()')]
            driver = driver.replace('teaql_runtime::', 'crate::').replace(
                '../../../rust_values/src/fixture_value.rs', str(ROOT / 'tests/parity/rust_values/src/fixture_value.rs'))
            driver = driver.replace('values:MutationValues,runtime:Option<EntityRuntimeState>,comment:Option<String>,',
                'values:MutationValues,runtime:Option<EntityRuntimeState>,comment:Option<String>,dynamic:Option<teaql_core::dynamic_fields::DynamicFieldValues>,')
            driver = driver.replace('values:row.into_map().into(),runtime:None,', 'values:row.into_map().into(),dynamic:None,runtime:None,')
            driver = driver.replace(' fn into_values(self)->MutationValues{self.values}',
                ' fn dynamic_field_values(&self)->Option<&teaql_core::dynamic_fields::DynamicFieldValues>{self.dynamic.as_ref()}\n fn into_values(self)->MutationValues{self.values}')
            driver = driver.replace('let model=Model{values:record(&j["values"]).into(),',
                'let model=Model{dynamic:if j["dynamic"]==true {Some(teaql_core::dynamic_fields::DynamicFieldValues::from_values(teaql_core::dynamic_fields::DynamicFieldDefinitions::new("Order","1",[]).unwrap(),Default::default()).unwrap())} else {None},values:record(&j["values"]).into(),')
            # Extend only fixture output, preserving all original runtime bodies.
            before = 'json!({"outcome":outcome,"notes":'
            after = 'json!({"root_scope":trace_out(&root.get_trace_chain(&EntityKey::new("Order", 7_u64))),"outcome":outcome,"notes":'
            assert driver.count(before) == 1
            driver = driver.replace(before, after)
            observer = work / 'observer.rs'
            observer.write_text('#[cfg(test)] mod cpp_explicit_saved_dynamic_observer {\n' + driver +
                '\n#[test] fn observe() { let inputs: J = serde_json::from_str(r###"' + json.dumps(inputs) +
                '"###).unwrap(); for input in inputs.as_array().unwrap() { println!("EXPLICIT_SAVED_DYNAMIC:{}", evaluate(input.clone())); } }\n}\n')
            view = ReferenceRuntimeView(ROOT, work / 'runtime', {'src/entity_save.rs': observer}, run)
            output = run(['cargo', 'test', '--offline', '--locked', '--manifest-path',
                          work / 'runtime/Cargo.toml', '--lib', 'cpp_explicit_saved_dynamic_observer::observe',
                          '-j', '2', '--', '--nocapture'])
            receipt['observations'] = [json.loads(line.split('EXPLICIT_SAVED_DYNAMIC:', 1)[1])
                                       for line in output.split('\n') if line.startswith('EXPLICIT_SAVED_DYNAMIC:')]
            assert len(receipt['observations']) == len(inputs)
            assert all('error' not in o['outcome'] and not o['outcome']['ledger'] for o in receipt['observations'])
            receipt['provenance'] = view.provenance()
            receipt['disposable_bytes'] = sum(p.stat().st_size for p in work.rglob('*') if p.is_file())
        receipt['disposable_outputs_removed'] = True
        run(['python3', 'scripts/verify_upstream.py'])
        receipt['complete'] = True
    finally:
        RECORD.write_bytes(gzip.compress((json.dumps(receipt, indent=2) + '\n').encode(), mtime=0))
    print('Original explicit saved dynamic observations:', len(inputs), flush=True)


if __name__ == '__main__':
    main()
