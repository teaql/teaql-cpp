// Compile verbatim locked private query methods against existing observer SPI fixtures.
use std::{env, fs, path::PathBuf};

fn between<'a>(source: &'a str, start: &str, end: &str) -> &'a str {
    let from = source.find(start).expect("locked start marker");
    let to = from + source[from..].find(end).expect("locked end marker");
    &source[from..to]
}

fn main() {
    let root = PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap())
        .join("../../../build_rust_private_save_5_0_6/original/teaql-runtime/src");
    let out = PathBuf::from(env::var("OUT_DIR").unwrap());
    let base = root.join("data_service/base.rs");
    println!("cargo:rerun-if-changed={}", base.display());
    let text = fs::read_to_string(base).unwrap();
    let methods = between(&text, "    pub(crate) fn new(", "    pub(crate) async fn insert(");
    fs::write(out.join("runtime_base_query.rs"), format!(
        "impl<'a,M,E> RuntimeDataService<'a,M,E> where M: MetadataStore,E: teaql_data_service::QueryExecutor + teaql_data_service::MutationExecutor + Sync {{\n{methods}\n}}"
    )).unwrap();
    let context = root.join("data_service/context.rs");
    println!("cargo:rerun-if-changed={}", context.display());
    let text = fs::read_to_string(context).unwrap();
    let methods = between(&text, "    async fn observe<T, F, N>(", "    pub(crate) async fn insert(");
    let comments = between(&text, "    pub(crate) fn resolve_final_comment(", "\n}\n");
    fs::write(out.join("runtime_context_query.rs"), format!(
        "impl<'a,E> ContextDataService<'a,E> where E: teaql_data_service::QueryExecutor + teaql_data_service::MutationExecutor + Send + Sync {{\n{methods}\n{comments}\n}}"
    )).unwrap();
    let relation_path = root.join("data_service/relation.rs");
    let helpers_path = root.join("data_service/helpers.rs");
    println!("cargo:rerun-if-changed={}", relation_path.display());
    println!("cargo:rerun-if-changed={}", helpers_path.display());
    let relation = fs::read_to_string(relation_path).unwrap();
    let helpers = fs::read_to_string(helpers_path).unwrap();
    let mut native = between(&relation,
        "#[derive(Debug, Clone, PartialEq, Eq, PartialOrd, Ord)]",
        "impl<'a, E> EntityDataService<'a, E>").to_owned();
    for (start, end) in [
        ("pub(super) fn relation_bucket_key(", "pub(super) fn aggregation_cache_namespace("),
        ("pub(super) fn graph_identity_key(", "pub(super) fn ensure_relation_target<"),
        ("pub(super) fn ensure_projection(", "pub(super) fn attach_empty_relation_aggregate("),
    ] {
        native.push_str(between(&helpers, start, end));
    }
    native.push_str("impl<'a,E> EntityDataService<'a,E> where E: teaql_data_service::QueryExecutor + teaql_data_service::MutationExecutor + Send + Sync {\n");
    for (start, end) in [
        ("    fn query_for_plan(", "    async fn fetch_relation_rows("),
        ("    async fn fetch_native_relation_rows(", "    async fn relation_facets("),
        ("    fn base_relation_query(", "    fn ensure_stable_top_n_order("),
        ("    fn ensure_stable_top_n_order(", "    fn query_for_compact_plan("),
        ("    fn query_for_compact_plan(", "    fn attach_relation_rows("),
    ] {
        native.push_str(between(&relation, start, end));
    }
    native.push_str("}\n");
    fs::write(out.join("relation_execution.rs"), native).unwrap();

    let mut enhancement = String::new();
    for (start, end) in [
        ("pub(super) fn default_aggregate_value(", "pub(super) fn aggregation_cache_namespace("),
        ("pub(super) fn graph_identity_key(", "pub(super) fn ensure_relation_target<"),
        ("pub(super) fn ensure_projection(", "#[cfg(test)]\nmod aggregation_cache_key_tests"),
        ("pub(super) fn attach_relation_aggregate_rows(", "pub(super) fn ensure_initial_version("),
    ] {
        enhancement.push_str(between(&helpers, start, end));
    }
    enhancement.push_str("impl<'a,E> EntityDataService<'a,E> where E: teaql_data_service::QueryExecutor + teaql_data_service::MutationExecutor + Send + Sync {\n");
    for (start, end) in [
        ("    pub(crate) fn relation_aggregate_key_rows(", "    fn build_relation_plans("),
        ("    fn build_relation_plans_from_loads(", "    fn enhance_plan<'b>("),
        ("    fn relation_child_repo(", "    fn query_for_plan("),
    ] {
        enhancement.push_str(between(&relation, start, end));
    }
    enhancement.push_str("}\n");
    fs::write(out.join("relation_enhancement.rs"), enhancement).unwrap();

}
