#!/usr/bin/env python3
"""Compare nested public saves with byte-preserved Rust 5.0.6 source."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile

from reference_runtime_view_5_0_6 import ReferenceRuntimeView

ROOT = Path(__file__).resolve().parents[1]
OBSERVER = r'''
#[cfg(test)] mod cpp_nested_save_scope_observer {
use std::sync::{Arc,Mutex};
use teaql_core::*;
use teaql_core::dynamic_fields::{DynamicFieldDefinitions,DynamicFieldValues};
use crate::{AuditedSaveExt,EntityRuntimeState,UserContext,InMemoryMetadataStore,FixEvidence,FixEvidenceSource};

#[derive(Default)] struct Notes {
    parent: Option<Arc<Mutex<Vec<FixEvidence>>>>,
    inherits_parent: bool,
    inner_failed: bool,
    after_inner: Vec<String>,
}
struct NoExecutor;
impl teaql_data_service::DataServiceExecutor for NoExecutor {
    type Error=std::io::Error;
    fn capabilities(&self)->teaql_data_service::DataServiceCapabilities { Default::default() }
}
impl teaql_data_service::QueryExecutor for NoExecutor {
    async fn query(&self,_:teaql_data_service::QueryRequest)->Result<teaql_data_service::QueryResult,Self::Error> {
        panic!("nested scope fixture must not execute a query");
    }
}
impl teaql_data_service::MutationExecutor for NoExecutor {
    async fn mutate(&self,_:teaql_data_service::MutationRequest)->Result<teaql_data_service::MutationResult,Self::Error> {
        panic!("nested scope fixture must not execute a mutation");
    }
}
struct Model {
    context: Arc<UserContext>, notes: Arc<Mutex<Notes>>, route: &'static str, outer: bool,
    fields: Option<DynamicFieldValues>, root: Option<EntityRuntimeState>, comment: Option<String>,
}
fn model(context:Arc<UserContext>,notes:Arc<Mutex<Notes>>,route:&'static str,outer:bool)->Model {
    let dynamic=!outer && route=="dynamic";
    let fields=dynamic.then(||DynamicFieldValues::from_values(
        DynamicFieldDefinitions::new("Order","1",[]).unwrap(),Default::default()).unwrap());
    Model {context,notes,route,outer,fields,root:dynamic.then(EntityRuntimeState::default),comment:None}
}
fn record(context:&UserContext,label:&str) {
    context.record_fix_evidence(FixEvidence::new("Order","name",FixEvidenceSource::Context,label));
}
impl TeaqlEntity for Model {
    const ENTITY_NAME:&'static str="Order";
    fn entity_descriptor()->EntityDescriptor {
        EntityDescriptor::new("Order")
            .property(PropertyDescriptor::new("id",DataType::U64).id())
            .property(PropertyDescriptor::new("version",DataType::I64).version())
            .property(PropertyDescriptor::new("name",DataType::Text))
            .relation(RelationDescriptor::new("children","Order").many())
    }
}
impl Entity for Model {
    fn from_compact_row(_:CompactRow)->Result<Self,EntityError> {
        panic!("nested scope fixture must fail before hydration");
    }
    fn into_values(self)->MutationValues {
        if self.outer {
            self.notes.lock().unwrap().parent=Some(super::GRAPH_FIX_EVIDENCE.with(Arc::clone));
            record(&self.context,"outer_before");
            let entity=model(self.context.clone(),self.notes.clone(),self.route,false);
            let context=self.context.clone();
            let route=self.route;
            // Each selected future must finish before provider I/O. Polling once
            // keeps the nested call inside the original outer task-local scope.
            let mut operation=Box::pin(async move {
                let audited=entity.audit_as("nested scope verification");
                match route {
                    "ledger"=>crate::save_audited_ledger_entity(audited,&context).await.is_err(),
                    "executor"=>crate::save_audited_ledger_entity_with_executor(audited,&context,&NoExecutor).await.is_err(),
                    _=>audited.save(&context).await.is_err(),
                }
            });
            let mut task=std::task::Context::from_waker(std::task::Waker::noop());
            let failed=match std::future::Future::poll(operation.as_mut(),&mut task) {
                std::task::Poll::Ready(failed)=>failed,
                std::task::Poll::Pending=>panic!("controlled extraction error must complete immediately"),
            };
            let mut notes=self.notes.lock().unwrap();
            notes.inner_failed=failed;
            notes.after_inner=self.context.last_fix_evidence().into_iter().map(|e|e.source_label).collect();
            drop(notes);
            record(&self.context,"outer_after");
        } else {
            let mut notes=self.notes.lock().unwrap();
            notes.inherits_parent=super::GRAPH_FIX_EVIDENCE.with(|active|
                Arc::ptr_eq(active,notes.parent.as_ref().unwrap()));
            drop(notes);
            record(&self.context,"inner");
        }
        let values:std::collections::BTreeMap<String,Value>=[
            ("id".into(),Value::U64(7)),("version".into(),Value::I64(1)),
            ("name".into(),Value::Text("fixture".into())),("children".into(),Value::Bool(true))
        ].into_iter().collect();
        values.into()
    }
    fn dynamic_field_values(&self)->Option<&DynamicFieldValues> {self.fields.as_ref()}
    fn __teaql_runtime_state_any(&self)->Option<&dyn std::any::Any> {
        self.root.as_ref().map(|state|state as &dyn std::any::Any)
    }
    fn set_comment(&mut self,comment:String) {self.comment=Some(comment);}
    fn get_comment(&self)->Option<String> {self.comment.clone()}
}
impl crate::LedgerEntity for Model {
    fn entity_runtime_state(&self)->Option<EntityRuntimeState> {self.root.clone()}
}
#[tokio::test] async fn observe() {
    for route in ["ledger","dynamic","plain","executor"] {
        let context=Arc::new(UserContext::new().with_metadata(
            InMemoryMetadataStore::new().with_entity(Model::entity_descriptor())));
        let notes=Arc::new(Mutex::new(Notes::default()));
        let outer=model(context.clone(),notes.clone(),route,true);
        let result=crate::save_audited_ledger_entity(
            outer.audit_as("outer scope verification"),&context).await;
        let notes=notes.lock().unwrap();
        let final_labels:Vec<_>=context.last_fix_evidence().into_iter().map(|e|e.source_label).collect();
        let plain=route=="plain";
        assert!(result.is_err() && notes.inner_failed);
        assert_eq!(notes.inherits_parent,plain);
        assert_eq!(notes.after_inner,if plain {vec![]} else {vec!["inner"]});
        assert_eq!(final_labels,if plain {vec!["outer_before","inner","outer_after"]}
            else {vec!["outer_before","outer_after"]});
        println!("NESTED_SAVE_SCOPE:{}",serde_json::json!({"route":route,
            "inherits_parent":notes.inherits_parent,"inner_failed":notes.inner_failed,
            "outer_failed":result.is_err(),"after_inner":notes.after_inner,"final":final_labels}));
    }
}
}
'''


def observations(output):
    prefix = 'NESTED_SAVE_SCOPE:'
    return [json.loads(line[len(prefix):]) for line in output.splitlines()
            if line.startswith(prefix)]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cpp', required=True, type=Path)
    parser.add_argument('--record', required=True, type=Path)
    parser.add_argument('--expect-differences', type=int, default=0)
    parser.add_argument('--scope-header', type=Path,
                        help='Actual header used by an explicitly recorded counterfactual compiler')
    args = parser.parse_args()
    if args.record.exists():
        raise SystemExit('Preserve the existing receipt; select a new record path')
    receipt = dict(complete=False, controller=Path(__file__).read_text(), steps=[])
    env = dict(os.environ, CARGO_INCREMENTAL='0', CARGO_PROFILE_DEV_DEBUG='0',
               CARGO_PROFILE_TEST_DEBUG='0', TEAQL_LOG_ENDPOINT='off',
               CARGO_TARGET_X86_64_UNKNOWN_LINUX_GNU_LINKER='/usr/bin/cc')

    def run(command, allow_failure=False):
        command = list(map(str, command))
        result = subprocess.run(command, cwd=ROOT, env=env, capture_output=True, text=True)
        receipt['steps'].append(dict(command=command, exit_code=result.returncode,
                                     stdout=result.stdout, stderr=result.stderr))
        if not allow_failure:
            result.check_returncode()
        return result

    try:
        run(['python3', 'scripts/verify_upstream.py'])
        with tempfile.TemporaryDirectory(prefix='teaql_nested_save_scope_') as temporary:
            work = Path(temporary)
            env['CARGO_TARGET_DIR'] = str(work / 'target')
            observer = work / 'observer.rs'
            observer.write_text(OBSERVER)
            view = ReferenceRuntimeView(ROOT, work / 'runtime', {'src/entity_save.rs': observer},
                                        lambda command: run(command).stdout)
            original = run(['cargo', 'test', '--offline', '--locked', '--manifest-path',
                            work / 'runtime/Cargo.toml', '--lib',
                            'cpp_nested_save_scope_observer::observe', '-j', '2', '--', '--nocapture'])
            receipt['original'] = observations(original.stdout)
            receipt['provenance'] = view.provenance()
            receipt['disposable_bytes'] = sum(p.stat().st_size for p in work.rglob('*') if p.is_file())
        receipt['disposable_outputs_removed'] = True
        cpp = args.cpp.resolve()
        receipt['candidate_sha256'] = hashlib.sha256(cpp.read_bytes()).hexdigest()
        for relative in ('teaql_runtime/include/teaql/runtime/detail/graph_fix_scope.hpp',
                         'teaql_runtime/tests/graph_fix_tests.cpp',
                         'tests/support/nested_save_scope_fixture.hpp'):
            receipt.setdefault('candidate_sources', {})[relative] = hashlib.sha256(
                (ROOT / relative).read_bytes()).hexdigest()
        if args.scope_header:
            scope = args.scope_header.resolve()
            receipt['counterfactual_scope_header'] = dict(path=str(scope), text=scope.read_text(),
                sha256=hashlib.sha256(scope.read_bytes()).hexdigest())
        candidate = run([cpp], allow_failure=bool(args.expect_differences))
        receipt['candidate'] = observations(candidate.stdout)
        assert len(receipt['original']) == len(receipt['candidate']) == 4
        receipt['differences'] = [dict(original=a, candidate=b)
                                  for a, b in zip(receipt['original'], receipt['candidate']) if a != b]
        assert len(receipt['differences']) == args.expect_differences
        if args.expect_differences:
            assert candidate.returncode != 0
        run(['python3', 'scripts/verify_upstream.py'])
        receipt['complete'] = True
    finally:
        args.record.parent.mkdir(parents=True, exist_ok=True)
        args.record.write_bytes(gzip.compress((json.dumps(receipt, indent=2) + '\n').encode(), mtime=0))
    print(f"Nested public saves: 4 observed; {len(receipt['differences'])} differences; "
          f"removed {receipt['disposable_bytes']} disposable bytes", flush=True)


if __name__ == '__main__':
    main()
