use std::{env, fs, path::PathBuf};

fn segment(text: &str, start: &str, end: &str) -> String {
    let from = text.find(start).expect("original source start marker");
    let to = from + text[from..].find(end).expect("original source end marker");
    text[from..to].to_owned()
}

fn main() {
    let root = PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap())
        .join("../../../.reference/teaql-rs/teaql-runtime/src/data_service");
    let resolved_path = root.join("resolved.rs");
    let helper_path = root.join("helpers.rs");
    println!("cargo:rerun-if-changed={}", resolved_path.display());
    println!("cargo:rerun-if-changed={}", helper_path.display());
    let resolved = fs::read_to_string(resolved_path).unwrap();
    let helpers = fs::read_to_string(helper_path).unwrap();
    let mut body = segment(&resolved, "fn decode_compact_rows<T: Entity>(", "impl<'a, E> EntityDataService<'a, E>");
    body += "impl<'a, E> SourceRepo<'a, E> where E: teaql_data_service::QueryExecutor + teaql_data_service::MutationExecutor + Send + Sync {\n";
    body += &segment(&resolved, "    fn ensure_entity_identity_projection(", "    pub fn prepare_insert_command(");
    body += &segment(&resolved, "    pub(crate) async fn fetch_entities_with_relation_aggregates_internal<T>(", "    pub(crate) async fn fetch_enhanced_entities_with_relation_aggregates_internal<T>(");
    body += "}\n";
    body += &segment(&helpers, "pub(super) fn ensure_projection(", "pub(super) fn attach_empty_relation_aggregate(");
    fs::write(PathBuf::from(env::var("OUT_DIR").unwrap()).join("typed_aggregate_scope.rs"), body).unwrap();
}
