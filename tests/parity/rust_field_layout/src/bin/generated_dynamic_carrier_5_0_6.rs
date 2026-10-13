use school_management_service_core::{Platform, School};
use serde_json::{json, Value as Json};
use std::{
    collections::{BTreeMap, HashMap},
    io::{self, BufRead},
    sync::Arc,
};
use teaql_core::dynamic_fields::{DynamicFieldDefinitions, DynamicFieldValues};
use teaql_core::{CompactRow, DataType, Entity, LoadedSnapshot, TeaqlEntity, Value};
#[path = "../../../rust_values/src/fixture_value.rs"]
mod fixture_value;
fn observe(input: &Json) -> Json {
    let row = CompactRow::from_map(BTreeMap::from([
        ("id".into(), Value::U64(7)),
        ("name".into(), Value::Text("original".into())),
        ("version".into(), Value::I64(3)),
    ]));
    let mut entity = School::from_compact_row(row).unwrap();
    let defs = DynamicFieldDefinitions::new(
        "School",
        "v1",
        [
            ("title".into(), DataType::Text),
            ("count".into(), DataType::I64),
        ],
    )
    .unwrap();
    let values = input["stored"]
        .as_object()
        .unwrap()
        .iter()
        .map(|(name, value)| (name.clone(), fixture_value::value(value)))
        .collect::<HashMap<_, _>>();
    let fields = DynamicFieldValues::from_values(defs, values).unwrap();
    let base = entity.loaded_state_snapshot().unwrap();
    let state = LoadedSnapshot::with_dynamic_fields(&base, &fields).unwrap();
    entity.install_loaded_dynamic_fields(fields, state).unwrap();
    if input["mode"] == "missing" {
        entity.__load_state = teaql_core::eval::LoadState::FullyLoaded;
    }
    if input["mode"] == "layout" {
        entity.__load_state = teaql_core::eval::LoadState::FullyLoaded
            .into_indexed(Platform::field_layout().unwrap().unwrap())
            .unwrap();
    }
    let before = entity.loaded_state_snapshot();
    let copy = entity.clone();
    let error = if input["mode"] == "install_owner" {
        let wrong = DynamicFieldDefinitions::new("Other", "v1", [("title".into(), DataType::Text)])
            .unwrap();
        let fields = DynamicFieldValues::from_values(wrong, HashMap::new()).unwrap();
        entity
            .install_loaded_dynamic_fields(fields, base)
            .err()
            .map(|error| error.message)
    } else if input["mode"] == "install_layout" {
        let fields = entity.dynamic_field_values().unwrap().clone();
        let state = teaql_core::eval::LoadState::FullyLoaded
            .into_indexed(Platform::field_layout().unwrap().unwrap())
            .unwrap();
        let teaql_core::eval::LoadState::Indexed(state) = state else {
            unreachable!()
        };
        entity
            .install_loaded_dynamic_fields(fields, state)
            .err()
            .map(|error| error.message)
    } else if input["op"] == "delete" {
        entity
            .delete_dynamic_field(input["code"].as_str().unwrap())
            .err()
            .map(|error| error.to_string())
    } else {
        entity
            .update_dynamic_field(
                input["code"].as_str().unwrap(),
                fixture_value::value(&input["value"]),
            )
            .err()
            .map(|error| error.to_string())
    };
    let after = entity.loaded_state_snapshot();
    let view = |e: &School| json!({"loaded":[e.is_field_loaded("#title"),e.is_field_loaded("#count")],"values":e.dynamic_field_values().unwrap().values().iter().map(|(name,value)|(name.clone(),value.to_json_value())).collect::<serde_json::Map<_,_>>(),"pending":e.has_pending_dynamic_mutations()});
    json!({"error":error,"view":view(&entity),"copy":view(&copy),"same_snapshot":match(before,after){(Some(a),Some(b))=>Arc::ptr_eq(&a,&b),(None,None)=>true,_=>false}})
}
fn main() {
    for line in io::stdin().lock().lines() {
        let input: Json = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", observe(&input));
    }
}
