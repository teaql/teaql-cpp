"""Expose original typed/erased transactional savers without replacing bodies."""
import sys
import build_graph_fix_oracle as base

BRIDGE = r'''
#[doc(hidden)]
pub async fn __cpp_registered_save<E>(context:&UserContext,node:GraphNode,
    root:Option<crate::EntityRuntimeState>)->Result<GraphNode,RuntimeError>
where E:teaql_data_service::QueryExecutor+teaql_data_service::MutationExecutor+
    teaql_data_service::TransactionExecutor+Send+Sync+'static,
    for<'tx> <E as teaql_data_service::TransactionExecutor>::Tx<'tx>:Send+Sync {
    let registered=context.require_resource::<Arc<dyn DynGraphSaver>>()
        .map_err(|e|RuntimeError::Graph(e.to_string()))?;
    let saver:&dyn DynGraphSaver=registered.as_ref();
    if let Some(root)=root {saver.save_ledger_dyn(context,node,root).await}
    else {saver.save_graph_dyn(context,node).await}
}
'''
if __name__ == '__main__':
    base.EXTRA_BRIDGES = [('entity_save.rs', BRIDGE)]
    base.EXTRA_EXPORTS = '\npub use entity_save::__cpp_registered_save;\n'
    sys.argv += ['--oracle', str(base.ROOT / 'tests/parity/graph_saver_oracle.rs'),
                 '--generated-dir', str(base.ROOT / 'build/rust-graph-saver-original')]
    if '--target-dir' not in sys.argv:
        sys.argv += ['--target-dir', str(base.ROOT / 'build/rust-graph-saver-target')]
    base.main()
