//! Actual original API borrow boundaries; no runtime algorithm is replaced.
use cpp_dynamic_json_core::Q;
use std::sync::Arc;
struct Registry;
impl teaql_runtime::EntityRegistry for Registry {
    fn contains(&self, _: &str) -> bool { true }
}
fn main() {
    let mut user_context = teaql_runtime::UserContext::new();
    let query = Q::orders().comment("Check original resource mutation borrowing")
        .purpose("Distinguish C++ lifetime adaptations");
    let future = query.execute_for_list(&user_context);
    #[cfg(teaql_resource_negative)]
    user_context.insert_resource(7_u64);
    #[cfg(teaql_registry_negative)]
    user_context.set_entity_registry(Registry);
    #[cfg(teaql_store_negative)]
    user_context.set_id_set_store(Arc::new(teaql_runtime::InMemoryIdSetStore::default()));
    drop(future);
    // Reconfiguration is valid once the immutable execution borrow has ended.
    user_context.insert_resource(7_u64);
    user_context.set_entity_registry(Registry);
    user_context.set_id_set_store(Arc::new(teaql_runtime::InMemoryIdSetStore::default()));
}
