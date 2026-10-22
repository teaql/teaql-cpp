#!/usr/bin/env python3
"""Observe the original saved-row dynamic snapshot precondition."""
import gzip
import json
import os
from pathlib import Path
import subprocess
import tempfile
from reference_runtime_view_5_0_6 import ReferenceRuntimeView

ROOT = Path(__file__).resolve().parents[1]
RECORD = ROOT / 'docs/parity/saved_dynamic_snapshot_original_5_0_6.json.gz'


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
        with tempfile.TemporaryDirectory(prefix='teaql_saved_dynamic_snapshot_') as temporary:
            work = Path(temporary)
            env['CARGO_TARGET_DIR'] = str(work / 'target')
            driver = (ROOT / 'tests/parity/rust_field_layout/src/bin/audited_save_5_0_6.rs').read_text()
            model = driver[driver.index('#[derive(Clone)]'):driver.index('struct ProbeChecker')]
            observer = work / 'observer.rs'
            observer.write_text('''#[cfg(test)] mod cpp_saved_dynamic_snapshot_observer {
use teaql_core::*;
use crate::{EntityRuntimeState, LedgerEntity};
''' + model + '''
#[test] fn observe() {
    for present in [false, true] {
        let mut node = crate::GraphNode::new("Order").value("id", 7_u64).value("name", "loaded");
        if present {
            let definitions = teaql_core::dynamic_fields::DynamicFieldDefinitions::new("Order", "1", []).unwrap();
            node.dynamic_fields = Some(Box::new(teaql_core::dynamic_fields::DynamicFieldValues::from_values(definitions, Default::default()).unwrap()));
        }
        let result = super::decode_saved_ledger_entity::<Model>(node, &EntityRuntimeState::default(), false);
        let error = result.err().map(|error| error.to_string());
        println!("SAVED_DYNAMIC_SNAPSHOT:{}", serde_json::json!({"present":present,"error":error}));
    }
}
}
''')
            view = ReferenceRuntimeView(ROOT, work / 'runtime', {'src/entity_save.rs': observer}, run)
            output = run(['cargo', 'test', '--offline', '--locked', '--manifest-path', work / 'runtime/Cargo.toml',
                          '--lib', 'cpp_saved_dynamic_snapshot_observer::observe', '-j', '2', '--', '--nocapture'])
            receipt['observations'] = [json.loads(line.split(':', 1)[1]) for line in output.split('\n')
                                       if line.startswith('SAVED_DYNAMIC_SNAPSHOT:')]
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
