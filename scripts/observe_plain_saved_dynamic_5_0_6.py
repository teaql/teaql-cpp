#!/usr/bin/env python3
"""Observe plain versus dynamic Save result decoding in the locked original."""
import gzip
import json
import os
from pathlib import Path
import subprocess
import tempfile
from reference_runtime_view_5_0_6 import ReferenceRuntimeView

ROOT = Path(__file__).resolve().parents[1]
RECORD = ROOT / 'docs/parity/plain_saved_dynamic_original_5_0_6.json.gz'
OBSERVER = r'''
#[cfg(test)] mod cpp_plain_saved_dynamic_observer {
use teaql_core::*;
use teaql_core::dynamic_fields::{DynamicFieldDefinitions,DynamicFieldValues};
use crate::{AuditedSaveExt,EntityRuntimeState,UserContext,InMemoryMetadataStore,FixEvidence,FixEvidenceSource};
struct Model { values: MutationValues, fields: Option<DynamicFieldValues>, root: Option<EntityRuntimeState>, comment: Option<String> }
impl TeaqlEntity for Model {
    const ENTITY_NAME: &'static str = "Order";
    fn entity_descriptor() -> EntityDescriptor {
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
        self.root.as_ref().map(|state| state as &dyn std::any::Any)
    }
    fn set_comment(&mut self,comment:String) { self.comment=Some(comment); }
    fn get_comment(&self) -> Option<String> { self.comment.clone() }
}

struct Saver;
impl super::DynGraphSaver for Saver {
    fn save_graph_dyn<'a>(&'a self, _: &'a UserContext, mut node: crate::GraphNode)
        -> std::pin::Pin<Box<dyn std::future::Future<Output=Result<crate::GraphNode,crate::RuntimeError>> + Send + 'a>> {
        Box::pin(async move {
            node.dynamic_fields=Some(Box::new(DynamicFieldValues::from_values(
                DynamicFieldDefinitions::new("Order","1",[]).unwrap(),Default::default()).unwrap()));
            Ok(node)
        })
    }
    fn save_ledger_dyn<'a>(&'a self, context: &'a UserContext, node: crate::GraphNode, _: EntityRuntimeState)
        -> std::pin::Pin<Box<dyn std::future::Future<Output=Result<crate::GraphNode,crate::RuntimeError>> + Send + 'a>> {
        self.save_graph_dyn(context,node)
    }
}
#[tokio::test] async fn observe() {
    for dynamic in [false,true] {
        let mut context=UserContext::new().with_metadata(InMemoryMetadataStore::new().with_entity(Model::entity_descriptor()));
        context.insert_resource::<std::sync::Arc<dyn super::DynGraphSaver>>(std::sync::Arc::new(Saver));
        let values: std::collections::BTreeMap<String,Value>=[("id".to_owned(),Value::U64(7)),("version".to_owned(),Value::I64(1)),("name".to_owned(),Value::Text("loaded".into()))].into_iter().collect();
        let fields=dynamic.then(||DynamicFieldValues::from_values(DynamicFieldDefinitions::new("Order","1",[]).unwrap(),Default::default()).unwrap());
        let entity=Model { values:MutationValues::from(values),fields,root:Some(EntityRuntimeState::default()),comment:None };
        let result=entity.audit_as("verify saved dynamic decoding boundary").save(&context).await;
        let error=result.as_ref().err().map(|error|error.to_string());
        if dynamic {
            assert_eq!(error.as_deref(),Some("graph write error: indexed state missing from dynamic save result"));
        } else {
            assert!(result.is_ok());
            assert!(result.as_ref().unwrap().fields.is_none());
        }
        println!("PLAIN_SAVED_DYNAMIC:{}",serde_json::json!({"dynamic_input":dynamic,"ok":result.is_ok(),"error":error}));
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
        with tempfile.TemporaryDirectory(prefix='teaql_plain_saved_dynamic_') as temporary:
            work = Path(temporary)
            env['CARGO_TARGET_DIR'] = str(work / 'target')
            observer = work / 'observer.rs'
            observer.write_text(OBSERVER)
            view = ReferenceRuntimeView(ROOT, work / 'runtime', {'src/entity_save.rs': observer}, run)
            output = run(['cargo', 'test', '--offline', '--locked', '--manifest-path', work / 'runtime/Cargo.toml',
                          '--lib', 'cpp_plain_saved_dynamic_observer::observe', '-j', '2', '--', '--nocapture'])
            receipt['observations'] = [json.loads(line.split(':', 1)[1]) for line in output.splitlines()
                                       if line.startswith('PLAIN_SAVED_DYNAMIC:')]
            assert len(receipt['observations']) == 2
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
