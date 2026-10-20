//! Observe all container metadata through the complete unchanged source parser.
use serde_json::{json, Value};
use std::io::{self, BufRead};
#[path="../../../.reference/teaql-rs/teaql-macros/src/attr.rs"]
mod original;
fn main() {
    std::panic::set_hook(Box::new(|_| {}));
    for line in io::stdin().lock().lines() {
        let input: Value = serde_json::from_str(&line.unwrap()).unwrap();
        let text = match input.as_str() {
            Some(value) => format!("#[teaql(audit_value_max_len={value})] struct Probe {{}}"),
            None => "struct Probe {}".into(),
        };
        let output = match syn::parse_str::<syn::DeriveInput>(&text) {
            Ok(input) => match std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
                original::parse_container_attrs(&input.attrs, "Probe")
            })) {
                Ok(attrs) => json!({"parsed":true,
                    "entity":attrs.entity_name,"table":attrs.table_name,
                    "service":attrs.data_service,"indexed":attrs.indexed_layout,
                    "limit":attrs.audit_value_max_len.map(|n|n.to_string()),
                    "masks":attrs.audit_mask_fields,"declared":attrs.audit_mask_fields_declared,
                    "relations":attrs.reverse_relations.iter().map(|r| json!({
                        "name":r.name,"target":r.target,
                        "local":r.local_key.as_deref().unwrap_or("id"),
                        "foreign":r.foreign_key.as_deref().unwrap_or("id"),
                        "many":r.many,"typed":r.json_type.is_some()})).collect::<Vec<_>>() }),
                Err(error) => json!({"parsed":false,"stage":"attribute",
                    "error":error.downcast_ref::<String>().map(|s|s.as_str())
                        .or_else(||error.downcast_ref::<&str>().copied()).unwrap_or("attribute panic")}),
            },
            Err(error) => json!({"parsed":false,"stage":"lexical","error":error.to_string()}),
        };
        println!("{output}");
    }
}
