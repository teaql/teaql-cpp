use serde_json::{json, Value as Json};
use std::{
    collections::HashMap,
    io::{self, BufRead},
};
use teaql_core::dynamic_fields::{DynamicFieldDefinitions, DynamicFieldValues};
use teaql_core::{DataType, Value};
use teaql_runtime::GraphNode;
fn main() {
    for line in io::stdin().lock().lines() {
        let input: Json = serde_json::from_str(&line.unwrap()).unwrap();
        let mut node = GraphNode::new("School");
        if input["mode"] != "none" {
            let mut definition =
                DynamicFieldDefinitions::new("School", "v1", [("title".into(), DataType::Text)])
                    .unwrap();
            if input["bound"] == true {
                definition = definition.with_storage_binding("profile", 37);
            }
            let mut values = HashMap::new();
            if input["mode"] != "empty" {
                values.insert(
                    "title".into(),
                    if input["mode"] == "null" {
                        Value::Null
                    } else {
                        Value::Text(input["value"].as_str().unwrap().into())
                    },
                );
            }
            node.dynamic_fields = Some(Box::new(
                DynamicFieldValues::from_values(definition, values).unwrap(),
            ));
        }
        let copied = node.clone();
        let before = json!({"debug":format!("{node:?}"), "pretty":format!("{node:#?}"), "copy_equal":copied==node});
        if let Some(fields) = &mut node.dynamic_fields {
            fields
                .assign(
                    "title",
                    Value::Text(input["after"].as_str().unwrap().into()),
                )
                .unwrap();
        }
        println!(
            "{}",
            json!({"before":before,"equal_after":copied==node,"copy_debug":format!("{copied:?}")})
        );
    }
}
