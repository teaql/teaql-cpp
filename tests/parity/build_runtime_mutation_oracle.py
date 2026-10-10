"""Add access-only bridges for original runtime mutation adapters in a locked archive."""
import sys
import build_graph_fix_oracle as base

BRIDGE = r'''
#[doc(hidden)]
pub enum __CppMutation {
    Insert(teaql_core::InsertCommand), Update(teaql_core::UpdateCommand),
    Delete(teaql_core::DeleteCommand), Recover(teaql_core::RecoverCommand),
    BatchInsert(teaql_core::BatchInsertCommand), BatchUpdate(teaql_core::BatchUpdateCommand),
    Inserts(Vec<teaql_core::InsertCommand>), Updates(Vec<teaql_core::UpdateCommand>),
    Deletes(Vec<teaql_core::DeleteCommand>), Recovers(Vec<teaql_core::RecoverCommand>),
}
#[doc(hidden)]
pub async fn __cpp_mutation<E>(context: &crate::UserContext, executor: &E, layer: &str,
    mutation: __CppMutation, lineage: Vec<teaql_core::TraceNode>)
    -> Result<u64, crate::DataServiceError<E::Error>>
where E: teaql_data_service::QueryExecutor + teaql_data_service::MutationExecutor + Send + Sync {
    let service = EntityDataService::for_executor(context, "Order", executor).with_trace_context(lineage.clone());
    let runtime = RuntimeDataService::new(&service.data_service.metadata, executor);
    use __CppMutation::*;
    if layer == "base" {
        return match mutation {
            Insert(c)=>runtime.insert(&c).await, Update(c)=>runtime.update(&c).await,
            Delete(c)=>runtime.delete(&c).await, Recover(c)=>runtime.recover(&c).await,
            BatchInsert(c)=>runtime.batch_insert(&c).await, BatchUpdate(c)=>runtime.batch_update(&c).await,
            Inserts(c)=>runtime.insert_many(&c).await, Updates(c)=>runtime.update_many(&c).await,
            Deletes(c)=>runtime.delete_many(&c).await, Recovers(c)=>runtime.recover_many(&c).await,
        };
    }
    if layer == "context" {
        return match mutation {
            Insert(c)=>service.data_service.insert(&c).await, Update(c)=>service.data_service.update(&c).await,
            Delete(c)=>service.data_service.delete(&c).await, Recover(c)=>service.data_service.recover(&c).await,
            BatchInsert(c)=>service.data_service.batch_insert(&c).await, BatchUpdate(c)=>service.data_service.batch_update(&c).await,
            _=>panic!("invalid context entry"),
        };
    }
    match mutation {
        Insert(c) if layer=="prepared"=>service.execute_prepared_insert_with_comment(c,lineage).await,
        Update(c) if layer=="prepared"=>service.execute_prepared_update_with_comment(c,lineage).await,
        Insert(c)=>service.insert_internal(&c).await, Update(c)=>service.update_internal(&c).await,
        Delete(c)=>service.delete_internal(&c).await, Recover(c)=>service.recover_internal(&c).await,
        BatchInsert(c)=>service.execute_prepared_batch_insert(c).await,
        BatchUpdate(c)=>service.execute_prepared_batch_update(c).await,
        _=>panic!("invalid entity entry"),
    }
}
'''

if __name__ == '__main__':
    base.EXTRA_BRIDGES = [('data_service/mod.rs', BRIDGE)]
    base.EXTRA_EXPORTS = '\npub use data_service::{__CppMutation,__cpp_mutation};\n'
    sys.argv += ['--oracle', str(base.ROOT / 'tests/parity/runtime_mutation_oracle.rs'),
                 '--generated-dir', str(base.ROOT / 'build/rust-runtime-mutation-original')]
    if '--target-dir' not in sys.argv:
        sys.argv += ['--target-dir', str(base.ROOT / 'build/rust-runtime-mutation-target')]
    base.main()
