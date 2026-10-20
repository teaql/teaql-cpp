
impl crate::InMemoryEntityGraphDecoderRegistry {
    #[doc(hidden)] pub fn __cpp_json_capability(&self, descriptor: &teaql_core::EntityDescriptor)
        -> Result<bool, teaql_core::EntityError> { self.json_capability(descriptor) }
    #[doc(hidden)] pub fn __cpp_decode_json_edge(&self, entity: &str, rows: Vec<(u64, teaql_core::CompactRow)>,
        root: &crate::EntityRuntimeState, graph: &mut crate::EntityGraphBuilder,
        owner: &str, id: u64, relation: &str, many: bool) -> Result<(), teaql_core::EntityError> {
        self.decode_json_edge(entity, rows, root, graph, crate::registry::JsonEdgeRequest{owner,id,relation,many})
    }
}
impl crate::EntityRuntimeState {
    #[doc(hidden)] pub fn __cpp_weak_json_view(source: &Self, view: u64) -> Self {
        Self::fresh_with_weak_graph(source).with_json_view(view)
    }
}
