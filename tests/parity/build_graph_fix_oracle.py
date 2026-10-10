"""Build untouched locked method bodies with private scope/validation access.

Only access shims and a delegated DynGraphSaver fixture are appended to an
ignored git archive. The reference checkout and generator remain unchanged.
"""
import argparse
import io
import json
import os
from pathlib import Path
import re
import subprocess
import tarfile

ROOT = Path(__file__).resolve().parents[2]
EXTRA_EXPORTS = ""
EXTRA_BRIDGES = []
CONTEXT_BRIDGE = '''
impl UserContext {
    #[doc(hidden)]
    pub fn __teaql_cpp_required_create(&self, entity: &str, values: &crate::EntityValues,
        location: &crate::ObjectLocation) -> Result<(), crate::RuntimeError> {
        self.validate_required_create_payload(entity, values, location)
    }
    #[doc(hidden)]
    pub fn __teaql_cpp_replace_fix_evidence(&self, evidence: Vec<crate::FixEvidence>) {
        self.replace_last_fix_evidence(evidence);
    }
}
'''
SAVE_BRIDGE = '''
#[doc(hidden)]
pub fn __teaql_cpp_fix_scope<F: Future>(time: teaql_core::time::Timestamp,
    evidence: Arc<std::sync::Mutex<Vec<crate::FixEvidence>>>, future: F)
    -> impl Future<Output = F::Output> {
    GRAPH_FIX_TIME.scope(time, GRAPH_FIX_EVIDENCE.scope(evidence, future))
}
#[doc(hidden)]
pub type __TeaqlCppSaveHook = for<'a> fn(&'a UserContext, GraphNode)
    -> Pin<Box<dyn Future<Output = Result<GraphNode, RuntimeError>> + Send + 'a>>;
#[doc(hidden)]
pub fn __teaql_cpp_install_save_hook(context: &mut UserContext, hook: __TeaqlCppSaveHook) {
    struct Hook(__TeaqlCppSaveHook);
    impl DynGraphSaver for Hook {
        fn save_graph_dyn<'a>(&'a self, context: &'a UserContext, node: GraphNode)
            -> Pin<Box<dyn Future<Output = Result<GraphNode, RuntimeError>> + Send + 'a>> {
            (self.0)(context, node)
        }
        fn save_ledger_dyn<'a>(&'a self, context: &'a UserContext, node: GraphNode,
            _root: crate::EntityRuntimeState)
            -> Pin<Box<dyn Future<Output = Result<GraphNode, RuntimeError>> + Send + 'a>> {
            (self.0)(context, node)
        }
    }
    context.insert_resource::<Arc<dyn DynGraphSaver>>(Arc::new(Hook(hook)));
}
'''


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--target-dir', type=Path, default=ROOT/'build/rust-graph-fix-target')
    parser.add_argument('--oracle', type=Path, default=ROOT/'tests/parity/graph_fix_oracle.rs')
    parser.add_argument('--generated-dir', type=Path, default=ROOT/'build/rust-graph-fix-original')
    args = parser.parse_args()
    subprocess.run(['python3',str(ROOT/'scripts/verify-upstream.py')],check=True)
    lock=json.loads((ROOT/'upstream.lock.json').read_text())
    generated=args.generated_dir; source=generated/'original'
    archive=subprocess.check_output(['git','-C',str(ROOT/'.reference/teaql-rs'),'archive',lock['commit']])
    with tarfile.open(fileobj=io.BytesIO(archive)) as tar:
        for member in tar:
            destination=source/member.name
            if not destination.resolve().is_relative_to(source.resolve()):
                raise RuntimeError('Archive path escapes generated source tree')
            if member.isdir(): destination.mkdir(parents=True,exist_ok=True)
            elif member.isfile():
                destination.parent.mkdir(parents=True,exist_ok=True)
                destination.write_bytes(tar.extractfile(member).read()); os.utime(destination,(member.mtime,member.mtime))
    for name,bridge in [('context.rs',CONTEXT_BRIDGE),('entity_save.rs',SAVE_BRIDGE)] + EXTRA_BRIDGES:
        target=source/'teaql-runtime/src'/name; target.write_text(target.read_text()+bridge)
    exports=source/'teaql-runtime/src/lib.rs'
    exports.write_text(exports.read_text()+'\npub use entity_save::{__teaql_cpp_fix_scope, __teaql_cpp_install_save_hook};\n'+EXTRA_EXPORTS)
    project=generated/'probe'; (project/'src').mkdir(parents=True,exist_ok=True)
    manifest=(ROOT/'tests/parity/rust-values/Cargo.toml').read_text()
    manifest=re.sub(r'path = "\.\./\.\./\.\./\.reference/teaql-rs/([^"]+)"',
                    lambda match:'path = '+json.dumps(str(source/match[1])),manifest)
    (project/'Cargo.toml').write_text(manifest)
    (project/'Cargo.lock').write_bytes((ROOT/'tests/parity/rust-values/Cargo.lock').read_bytes())
    (project/'src/main.rs').write_bytes(args.oracle.read_bytes())
    (project/'src/fixture_value.rs').write_bytes((ROOT/'tests/parity/rust-values/src/fixture_value.rs').read_bytes())
    env=dict(os.environ,CARGO_TARGET_DIR=str(args.target_dir.resolve()))
    subprocess.run(['cargo','build','--locked','--offline','--manifest-path',str(project/'Cargo.toml')],env=env,check=True)
    (generated/'baseline.json').write_text(json.dumps(dict(commit=lock['commit'],context_bridge=CONTEXT_BRIDGE,save_bridge=SAVE_BRIDGE,extra_bridges=EXTRA_BRIDGES,extra_exports=EXTRA_EXPORTS),indent=2)+'\n')
    print(args.target_dir.resolve()/'debug/teaql-cpp-value-oracle')


if __name__ == '__main__': main()
