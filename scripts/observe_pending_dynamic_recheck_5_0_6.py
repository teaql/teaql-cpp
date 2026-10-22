#!/usr/bin/env python3
"""Observe pending dynamic Checker changes to later entity status."""
import gzip
import json
import os
from pathlib import Path
import subprocess
import tempfile
from reference_runtime_view_5_0_6 import ReferenceRuntimeView

ROOT = Path(__file__).resolve().parents[1]
RECORD = ROOT / 'docs/parity/pending_dynamic_recheck_original_5_0_6.json.gz'
OBSERVER = r'''
#[cfg(test)] mod cpp_pending_dynamic_recheck_observer {
use std::sync::{Arc,Mutex};
use teaql_core::{Value,DataType,EntityDescriptor,PropertyDescriptor};
use teaql_core::dynamic_fields::{DynamicFieldDefinitions,DynamicFieldValues};
use crate::{EntityRuntimeState,EntityKey,UserContext,InMemoryMetadataStore,InMemoryCheckerRegistry,Checker,CheckResults,CheckObjectStatus,ObjectLocation,EntityValues,GraphNode};
struct Probe { root: EntityRuntimeState, action: &'static str, seen: Arc<Mutex<Vec<String>>> }
impl Checker for Probe {
    fn entity(&self)->&str { "Order" }
    fn check_and_fix(&self,_:&UserContext,values:&mut EntityValues,_:&ObjectLocation,_:&mut CheckResults) {
        let id=values.get("id").unwrap().try_u64().unwrap();
        self.seen.lock().unwrap().push(format!("{id}:{}",CheckObjectStatus::from_values(values).as_str()));
        if id==1 {
            let key=EntityKey::new("Order",2_u64);
            if self.action=="delete" { self.root.mark_as_delete(key); }
            else if self.action=="new" { self.root.mark_as_new(key); }
        }
    }
}
#[test] fn observe() {
    for action in ["none","delete","new"] {
        let mut root=EntityRuntimeState::default();
        let definitions=DynamicFieldDefinitions::new("Order","1",[("score".into(),DataType::I64)]).unwrap();
        root.install_loaded_dynamic_fields(DynamicFieldValues::from_values(definitions,Default::default()).unwrap());
        for id in [1_u64,2] { root.update_dynamic_field(EntityKey::new("Order",id),"score",Value::I64(5)).unwrap(); }
        let seen=Arc::new(Mutex::new(Vec::new()));
        let mut context=UserContext::new().with_metadata(InMemoryMetadataStore::new().with_entity(
            EntityDescriptor::new("Order").property(PropertyDescriptor::new("id",DataType::U64).id())));
        context.set_checker_registry(InMemoryCheckerRegistry::new().with_checker(Probe{root:root.clone(),action,seen:seen.clone()}));
        super::preflight_pending_dynamic_entities(&context,&root,&GraphNode::new("Parent")).unwrap();
        let actual=seen.lock().unwrap().clone();
        let expected=match action { "delete"=>vec!["1:update"],"new"=>vec!["1:update","2:create"],_=>vec!["1:update","2:update"] };
        assert_eq!(actual,expected);
        println!("PENDING_DYNAMIC_RECHECK:{}",serde_json::json!({"action":action,"seen":actual}));
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
        with tempfile.TemporaryDirectory(prefix='teaql_pending_dynamic_recheck_') as temporary:
            work = Path(temporary)
            env['CARGO_TARGET_DIR'] = str(work / 'target')
            observer = work / 'observer.rs'
            observer.write_text(OBSERVER)
            view = ReferenceRuntimeView(ROOT, work / 'runtime', {'src/entity_save.rs': observer}, run)
            output = run(['cargo', 'test', '--offline', '--locked', '--manifest-path', work / 'runtime/Cargo.toml',
                          '--lib', 'cpp_pending_dynamic_recheck_observer::observe', '-j', '2', '--', '--nocapture'])
            receipt['observations'] = [json.loads(line.split(':', 1)[1]) for line in output.splitlines()
                                       if line.startswith('PENDING_DYNAMIC_RECHECK:')]
            assert len(receipt['observations']) == 3
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
