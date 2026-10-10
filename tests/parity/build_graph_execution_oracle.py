"""Expose original graph execution/save/read helpers without replacing their bodies."""
import sys
import build_graph_fix_oracle as base

BRIDGE = r'''
#[doc(hidden)]
pub async fn __cpp_execute_graph<E>(context:&crate::UserContext,executor:&E,
    service:&str,plan:crate::GraphMutationPlan)->Result<crate::GraphNode,crate::DataServiceError<E::Error>>
where E:teaql_data_service::QueryExecutor+teaql_data_service::MutationExecutor+Send+Sync {
    EntityDataService::for_executor(context,service,executor).execute_graph_plan_internal(plan).await
}
#[doc(hidden)]
pub async fn __cpp_save_graph<E>(context:&crate::UserContext,executor:&E,
    service:&str,node:crate::GraphNode)->Result<crate::GraphNode,crate::DataServiceError<E::Error>>
where E:teaql_data_service::QueryExecutor+teaql_data_service::MutationExecutor+Send+Sync {
    EntityDataService::for_executor(context,service,executor).save_graph_internal(node).await
}
#[doc(hidden)]
pub async fn __cpp_current_row<E>(context:&crate::UserContext,executor:&E,
    service:&str,entity:&str,property:&str,id:&teaql_core::Value,trace:Vec<teaql_core::TraceNode>)
    ->Result<Option<teaql_core::CompactRow>,crate::DataServiceError<E::Error>>
where E:teaql_data_service::QueryExecutor+teaql_data_service::MutationExecutor+Send+Sync {
    EntityDataService::for_executor(context,service,executor)
        .fetch_graph_current_row_internal(entity,property,id,trace).await
}

#[doc(hidden)]
pub async fn __cpp_save_entity_adapter<T:teaql_core::Entity,E>(
    context:&crate::UserContext,executor:&E,service:&str,entity:T,
    status:crate::EntityStatus,comment:String,mode:&str)
    ->Result<crate::GraphNode,crate::DataServiceError<E::Error>>
where E:teaql_data_service::QueryExecutor+teaql_data_service::MutationExecutor+Send+Sync {
    let repo=EntityDataService::for_executor(context,service,executor);
    match mode {
        "adapter_graph"=>repo.save_entity_graph_internal(entity).await,
        "adapter_status"=>repo.save_entity_internal(entity,status).await,
        "adapter_status_comment"=>repo.save_entity_with_comment_internal(entity,status,comment).await,
        "adapter_comment"=>repo.save_entity_graph_with_comment_internal(entity,comment).await,
        "adapter_create"=>repo.create_entity_graph_with_comment_internal(entity,comment).await,
        _=>panic!("unknown adapter mode"),
    }
}
#[doc(hidden)]
pub async fn __cpp_save_core_graph<E>(context:&crate::UserContext,executor:&E,
    service:&str,graph:teaql_core::EntityGraph)
    ->Result<crate::GraphNode,crate::DataServiceError<E::Error>>
where E:teaql_data_service::QueryExecutor+teaql_data_service::MutationExecutor+Send+Sync {
    EntityDataService::for_executor(context,service,executor).save_entity_graph_from_internal(graph).await
}
'''
NODE_BRIDGE = r'''
#[doc(hidden)]
pub async fn __cpp_reference_node<E>(context:&crate::UserContext,executor:&E,
    service:&str,node:crate::GraphNode,trace:Vec<teaql_core::TraceNode>)
    ->Result<crate::GraphNode,crate::DataServiceError<E::Error>>
where E:teaql_data_service::QueryExecutor+teaql_data_service::MutationExecutor+Send+Sync {
    EntityDataService::for_executor(context,service,executor).validate_reference_node(node,trace).await
}
#[doc(hidden)]
pub async fn __cpp_remove_node<E>(context:&crate::UserContext,executor:&E,
    service:&str,node:&crate::GraphNode,trace:Vec<teaql_core::TraceNode>)
    ->Result<(),crate::DataServiceError<E::Error>>
where E:teaql_data_service::QueryExecutor+teaql_data_service::MutationExecutor+Send+Sync {
    EntityDataService::for_executor(context,service,executor).validate_remove_node(node,trace).await
}
#[doc(hidden)]
pub fn __cpp_update_node<E>(context:&crate::UserContext,executor:&E,
    service:&str,node:&mut crate::GraphNode,id:&teaql_core::Value)
    ->Result<teaql_core::UpdateCommand,crate::DataServiceError<E::Error>>
where E:teaql_data_service::QueryExecutor+teaql_data_service::MutationExecutor+Send+Sync {
    let descriptor=context.require_entity(&node.entity).map_err(crate::DataServiceError::Runtime)?;
    let property=descriptor.id_property().unwrap_or_else(|| &descriptor.properties[0]);
    EntityDataService::for_executor(context,service,executor).graph_update_command(node,descriptor,property,id)
}
#[doc(hidden)]
pub async fn __cpp_delete_node<E>(context:&crate::UserContext,executor:&E,
    service:&str,node:&crate::GraphNode,parent:Option<&crate::ScopedCommentNode<'_>>)
    ->Result<u64,crate::DataServiceError<E::Error>>
where E:teaql_data_service::QueryExecutor+teaql_data_service::MutationExecutor+Send+Sync {
    EntityDataService::for_executor(context,service,executor).delete_graph_node(node,parent).await
}

#[doc(hidden)]
pub async fn __cpp_recursive_graph<E>(context:&crate::UserContext,executor:&E,
    service:&str,node:crate::GraphNode,parent:Option<&crate::ScopedCommentNode<'_>>,insert:bool)
    ->Result<crate::GraphNode,crate::DataServiceError<E::Error>>
where E:teaql_data_service::QueryExecutor+teaql_data_service::MutationExecutor+Send+Sync {
    let repo=EntityDataService::for_executor(context,service,executor);
    if insert {repo.insert_graph_node_scoped(node,parent).await}
    else {repo.upsert_graph_node_scoped(node,parent).await}
}
'''
NODE_EXPORTS = '__cpp_reference_node,__cpp_remove_node,__cpp_update_node,__cpp_delete_node,__cpp_recursive_graph'
if __name__ == '__main__':
    base.EXTRA_BRIDGES = [('data_service/mod.rs', BRIDGE + '\npub use graph::{'+NODE_EXPORTS+'};\n'), ('data_service/graph.rs', NODE_BRIDGE)]
    base.EXTRA_EXPORTS = '\npub use data_service::{__cpp_execute_graph,__cpp_save_graph,__cpp_current_row,__cpp_save_entity_adapter,__cpp_save_core_graph,'+NODE_EXPORTS+'};\n'
    sys.argv += ['--oracle', str(base.ROOT / 'tests/parity/graph_execution_oracle.rs'),
                 '--generated-dir', str(base.ROOT / 'build/rust-graph-execution-original')]
    if '--target-dir' not in sys.argv:
        sys.argv += ['--target-dir', str(base.ROOT / 'build/rust-graph-execution-target')]
    base.main()
