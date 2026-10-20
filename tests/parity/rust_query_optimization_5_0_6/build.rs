// Development-only slices compile complete unchanged original private methods.
use std::{env, fs, path::PathBuf};

fn slice(text: &str, start: &str, end: &str) -> String {
    let from = text.find(start).expect("locked start marker");
    let to = from + text[from..].find(end).expect("locked end marker");
    text[from..to].to_owned()
}

fn main() {
    let root = PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap())
        .join("../../../.reference/teaql-rs/teaql-runtime/src");
    let out = PathBuf::from(env::var("OUT_DIR").unwrap());
    for path in ["data_service/helpers.rs", "data_service/resolved.rs",
                 "context/pagination.rs", "data_service/cache.rs"] {
        println!("cargo:rerun-if-changed={}", root.join(path).display());
    }
    let helpers = fs::read_to_string(root.join("data_service/helpers.rs")).unwrap();
    let resolved = fs::read_to_string(root.join("data_service/resolved.rs")).unwrap();
    let mut keys = slice(&helpers, "pub(super) fn aggregation_cache_namespace(",
                         "pub(super) fn invalidate_aggregation_cache_namespace(");
    keys += &slice(&helpers, "pub(super) fn aggregation_cache_key(",
                   "pub(super) fn ensure_projection(");
    keys += "impl KeyRepo {\n";
    keys += &slice(&resolved, "    fn continuous_page_query_key(",
                   "    async fn prepare_id_set_page(");
    keys += "}\n";
    fs::write(out.join("query_keys.rs"), keys).unwrap();
    let pagination = fs::read_to_string(root.join("context/pagination.rs")).unwrap();
    let lock = pagination.find("pub(super) fn id_set_build_lock(").unwrap();
    fs::write(out.join("query_stores.rs"), &pagination[..lock]).unwrap();
    let mut locks = String::from("use std::{collections::HashMap,sync::{Arc,Mutex,OnceLock}};\n");
    locks += &slice(&pagination, "pub(super) fn id_set_build_lock(", "\nimpl UserContext {");
    fs::write(out.join("id_set_build_lock.rs"), locks).unwrap();
    let mut observations = String::from("impl UserContext {\n");
    for (start, end) in [
        ("    pub fn continuous_page_plan(", "    pub(crate) fn continuous_page_cursor_store("),
        ("    pub fn id_set_plan(", "    pub(crate) fn id_set_store(")
    ] {
        observations += &slice(&pagination, start, end);
    }
    observations += "}\n";
    fs::write(out.join("pagination_observations.rs"), observations).unwrap();
    fs::write(out.join("aggregation_cache.rs"),
              fs::read_to_string(root.join("data_service/cache.rs")).unwrap()).unwrap();
}
