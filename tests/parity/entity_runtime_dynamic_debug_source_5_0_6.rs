use serde_json::{Value as Json, json};
use std::{collections::{BTreeMap, HashMap}, io::{self, BufRead}};
use teaql_core::dynamic_fields::{DynamicFieldDefinitions, DynamicFieldValues};
use teaql_core::{CompactRow, DataType, EntitySnapshot, Value};
use teaql_runtime::{EntityKey, EntityRuntimeState};

fn evaluate(input: Json) -> Json {
    let mut definitions = DynamicFieldDefinitions::new("Item", "v1",
        [("name".to_owned(), DataType::Text)]).unwrap();
    if input["bound"] == true { definitions = definitions.with_storage_binding("profile", 37); }
    let values = match input["loaded"].as_u64().unwrap() {
        0 => HashMap::new(),
        1 => HashMap::from([("name".to_owned(), Value::Null)]),
        _ => HashMap::from([("name".to_owned(), Value::Text("loaded".to_owned()))]),
    };
    let mut state = EntityRuntimeState::default();
    state.install_loaded_dynamic_fields(DynamicFieldValues::from_values(definitions, values).unwrap());
    let key = EntityKey::new_static("Item", 7_u64);
    if input["row"] == true {
        state.set_original_compact_row("Item", CompactRow::from_map(BTreeMap::from([
            ("id".to_owned(), Value::U64(7)), ("version".to_owned(), Value::I64(3))])));
    }
    if input["active"] == true {
        state.set_comment("root");
        state.set_entity_comment(key.clone(), "local");
        state.mark_as_new(key.clone());
        state.set_original_version(key.clone(), 3);
        state.set_original_snapshot(EntitySnapshot::from(BTreeMap::from([
            ("name".to_owned(), Value::Text("ledger original".to_owned()))])));
    }
    let copied = if input["copy"] == true { Some(state.clone()) } else { None };
    let before = format!("{state:?}");
    match input["action"].as_str().unwrap() {
        "none" => {},
        "set" => { state.update_dynamic_field(key.clone(), "name", "changed".into()).unwrap(); },
        "delete" => { state.delete_dynamic_field(key.clone(), "name").unwrap(); },
        "set_delete" => {
            state.update_dynamic_field(key.clone(), "name", "changed".into()).unwrap();
            state.delete_dynamic_field(key.clone(), "name").unwrap();
        },
        "delete_set" => {
            state.delete_dynamic_field(key.clone(), "name").unwrap();
            state.update_dynamic_field(key.clone(), "name", "changed".into()).unwrap();
        },
        "pop" | "clear" => {
            state.push_change_set();
            state.update_dynamic_field(key.clone(), "name", "changed".into()).unwrap();
            if input["action"] == "pop" { state.pop_change_set(); }
            else { state.clear_current_change_set(); }
        },
        "commit" => {
            state.update_dynamic_field(key.clone(), "name", "changed".into()).unwrap();
            state.clear_committed();
        },
        "mark_delete" => {
            state.update_dynamic_field(key.clone(), "name", "changed".into()).unwrap();
            state.mark_as_delete(key);
        },
        _ => panic!("Unknown dynamic diagnostic action"),
    }
    let changes = state.current_change_set();
    json!({"before":before,"after":format!("{state:?}"),"pretty":format!("{state:#?}"),
        "changes":format!("{changes:?}"),"pretty_changes":format!("{changes:#?}"),
        "copy_after":copied.as_ref().map(|value|format!("{value:?}"))})
}

fn main() {
    for line in io::stdin().lock().lines() {
        println!("{}", evaluate(serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
