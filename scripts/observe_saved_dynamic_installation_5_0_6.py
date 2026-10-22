#!/usr/bin/env python3
"""Observe saved-row dynamic overlay and installer errors in the locked original."""
import gzip
import json
import os
from pathlib import Path
import subprocess
import tempfile
from reference_runtime_view_5_0_6 import ReferenceRuntimeView

ROOT = Path(__file__).resolve().parents[1]
RECORD = ROOT / 'docs/parity/saved_dynamic_installation_original_5_0_6.json.gz'
OBSERVER = r'''
#[cfg(test)] mod cpp_saved_dynamic_installation_observer {
use std::sync::{Arc, atomic::{AtomicUsize, Ordering}};
use teaql_core::*;
use teaql_core::dynamic_fields::{DynamicFieldDefinitions, DynamicFieldValues};
use crate::{EntityRuntimeState, RuntimeError};
static INSTALLS: AtomicUsize = AtomicUsize::new(0);
struct Model { values: MutationValues, snapshot: Arc<LoadedSnapshot>, installed: bool }
impl TeaqlEntity for Model {
    const ENTITY_NAME: &'static str = "Order";
    fn entity_descriptor() -> EntityDescriptor { EntityDescriptor::new("Order") }
}
impl Entity for Model {
    fn from_compact_row(row: CompactRow) -> Result<Self, EntityError> {
        let layout = FieldLayout::from_generated("Order", "saved-dynamic-observer-v1",
            &[("id", 0), ("version", 1), ("name", 2)],
            &[("id", "id", "id"), ("version", "version", "version"), ("name", "name", "name")],
            &[], &["id", "version", "name"]).unwrap();
        Ok(Self { values: row.into_map().into(),
            snapshot: LoadedSnapshot::fully_loaded(layout).into_shared(), installed: false })
    }
    fn into_values(self) -> MutationValues { self.values }
    fn loaded_state_snapshot(&self) -> Option<Arc<LoadedSnapshot>> { Some(self.snapshot.clone()) }
    fn install_loaded_dynamic_fields(&mut self, _: DynamicFieldValues,
                                     state: Arc<LoadedSnapshot>) -> Result<(), EntityError> {
        INSTALLS.fetch_add(1, Ordering::SeqCst);
        if self.values.get("name").and_then(Value::try_text) == Some("reject") {
            return Err(EntityError::new("Order", "installer rejected saved dynamic fields"));
        }
        assert!(Arc::ptr_eq(&self.snapshot, &state), "empty overlay retains snapshot identity");
        self.snapshot = state;
        self.installed = true;
        Ok(())
    }
}
#[test] fn observe() {
    for preserve in [false, true] {
        for (label, owner, reject) in [("owner_mismatch", "Other", false),
                                     ("installer_error", "Order", true),
                                     ("success", "Order", false)] {
            INSTALLS.store(0, Ordering::SeqCst);
            let mut node = crate::GraphNode::new("Order").value("id", 7_u64)
                .value("version", 1_i64).value("name", if reject { "reject" } else { "accept" });
            let definitions = DynamicFieldDefinitions::new(owner, "1", []).unwrap();
            node.dynamic_fields = Some(Box::new(DynamicFieldValues::from_values(
                definitions, Default::default()).unwrap()));
            let result = super::decode_saved_ledger_entity::<Model>(node, &EntityRuntimeState::default(), preserve);
            let error = result.as_ref().err().map(|error| error.to_string());
            let graph_error = matches!(&result, Err(RuntimeError::Graph(_)));
            let calls = INSTALLS.load(Ordering::SeqCst);
            let installed = result.as_ref().ok().map(|entity| entity.installed);
            let loaded = result.as_ref().ok().map(|entity|
                ["id", "version", "name"].iter().all(|field| entity.snapshot.is_loaded(field)));
            match label {
                "owner_mismatch" => {
                    assert_eq!(calls, 0);
                    assert_eq!(error.as_deref(), Some("graph write error: dynamic field owner disagrees with fixed entity layout"));
                    assert!(graph_error);
                }
                "installer_error" => {
                    assert_eq!(calls, 1);
                    assert_eq!(error.as_deref(), Some("graph write error: Order: installer rejected saved dynamic fields"));
                    assert!(graph_error);
                }
                _ => { assert_eq!(calls, 1); assert!(error.is_none()); assert_eq!(installed, Some(true)); assert_eq!(loaded, Some(true)); }
            }
            println!("SAVED_DYNAMIC_INSTALLATION:{}", serde_json::json!({
                "case": label, "preserve": preserve, "error": error, "graph_error": graph_error,
                "installer_calls": calls, "installed": installed, "fixed_fields_loaded": loaded}));
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
        with tempfile.TemporaryDirectory(prefix='teaql_saved_dynamic_installation_') as temporary:
            work = Path(temporary)
            env['CARGO_TARGET_DIR'] = str(work / 'target')
            observer = work / 'observer.rs'
            observer.write_text(OBSERVER)
            view = ReferenceRuntimeView(ROOT, work / 'runtime', {'src/entity_save.rs': observer}, run)
            output = run(['cargo', 'test', '--offline', '--locked', '--manifest-path', work / 'runtime/Cargo.toml',
                          '--lib', 'cpp_saved_dynamic_installation_observer::observe', '-j', '2', '--', '--nocapture'])
            receipt['observations'] = [json.loads(line.split(':', 1)[1]) for line in output.splitlines()
                                       if line.startswith('SAVED_DYNAMIC_INSTALLATION:')]
            assert len(receipt['observations']) == 6
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
