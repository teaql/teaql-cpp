
impl crate::EntityGraphBuilder {
    #[doc(hidden)] pub fn __cpp_for_json_read() -> Self { Self::for_json_read() }
    #[doc(hidden)] pub fn __cpp_json_list<T: std::any::Any + Send + Sync>(
        &mut self, view: u64, owner: &str, id: u64, relation: &str, list: teaql_core::SmartList<T>) {
        self.install_json_relation_list(view, owner, id, relation, list)
    }
    #[doc(hidden)] pub fn __cpp_json_option<T: teaql_core::IdentifiableEntity + std::any::Any + Send + Sync>(
        &mut self, view: u64, owner: &str, id: u64, relation: &str, value: Option<T>) {
        self.install_typed_relation_option(view, owner, id, relation, value)
    }
}
impl crate::EntityRuntimeState {
    #[doc(hidden)] pub fn __cpp_json_view(self, view: u64) -> Self { self.with_json_view(view) }
    #[doc(hidden)] pub fn __cpp_json_view_id(&self) -> u64 { self.json_view_id() }
}
