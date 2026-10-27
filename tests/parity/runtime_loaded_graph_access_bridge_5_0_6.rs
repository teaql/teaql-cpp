#[cfg(test)]
impl<'a, E> EntityDataService<'a, E>
where
    E: teaql_data_service::QueryExecutor + teaql_data_service::MutationExecutor + Send + Sync,
{
    pub(crate) fn __cpp_reconstruct_loaded_graph(
        &self,
        entity: &str,
        rows: &mut [CompactRow],
    ) -> Result<crate::EntityRuntimeState, teaql_core::EntityError> {
        self.attach_flat_relation_graph(entity, rows)
    }
}
