"""Expose original private save-preparation functions in a locked archive."""
import sys
import build_graph_fix_oracle as base

BRIDGE='''
#[doc(hidden)]
pub fn __teaql_cpp_extract(context: &UserContext, entity: &str, values: MutationValues)
    -> Result<GraphNode, RuntimeError> { graph_node_from_values(context, entity, values) }
#[doc(hidden)]
pub fn __teaql_cpp_preflight(context: &UserContext, node: &mut GraphNode,
    path: &ObjectLocation, root: Option<&crate::EntityRuntimeState>) -> Result<(), RuntimeError> {
    preflight_graph(context, node, path, root)
}
#[doc(hidden)]
pub fn __teaql_cpp_merge(root: &crate::EntityRuntimeState, node: &GraphNode) -> Result<(), RuntimeError> {
    merge_relation_mutations_into_root(root, node)
}
#[doc(hidden)]
pub fn __teaql_cpp_hydrate(context: &UserContext, root: &crate::EntityRuntimeState,
    node: &mut GraphNode, visited: &mut BTreeSet<crate::EntityKey>) -> Result<(), RuntimeError> {
    hydrate_ledger_relations(context, root, node, visited)
}
#[doc(hidden)]
pub fn __teaql_cpp_locations(node: &GraphNode) -> BTreeMap<crate::EntityKey, ObjectLocation> {
    ledger_object_locations(node)
}
#[doc(hidden)]
pub fn __teaql_cpp_cancelled(root: &crate::EntityRuntimeState, key: &crate::EntityKey) -> Result<(), RuntimeError> {
    reject_cancelled_new_root(root, key)
}
#[doc(hidden)]
pub fn __teaql_cpp_preserve(root: &crate::EntityRuntimeState, key: &crate::EntityKey,
    descriptor: &teaql_core::EntityDescriptor) -> bool { can_preserve_loaded_relations(root, key, descriptor) }
'''

if __name__=='__main__':
    base.SAVE_BRIDGE += BRIDGE
    # Export only the access shims, not replacement save algorithms.
    base.SAVE_BRIDGE += '\n'
    root=base.ROOT
    # Reexport access shims from the private entity_save module.
    base.EXTRA_EXPORTS = '\npub use entity_save::{__teaql_cpp_extract,__teaql_cpp_preflight,__teaql_cpp_merge,__teaql_cpp_hydrate,__teaql_cpp_locations,__teaql_cpp_cancelled,__teaql_cpp_preserve};\n'
    sys.argv += ['--oracle',str(root/'tests/parity/save_preparation_oracle.rs'),
                 '--generated-dir',str(root/'build/rust-save-preparation-original')]
    if "--target-dir" not in sys.argv:
        sys.argv += ["--target-dir", str(root/"build/rust-save-preparation-target")]
    base.main()
