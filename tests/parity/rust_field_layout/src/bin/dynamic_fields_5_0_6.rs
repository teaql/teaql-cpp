use serde_json::{Value as Json, json};
use std::collections::HashMap;
use std::io::{self, BufRead};
use teaql_core::dynamic_fields::{
    DynamicFieldDefinitions, DynamicFieldError, DynamicFieldMergeShapes, DynamicFieldSelection,
    DynamicFieldValues,
};
use teaql_core::{DataType, Value};
#[path = "../../../rust_values/src/fixture_value.rs"]
mod fixture_value;

fn types(input: &Json) -> Vec<(String, DataType)> {
    let all = [
        DataType::Bool,
        DataType::I64,
        DataType::U64,
        DataType::F64,
        DataType::Decimal,
        DataType::Text,
        DataType::LargeText,
        DataType::Json,
        DataType::Date,
        DataType::Timestamp,
    ];
    input
        .as_array()
        .unwrap()
        .iter()
        .map(|v| {
            (
                v[0].as_str().unwrap().into(),
                all[v[1].as_u64().unwrap() as usize],
            )
        })
        .collect()
}
fn values(input: &Json) -> HashMap<String, Value> {
    input
        .as_object()
        .unwrap()
        .iter()
        .map(|(k, v)| (k.clone(), fixture_value::value(v)))
        .collect()
}
fn error(e: DynamicFieldError) -> Json {
    json!({"error":e.code, "field":e.field})
}
fn view(input: &DynamicFieldValues, lookups: &Json) -> Json {
    let mut codes: Vec<_> = input.selected_codes().iter().collect();
    codes.sort();
    let fields: Vec<_> = lookups
        .as_array()
        .unwrap()
        .iter()
        .map(|code| match input.field(code.as_str().unwrap()) {
            Err(e) => error(e),
            Ok(v) => json!({"type": v.data_type() as u32, "state":format!("{:?}",v.state()),
                "value": v.value().map(Value::to_json_value), "loaded":v.is_loaded()}),
        })
        .collect();
    json!({"codes":codes,"fields":fields})
}
fn observe(input: &Json) -> Json {
    let definitions = match DynamicFieldDefinitions::new(
        input["owner"].as_str().unwrap(),
        input["revision"].as_str().unwrap(),
        types(&input["definitions"]),
    ) {
        Ok(d) => d,
        Err(e) => return error(e),
    };
    if input["mode"] == "selection" {
        let selection = if input["selection"].is_null() {
            DynamicFieldSelection::All
        } else {
            match DynamicFieldSelection::fields(types(&input["selection"])) {
                Ok(s) => s,
                Err(e) => return error(e),
            }
        };
        return match selection.validate(&definitions) {
            Err(e) => error(e),
            Ok(()) => {
                json!({"contains":input["lookups"].as_array().unwrap().iter().map(|c| selection.contains(c.as_str().unwrap())).collect::<Vec<_>>()})
            }
        };
    }
    if input["mode"] == "binding" {
        let a = definitions.with_storage_binding("profile", 7);
        let same = a.with_storage_binding("profile", 7);
        let b = a.with_storage_binding("profile", 8);
        return json!({"same_schema":a.same_schema(&b),"equal":a==b,"same_identity":std::sync::Arc::ptr_eq(&a,&same),
            "match":a.storage_binding_matches("profile",7),"wrong_profile":a.storage_binding_matches("other",7),"unbound":definitions.storage_binding_matches("profile",7)});
    }
    let rows = input["rows"].as_array().unwrap().iter().map(values);
    let mut batch = match DynamicFieldValues::from_batch(definitions, rows) {
        Ok(b) => b,
        Err(e) => return error(e),
    };
    let initial: Vec<_> = batch.iter().map(|v| view(v, &input["lookups"])).collect();
    let shared: Vec<_> = batch
        .iter()
        .map(|a| {
            batch
                .iter()
                .map(|b| std::sync::Arc::ptr_eq(a.selected_codes(), b.selected_codes()))
                .collect::<Vec<_>>()
        })
        .collect();
    let mut shapes = DynamicFieldMergeShapes::default();
    let mut outcomes = Vec::new();
    for action in input["actions"].as_array().unwrap() {
        let i = action["row"].as_u64().unwrap() as usize;
        let before = batch[i].selected_codes().clone();
        let result = match action["op"].as_str().unwrap() {
            "assign" => batch[i].assign(
                action["code"].as_str().unwrap(),
                fixture_value::value(&action["value"]),
            ),
            "delete" => batch[i].delete(action["code"].as_str().unwrap()),
            "merge" => {
                let other = batch[action["other"].as_u64().unwrap() as usize].clone();
                batch[i].merge_loaded(other, &mut shapes)
            }
            _ => panic!("unknown action"),
        };
        outcomes.push(
            json!({"result":match result {Ok(())=>Json::Null, Err(e)=>error(e)},
            "same_shape":std::sync::Arc::ptr_eq(&before,batch[i].selected_codes())}),
        );
    }
    let final_shared: Vec<_> = batch
        .iter()
        .map(|a| {
            batch
                .iter()
                .map(|b| std::sync::Arc::ptr_eq(a.selected_codes(), b.selected_codes()))
                .collect::<Vec<_>>()
        })
        .collect();
    json!({"initial":initial,"shared":shared,"outcomes":outcomes,"final_shared":final_shared,"final":batch.iter().map(|v|view(v,&input["lookups"])).collect::<Vec<_>>()})
}
fn main() {
    for line in io::stdin().lock().lines() {
        let input: Json = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", observe(&input));
    }
}
