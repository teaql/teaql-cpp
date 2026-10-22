#!/usr/bin/env python3
"""Observe complete private graph outcome model defaults, maps and diagnostics."""
import gzip
import json
import os
from pathlib import Path
import subprocess
import tempfile
from reference_runtime_view_5_0_6 import ReferenceRuntimeView

ROOT = Path(__file__).resolve().parents[1]
RECORD = ROOT / 'docs/parity/graph_outcome_models_original_5_0_6.json.gz'
OBSERVER = r'''
#[cfg(test)] mod cpp_graph_outcome_models_observer {
use super::{DynamicReadbacks, LedgerExecutionOutcome};
use crate::EntityKey;
use teaql_core::{Value, DataType};
use teaql_core::dynamic_fields::{DynamicFieldDefinitions, DynamicFieldValues};
#[test] fn observe() {
    for generated in 0..3 {
        for payload in ["absent", "readbacks_empty", "fields_empty", "null", "text"] {
            let mut outcome = LedgerExecutionOutcome::default();
            for index in 0..generated {
                outcome.generated_ids.insert(EntityKey::new(if index == 0 { "Order" } else { "Child" }, 7_u64), Value::U64(100 + index));
            }
            if payload != "absent" {
                let mut readbacks = DynamicReadbacks::default();
                if payload != "readbacks_empty" {
                    let definitions = DynamicFieldDefinitions::new("Order", "1", [("note".into(), DataType::Text)]).unwrap();
                    let mut values = std::collections::HashMap::new();
                    if payload == "null" { values.insert("note".into(), Value::Null); }
                    if payload == "text" { values.insert("note".into(), Value::Text("retained outcome payload".into())); }
                    let fields = DynamicFieldValues::from_values(definitions, values).unwrap();
                    readbacks.insert(EntityKey::new("Order", 7_u64), fields);
                }
                outcome.dynamic_readbacks = Some(Box::new(readbacks));
            }
            assert_eq!(outcome.len(), generated as usize);
            assert_eq!(outcome.is_empty(), generated == 0);
            if generated != 0 { assert_eq!(outcome.get(&EntityKey::new("Order", 7_u64)), Some(&Value::U64(100))); }
            println!("GRAPH_OUTCOME_MODEL:{}", serde_json::json!({"generated":generated,"payload":payload,"debug":format!("{outcome:?}"),"dynamic_debug":outcome.dynamic_readbacks.as_ref().map(|rows|format!("{rows:?}"))}));
        }
    }
}
}
'''


def main():
    if RECORD.exists():
        raise SystemExit('Preserve existing receipt')
    receipt = dict(complete=False, steps=[], controller=Path(__file__).read_text())
    env = dict(os.environ, CARGO_INCREMENTAL='0', CARGO_PROFILE_DEV_DEBUG='0',
               CARGO_PROFILE_TEST_DEBUG='0', TEAQL_LOG_ENDPOINT='off',
               CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')

    def run(command):
        command = list(map(str, command))
        result = subprocess.run(command, cwd=ROOT, env=env, capture_output=True, text=True)
        receipt['steps'].append(dict(command=command, exit_code=result.returncode,
                                     stdout=result.stdout, stderr=result.stderr))
        result.check_returncode()
        return result.stdout

    try:
        run(['python3', 'scripts/verify_upstream.py'])
        with tempfile.TemporaryDirectory(prefix='teaql_graph_outcome_models_') as temporary:
            work = Path(temporary)
            env['CARGO_TARGET_DIR'] = str(work / 'target')
            observer = work / 'observer.rs'
            observer.write_text(OBSERVER)
            view = ReferenceRuntimeView(ROOT, work / 'runtime', {'src/data_service/graph.rs': observer}, run)
            output = run(['cargo', 'test', '--offline', '--locked', '--manifest-path', work / 'runtime/Cargo.toml',
                          '--lib', 'cpp_graph_outcome_models_observer::observe', '-j', '2', '--', '--nocapture'])
            receipt['observations'] = [json.loads(line.split(':', 1)[1]) for line in output.splitlines()
                                       if line.startswith('GRAPH_OUTCOME_MODEL:')]
            assert len(receipt['observations']) == 15
            receipt['provenance'] = view.provenance()
            receipt['disposable_bytes'] = sum(p.stat().st_size for p in work.rglob('*') if p.is_file())
        receipt['disposable_outputs_removed'] = True
        run(['python3', 'scripts/verify_upstream.py'])
        receipt['complete'] = True
    finally:
        RECORD.write_bytes(gzip.compress((json.dumps(receipt, indent=2) + '\n').encode(), mtime=0))
    print('Observed 15 original graph outcome model controls; owned outputs removed', flush=True)


if __name__ == '__main__':
    main()
