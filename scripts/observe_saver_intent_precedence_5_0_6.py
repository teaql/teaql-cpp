#!/usr/bin/env python3
"""Observe saver intent validation before missing executor lookup."""
import gzip
import json
import os
from pathlib import Path
import subprocess
import tempfile
from reference_runtime_view_5_0_6 import ReferenceRuntimeView

ROOT = Path(__file__).resolve().parents[1]
RECORD = ROOT / 'docs/parity/saver_intent_precedence_original_5_0_6.json.gz'
OBSERVER = r'''
#[cfg(test)] mod cpp_saver_intent_precedence_observer {
use teaql_data_service::*;
use crate::{UserContext,RuntimeError,GraphNode,EntityRuntimeState};
use super::{GraphSaverFor,DynGraphSaver};
struct UnusedExecutor;
struct UnusedTx;
impl DataServiceExecutor for UnusedExecutor { type Error=std::io::Error; fn capabilities(&self)->DataServiceCapabilities { DataServiceCapabilities::default() } }
impl DataServiceExecutor for UnusedTx { type Error=std::io::Error; fn capabilities(&self)->DataServiceCapabilities { DataServiceCapabilities::default() } }
impl QueryExecutor for UnusedExecutor { async fn query(&self,_:QueryRequest)->Result<QueryResult,std::io::Error> { panic!("query must not execute") } }
impl QueryExecutor for UnusedTx { async fn query(&self,_:QueryRequest)->Result<QueryResult,std::io::Error> { panic!("query must not execute") } }
impl MutationExecutor for UnusedExecutor { async fn mutate(&self,_:MutationRequest)->Result<MutationResult,std::io::Error> { panic!("mutation must not execute") } }
impl MutationExecutor for UnusedTx { async fn mutate(&self,_:MutationRequest)->Result<MutationResult,std::io::Error> { panic!("mutation must not execute") } }
impl Transaction for UnusedTx {
    type Error=std::io::Error;
    async fn commit(self)->Result<(),std::io::Error> { panic!("commit must not execute") }
    async fn rollback(self)->Result<(),std::io::Error> { panic!("rollback must not execute") }
}
impl TransactionExecutor for UnusedExecutor {
    type Tx<'tx>=UnusedTx where Self:'tx;
    async fn begin(&self)->Result<UnusedTx,std::io::Error> { panic!("begin must not execute") }
}
#[tokio::test] async fn observe() {
    for ledger in [false,true] {
        for (label,comment) in [("absent",None),("empty",Some("")),("blank",Some(" \t")),("valid",Some("review save"))] {
            let context=UserContext::new();
            let mut node=GraphNode::new("Order");
            if let Some(comment)=comment { node.set_comment(comment); }
            let saver=GraphSaverFor::<UnusedExecutor>::new();
            let result=if ledger { saver.save_ledger_dyn(&context,node,EntityRuntimeState::default()).await }
                else { saver.save_graph_dyn(&context,node).await };
            let error=result.err().expect("missing intent or executor");
            let intent=matches!(&error,RuntimeError::RequestIntent(_));
            assert_eq!(intent,label!="valid");
            if !intent { assert!(error.to_string().contains("missing typed resource")); }
            println!("SAVER_INTENT_PRECEDENCE:{}",serde_json::json!({"ledger":ledger,"case":label,"intent":intent,"error":error.to_string()}));
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
        with tempfile.TemporaryDirectory(prefix='teaql_saver_intent_precedence_') as temporary:
            work = Path(temporary)
            env['CARGO_TARGET_DIR'] = str(work / 'target')
            observer = work / 'observer.rs'
            observer.write_text(OBSERVER)
            view = ReferenceRuntimeView(ROOT, work / 'runtime', {'src/entity_save.rs': observer}, run)
            output = run(['cargo', 'test', '--offline', '--locked', '--manifest-path', work / 'runtime/Cargo.toml',
                          '--lib', 'cpp_saver_intent_precedence_observer::observe', '-j', '2', '--', '--nocapture'])
            receipt['observations'] = [json.loads(line.split(':', 1)[1]) for line in output.splitlines()
                                       if line.startswith('SAVER_INTENT_PRECEDENCE:')]
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
