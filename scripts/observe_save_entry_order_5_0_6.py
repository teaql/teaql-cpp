#!/usr/bin/env python3
"""Observe generic and ledger Save metadata, audit and carrier ordering."""
import gzip
import json
import os
from pathlib import Path
import subprocess
import tempfile
from reference_runtime_view_5_0_6 import ReferenceRuntimeView

ROOT = Path(__file__).resolve().parents[1]
RECORD = ROOT / 'docs/parity/save_entry_order_original_5_0_6.json.gz'
OBSERVER = r'''
#[cfg(test)] mod cpp_save_entry_order_observer {
use teaql_core::*;
use teaql_core::dynamic_fields::{DynamicFieldDefinitions,DynamicFieldValues};
use crate::{AuditedSaveExt,EntityRuntimeState,UserContext,InMemoryMetadataStore,FixEvidence,FixEvidenceSource};
static EVENTS: std::sync::Mutex<Vec<&'static str>> = std::sync::Mutex::new(Vec::new());
fn note(event: &'static str) { EVENTS.lock().unwrap().push(event); }
struct Model { values: MutationValues, fields: Option<DynamicFieldValues>, root: Option<EntityRuntimeState>, comment: Option<String> }
impl TeaqlEntity for Model {
    const ENTITY_NAME: &'static str = "Order";
    fn entity_descriptor() -> EntityDescriptor {
        note("descriptor");
        EntityDescriptor::new("Order").property(PropertyDescriptor::new("id",DataType::U64).id())
            .property(PropertyDescriptor::new("version",DataType::I64).version())
            .property(PropertyDescriptor::new("name",DataType::Text))
    }
}
impl Entity for Model {
    fn from_compact_row(row: CompactRow) -> Result<Self,EntityError> {
        Ok(Self { values:row.into_map().into(),fields:None,root:None,comment:None })
    }
    fn into_values(self) -> MutationValues { self.values }
    fn dynamic_field_values(&self) -> Option<&DynamicFieldValues> { self.fields.as_ref() }
    fn __teaql_runtime_state_any(&self) -> Option<&dyn std::any::Any> {
        note("generic_carrier"); self.root.as_ref().map(|state| state as &dyn std::any::Any)
    }
    fn set_comment(&mut self,comment:String) { note("audit"); self.comment=Some(comment); }
    fn get_comment(&self) -> Option<String> { self.comment.clone() }
}

impl crate::LedgerEntity for Model {
    fn entity_runtime_state(&self)->Option<EntityRuntimeState> { note("ledger_carrier"); self.root.clone() }
}
#[tokio::test] async fn observe() {
    for ledger in [false,true] {
        for dynamic in [false,true] {
            for carrier in [false,true] {
                let context=UserContext::new().with_metadata(InMemoryMetadataStore::new().with_entity(Model::entity_descriptor()));
                let values: std::collections::BTreeMap<String,Value>=[("id".to_owned(),Value::U64(7)),("version".to_owned(),Value::I64(1)),("name".to_owned(),Value::Text("loaded".into()))].into_iter().collect();
                let fields=dynamic.then(||DynamicFieldValues::from_values(DynamicFieldDefinitions::new("Order","1",[]).unwrap(),Default::default()).unwrap());
                let entity=Model { values:MutationValues::from(values),fields,root:carrier.then(EntityRuntimeState::default),comment:None };
                EVENTS.lock().unwrap().clear();
                let result=if ledger { crate::save_audited_ledger_entity(entity.audit_as("verify save entry order"),&context).await }
                    else { entity.audit_as("verify save entry order").save(&context).await };
                let error=result.err().expect("missing saver or carrier").to_string();
                let route=if error.contains("no DynGraphSaver registered") { "missing_saver" }
                    else { assert!(error.contains("dynamic save requires the runtime mutation ledger carrier")); "missing_carrier" };
                assert_eq!(route,if !ledger && dynamic && !carrier {"missing_carrier"} else {"missing_saver"});
                println!("SAVE_ENTRY_ORDER:{}",serde_json::json!({"ledger":ledger,"dynamic":dynamic,"carrier":carrier,"route":route,"error":error,"events":*EVENTS.lock().unwrap()}));
            }
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
        with tempfile.TemporaryDirectory(prefix='teaql_save_entry_order_') as temporary:
            work = Path(temporary)
            env['CARGO_TARGET_DIR'] = str(work / 'target')
            observer = work / 'observer.rs'
            observer.write_text(OBSERVER)
            view = ReferenceRuntimeView(ROOT, work / 'runtime', {'src/entity_save.rs': observer}, run)
            output = run(['cargo', 'test', '--offline', '--locked', '--manifest-path', work / 'runtime/Cargo.toml',
                          '--lib', 'cpp_save_entry_order_observer::observe', '-j', '2', '--', '--nocapture'])
            receipt['observations'] = [json.loads(line.split(':', 1)[1]) for line in output.splitlines()
                                       if line.startswith('SAVE_ENTRY_ORDER:')]
            assert len(receipt['observations']) == 8
            receipt['provenance'] = view.provenance()
            receipt['disposable_bytes'] = sum(p.stat().st_size for p in work.rglob('*') if p.is_file())
        receipt['disposable_outputs_removed'] = True
        run(['python3', 'scripts/verify_upstream.py'])
        receipt['complete'] = True
    finally:
        RECORD.write_bytes(gzip.compress((json.dumps(receipt, indent=2) + '\n').encode(), mtime=0))
    print(receipt['observations'], flush=True)


if __name__ == '__main__':
    main()
