"""Expose the unchanged private ledger dependency sorter in the locked archive."""
import sys
import build_graph_fix_oracle as base

BRIDGE = r'''
#[doc(hidden)]
pub fn __cpp_order_ledger<E>(context:&crate::UserContext,executor:&E,
    keys:Vec<crate::EntityKey>,
    changes:&std::collections::BTreeMap<crate::EntityKey,crate::EntityValues>)
    ->Result<Vec<crate::EntityKey>,crate::RuntimeError>
where E:teaql_data_service::QueryExecutor+teaql_data_service::MutationExecutor+Send+Sync {
    EntityDataService::for_executor(context,"fixture",executor)
        .order_new_ledger_keys(keys,changes)
}
'''

if __name__ == '__main__':
    base.EXTRA_BRIDGES = [('data_service/mod.rs', BRIDGE)]
    base.EXTRA_EXPORTS = '\npub use data_service::__cpp_order_ledger;\n'
    sys.argv += ['--oracle', str(base.ROOT / 'tests/parity/ledger_plan_oracle.rs'),
                 '--generated-dir', str(base.ROOT / 'build/rust-ledger-plan-original')]
    if '--target-dir' not in sys.argv:
        sys.argv += ['--target-dir', str(base.ROOT / 'build/rust-ledger-plan-target')]
    base.main()
