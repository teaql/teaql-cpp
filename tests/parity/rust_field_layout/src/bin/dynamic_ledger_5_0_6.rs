use serde_json::{Value as Json, json};
use std::{
    collections::HashMap,
    io::{self, BufRead},
};
use teaql_core::dynamic_fields::{
    DynamicFieldDefinitions, DynamicFieldMutation, DynamicFieldValues,
};
use teaql_core::{DataType, Value};
use teaql_runtime::{EntityKey, EntityRuntimeState};
#[path = "../../../rust_values/src/fixture_value.rs"]
mod fixture_value;
fn key(input: &Json) -> EntityKey {
    EntityKey::new(input["entity"].as_str().unwrap_or("Item"), Value::U64(7))
}
fn load(state: &mut EntityRuntimeState, input: &Json) {
    let owner = input["owner"].as_str().unwrap_or("Item");
    let d = DynamicFieldDefinitions::new(
        owner,
        "v1",
        vec![
            ("name".into(), DataType::Text),
            ("count".into(), DataType::I64),
        ],
    )
    .unwrap();
    let d = if let Some(e) = input["executor"].as_u64() {
        d.with_storage_binding("profile", e)
    } else {
        d
    };
    let values: HashMap<_, _> = input["values"]
        .as_object()
        .unwrap()
        .iter()
        .map(|(k, v)| (k.clone(), fixture_value::value(v)))
        .collect();
    state.install_loaded_dynamic_fields(DynamicFieldValues::from_values(d, values).unwrap());
}
fn state(s: &EntityRuntimeState) -> Json {
    let k = EntityKey::new("Item", Value::U64(7));
    let c = s.current_change_set();
    let dynamic=c.dynamic_changes().map(|m|m.iter().map(|(key,fields)|{
        json!({"entity":key.entity.as_ref(),"id":key.id.to_json_value(),"fields":fields.iter().map(|(name,mutation)|{
            (name.clone(),match mutation{DynamicFieldMutation::Set(v)=>json!({"op":"Set","value":v.to_json_value()}),DynamicFieldMutation::Delete=>json!({"op":"Delete"})})
        }).collect::<serde_json::Map<_,_>>()})
    }).collect::<Vec<_>>());
    json!({"values":s.loaded_dynamic_fields().map(|v|v.values().iter().map(|(k,v)|(k.clone(),v.to_json_value())).collect::<serde_json::Map<_,_>>()),
        "pending":s.has_pending_dynamic_mutations(),"empty":c.is_empty(),"dynamic":dynamic,
        "fields":s.changed_field_names(&k),"native":s.get(&k,"native").map(|v|v.to_json_value()),"comment":s.get_comment()})
}
fn observe(input: &Json) -> Json {
    let mut states: Vec<_> = input["initial"]
        .as_array()
        .unwrap()
        .iter()
        .map(|v| {
            let mut s = EntityRuntimeState::default();
            if !v.is_null() {
                load(&mut s, v);
            }
            s
        })
        .collect();
    let initial: Vec<_> = states.iter().map(state).collect();
    let mut outcomes = Vec::new();
    for a in input["actions"].as_array().unwrap() {
        let i = a["state"].as_u64().unwrap() as usize;
        let op = a["op"].as_str().unwrap();
        let result = match op {
            "set" => match states[i].update_dynamic_field(
                key(a),
                a["code"].as_str().unwrap(),
                fixture_value::value(&a["value"]),
            ) {
                Ok(v) => json!(v),
                Err(e) => json!({"error":e.code,"field":e.field}),
            },
            "delete" => match states[i].delete_dynamic_field(key(a), a["code"].as_str().unwrap()) {
                Ok(v) => json!(v),
                Err(e) => json!({"error":e.code,"field":e.field}),
            },
            "clone" => {
                states.push(states[i].clone());
                Json::Null
            }
            "fresh" => {
                states.push(EntityRuntimeState::fresh_with_shared_graph(&states[i]));
                Json::Null
            }
            "rebind" => {
                let s = states[a["from"].as_u64().unwrap() as usize].clone();
                states[i] = states[i].clone().with_loaded_view_from(&s);
                Json::Null
            }
            "adopt" => {
                states[i].adopt_mutations_from(&states[a["from"].as_u64().unwrap() as usize]);
                Json::Null
            }
            "load" => {
                load(&mut states[i], &a["load"]);
                Json::Null
            }
            "native" => {
                states[i].set(key(a), "native", fixture_value::value(&a["value"]));
                Json::Null
            }
            "comment" => {
                states[i].set_comment(a["text"].as_str().unwrap());
                Json::Null
            }
            "push" => {
                states[i].push_change_set();
                Json::Null
            }
            "pop" => json!(states[i].pop_change_set().is_some()),
            "clear" => {
                states[i].clear_current_change_set();
                Json::Null
            }
            "commit" => {
                states[i].clear_committed();
                Json::Null
            }
            _ => panic!(),
        };
        outcomes.push(json!({"result":result,"states":states.iter().map(state).collect::<Vec<_>>(),"equal":states.iter().map(|s|s==&states[0]).collect::<Vec<_>>()}));
    }
    json!({"initial":initial,"outcomes":outcomes})
}
fn main() {
    for line in io::stdin().lock().lines() {
        let v: Json = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", observe(&v));
    }
}
