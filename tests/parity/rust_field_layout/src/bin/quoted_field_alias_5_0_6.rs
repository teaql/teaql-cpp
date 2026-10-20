//! Direct generated FK alias witness; keep source physical spelling unchanged.
use cpp_dynamic_json_core::Line;
use serde_json::json;
use std::sync::Arc;
use teaql_core::{CompactRow, CompactRowLayout, Entity, TeaqlEntity, Value};

fn main() {
    let descriptor = Line::entity_descriptor();
    let column = descriptor.properties.iter().find(|p| p.name == "order_id").unwrap().column_name.clone();
    for alias in ["order_id".to_owned(), "order".to_owned(), column, "order_typo".to_owned()] {
        for kind in ["scalar", "object", "null"] {
            let payload = match kind {
                "scalar" => Value::U64(42),
                "object" => Value::Object(std::collections::BTreeMap::from([("id".into(), Value::U64(42))])),
                _ => Value::Null,
            };
            let row = CompactRow::with_layout(
                CompactRowLayout::new(Arc::from(["id".into(), "version".into(), alias.clone()])),
                vec![Value::U64(1), Value::I64(1), payload],
            );
            let output = match Line::from_compact_row(row) {
                Ok(value) => json!({"ok":true,"id":value.order_id(),"loaded":value.is_field_loaded("order_id")}),
                Err(error) => json!({"ok":false,"error":error.to_string()}),
            };
            println!("{}", json!({"input":{"name":alias,"kind":kind},"output":output}));
        }
    }
}
