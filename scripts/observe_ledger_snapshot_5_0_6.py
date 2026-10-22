#!/usr/bin/env python3
"""Observe ledger snapshot timing across a reentrant metadata callback."""
import gzip
import json
import os
from pathlib import Path
import subprocess
import tempfile
from reference_runtime_view_5_0_6 import ReferenceRuntimeView

ROOT = Path(__file__).resolve().parents[1]
RECORD = ROOT / 'docs/parity/ledger_snapshot_original_v2_5_0_6.json.gz'
OBSERVER = r'''
#[cfg(test)] mod cpp_ledger_snapshot_observer {
use std::{collections::BTreeMap, io, sync::{Arc, atomic::{AtomicUsize, Ordering}}, future::Future, task::{Context, Poll, Wake, Waker}};
use teaql_core::*;
use teaql_data_service::*;
use crate::{EntityRuntimeState, EntityKey, MetadataStore, UserContext};
struct Metadata {
    descriptors: Vec<EntityDescriptor>, root: EntityRuntimeState, trigger: usize,
    calls: Arc<AtomicUsize>, fired: Arc<std::sync::atomic::AtomicBool>,
}
impl MetadataStore for Metadata {
    fn entity(&self, name: &str) -> Option<&EntityDescriptor> {
        if name == "Deleted" && self.calls.fetch_add(1, Ordering::SeqCst) + 1 == self.trigger {
            self.root.set(EntityKey::new("Order", 7_u64), "name", Value::Text("after".into()));
            self.fired.store(true, Ordering::SeqCst);
        }
        self.descriptors.iter().find(|d| d.name == name)
    }
    fn all_entities(&self) -> Vec<&EntityDescriptor> { self.descriptors.iter().collect() }
}
struct Executor { names: std::sync::Mutex<Vec<String>> }
impl DataServiceExecutor for Executor { type Error = io::Error; fn capabilities(&self) -> DataServiceCapabilities { DataServiceCapabilities::default() } }
impl QueryExecutor for Executor {
    async fn query(&self, _: QueryRequest) -> Result<QueryResult, Self::Error> {
        Ok(QueryResult { rows: vec![CompactRow::from_map(BTreeMap::from([
            ("id".into(), Value::U64(7)), ("version".into(), Value::I64(1)), ("name".into(), Value::Text("loaded".into()))]))], metadata: ExecutionMetadata::unrecorded_query(1) })
    }
}
impl MutationExecutor for Executor {
    async fn mutate(&self, request: MutationRequest) -> Result<MutationResult, Self::Error> {
        if let MutationCommand::Update(command) = &request.command {
            if command.entity == "Order" { self.names.lock().unwrap().push(command.values.get("name").and_then(Value::try_text).unwrap().into()); }
        }
        let mut metadata=ExecutionMetadata::unrecorded_query(0); metadata.affected_rows=Some(1);
        Ok(MutationResult { affected_rows:1, generated_values:Default::default(), persisted_snapshot:None, metadata })
    }
}
struct NoWake;impl Wake for NoWake { fn wake(self: Arc<Self>) {} }
fn wait<F:Future>(future:F)->F::Output { let mut future=std::pin::pin!(future);let w=Waker::from(Arc::new(NoWake));let mut cx=Context::from_waker(&w);loop { if let Poll::Ready(value)=future.as_mut().poll(&mut cx) { return value; } } }
#[test] fn observe() {
    for trigger in 0..=3 {
        let root=EntityRuntimeState::default(); let order=EntityKey::new("Order",7_u64);let deleted=EntityKey::new("Deleted",9_u64);
        root.set_comment("verify ledger snapshot timing");root.set(order.clone(),"name",Value::Text("before".into()));root.set_original_version(order.clone(),1);root.mark_as_delete(deleted.clone());root.set_original_version(deleted,1);
        let calls=Arc::new(AtomicUsize::new(0));let fired=Arc::new(std::sync::atomic::AtomicBool::new(false));
        let descriptors=["Order","Deleted"].into_iter().map(|name|EntityDescriptor::new(name).property(PropertyDescriptor::new("id",DataType::U64).id()).property(PropertyDescriptor::new("version",DataType::I64).version()).property(PropertyDescriptor::new("name",DataType::Text))).collect();
        let mut context=UserContext::default().with_metadata(Metadata {descriptors,root:root.clone(),trigger,calls:calls.clone(),fired:fired.clone()});context.disable_sql_log();
        let executor=Executor{names:Default::default()};
        let result=wait(super::EntityDataService::for_executor(&context,"Order",&executor).execute_ledger_plan_internal(root.clone(),&Default::default()));
        let pending=root.current_change_set().changes().get(&order).unwrap().get("name").unwrap().try_text().unwrap().to_owned();
        println!("LEDGER_SNAPSHOT:{}",serde_json::json!({"trigger":trigger,"callback_fired":fired.load(Ordering::SeqCst),"names":*executor.names.lock().unwrap(),"pending_name":pending,"error":result.err().map(|e|e.to_string())}));
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
        with tempfile.TemporaryDirectory(prefix='teaql_ledger_snapshot_') as temporary:
            work = Path(temporary)
            env['CARGO_TARGET_DIR'] = str(work / 'target')
            observer = work / 'observer.rs'
            observer.write_text(OBSERVER)
            view = ReferenceRuntimeView(ROOT, work / 'runtime', {'src/data_service/graph.rs': observer}, run)
            output = run(['cargo', 'test', '--offline', '--locked', '--manifest-path', work / 'runtime/Cargo.toml',
                          '--lib', 'cpp_ledger_snapshot_observer::observe', '-j', '2', '--', '--nocapture'])
            receipt['observations'] = [json.loads(line.split(':', 1)[1]) for line in output.splitlines()
                                       if line.startswith('LEDGER_SNAPSHOT:')]
            assert len(receipt['observations']) == 4
            receipt['provenance'] = view.provenance()
            receipt['disposable_bytes'] = sum(p.stat().st_size for p in work.rglob('*') if p.is_file())
        receipt['disposable_outputs_removed'] = True
        run(['python3', 'scripts/verify_upstream.py'])
        receipt['complete'] = True
    finally:
        RECORD.write_bytes(gzip.compress((json.dumps(receipt, indent=2) + '\n').encode(), mtime=0))
    print('Observed four original ledger snapshot controls; owned outputs removed', flush=True)


if __name__ == '__main__':
    main()
