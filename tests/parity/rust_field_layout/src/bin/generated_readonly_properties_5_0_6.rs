//! Invoke the unchanged generated School carrier's readonly property contract.
use school_management_service_core::School;
use serde_json::{Value as Json, json};
use std::{collections::BTreeMap, io::{self, BufRead}};
use teaql_core::{CompactRow, Entity, Value};
#[path = "../../../rust_values/src/fixture_value.rs"]
mod fixture_value;

fn observe(input: &Json) -> Json {
    let mut fields = BTreeMap::from([
        ("id".into(), Value::U64(7)),
        ("name".into(), Value::Text("original".into())),
        ("version".into(), Value::I64(3)),
    ]);
    for (name, value) in input["fields"].as_object().unwrap() {
        fields.insert(name.clone(), fixture_value::value(value));
    }
    let entity = School::from_compact_row(CompactRow::from_map(fields)).unwrap();
    let alias = entity.clone();
    let observations = input["keys"].as_array().unwrap().iter().map(|key| {
        let key = key.as_str().unwrap();
        json!({"key":key, "present":entity.has_dynamic_property(key),
               "value":entity.dynamic_property(key).map(|value|format!("{value:?}")),
               "clone_present":alias.has_dynamic_property(key),
               "clone_value":alias.dynamic_property(key).map(|value|format!("{value:?}"))})
    }).collect::<Vec<_>>();
    json!({"supported":School::supports_dynamic_property_load(), "observations":observations})
}

fn main() {
    for line in io::stdin().lock().lines() {
        println!("{}", observe(&serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
