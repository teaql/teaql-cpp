"""Expose the unchanged ledger executor via an access-only locked archive bridge."""
import sys
import build_graph_fix_oracle as base

BRIDGE = r'''
#[doc(hidden)]
pub async fn __cpp_execute_ledger<E>(context:&crate::UserContext,executor:&E,
    service:&str,root:crate::EntityRuntimeState,
    locations:&std::collections::BTreeMap<crate::EntityKey,crate::ObjectLocation>)
    ->Result<std::collections::BTreeMap<crate::EntityKey,teaql_core::Value>,crate::DataServiceError<E::Error>>
where E:teaql_data_service::QueryExecutor+teaql_data_service::MutationExecutor+Send+Sync {
    EntityDataService::for_executor(context,service,executor)
        .execute_ledger_plan_internal(root,locations).await
}
'''
if __name__ == '__main__':
    base.EXTRA_BRIDGES = [('data_service/mod.rs', BRIDGE)]
    base.EXTRA_EXPORTS = '\npub use data_service::__cpp_execute_ledger;\n'
    sys.argv += ['--oracle', str(base.ROOT / 'tests/parity/ledger_execution_oracle.rs'),
                 '--generated-dir', str(base.ROOT / 'build/rust-ledger-execution-original')]
    if '--target-dir' not in sys.argv:
        sys.argv += ['--target-dir', str(base.ROOT / 'build/rust-ledger-execution-target')]
    base.main()
