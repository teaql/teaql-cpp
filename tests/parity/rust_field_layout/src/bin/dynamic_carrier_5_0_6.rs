use serde_json::{Value as Json, json};
use std::{
    collections::{BTreeMap, HashMap},
    io::{self, BufRead},
    sync::Arc,
};
use teaql_core::dynamic_fields::{
    DynamicFieldDefinitions, DynamicFieldMergeShapes, DynamicFieldValues,
};
use teaql_core::dynamic_properties::DynamicPropertyDefinitions;
use teaql_core::{CompactRow, DataType, FieldLayout, LoadedSnapshot, SmartList, Value};
#[path = "../../../rust_values/src/fixture_value.rs"]
mod fixture_value;
fn dt(i: u64) -> DataType {
    [
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
    ][i as usize]
}
fn record(v: &Json) -> BTreeMap<String, Value> {
    v.as_object()
        .unwrap()
        .iter()
        .map(|(k, v)| (k.clone(), fixture_value::value(v)))
        .collect()
}
fn layout(input: &Json) -> Arc<FieldLayout> {
    let count = input["count"].as_u64().unwrap() as usize;
    let names: Vec<String> = (0..count)
        .map(|i| {
            if i == 0 {
                "id".into()
            } else if i == 1 {
                "version".into()
            } else {
                format!("field_{i}")
            }
        })
        .collect();
    let columns: Vec<_> = (0..count).map(|i| format!("db_{i}")).collect();
    let indexes: Vec<_> = names
        .iter()
        .enumerate()
        .map(|(i, n)| (n.as_str(), i))
        .collect();
    let mappings: Vec<_> = names
        .iter()
        .zip(&columns)
        .map(|(n, c)| (n.as_str(), n.as_str(), c.as_str()))
        .collect();
    let members: Vec<_> = names.iter().map(String::as_str).collect();
    FieldLayout::from_generated("Item", "v1", &indexes, &mappings, &["lines"], &members).unwrap()
}
fn schema(v: &Json) -> Option<Arc<DynamicPropertyDefinitions>> {
    if v.is_null() {
        None
    } else {
        Some(
            DynamicPropertyDefinitions::new(v.as_array().unwrap().iter().map(|p| {
                (
                    p[0].as_str().unwrap().to_owned(),
                    dt(p[1].as_u64().unwrap()),
                )
            }))
            .unwrap(),
        )
    }
}
fn definitions(v: &Json) -> Arc<DynamicFieldDefinitions> {
    let d = DynamicFieldDefinitions::new(
        v["owner"].as_str().unwrap(),
        "v1",
        vec![
            ("extension".into(), DataType::Text),
            ("count".into(), DataType::I64),
        ],
    )
    .unwrap();
    if let Some(executor) = v["executor"].as_u64() {
        d.with_storage_binding("profile", executor)
    } else {
        d
    }
}
fn dynamic(v: &Json) -> DynamicFieldValues {
    DynamicFieldValues::from_values(
        definitions(v),
        record(&v["values"]).into_iter().collect::<HashMap<_, _>>(),
    )
    .unwrap()
}
fn state(s: &Arc<LoadedSnapshot>, lookups: &Json) -> Json {
    let mut overflow = s.overflow().map(|v| v.iter().copied().collect::<Vec<_>>());
    if let Some(v) = &mut overflow {
        v.sort();
    }
    let mut names = s
        .selected_names()
        .map(|v| v.iter().cloned().collect::<Vec<_>>());
    if let Some(v) = &mut names {
        v.sort();
    }
    json!({"bits":s.bits().to_string(),"overflow":overflow,"names":names,
        "schema":s.dynamic_property_definitions().map(|d|d.data_type("_total").map(|t|t as u32)),
        "loaded":lookups.as_array().unwrap().iter().map(|n|s.is_loaded(n.as_str().unwrap())).collect::<Vec<_>>()})
}
fn row_state(row: &CompactRow, input: &Json, l: &Arc<FieldLayout>) -> Json {
    let mut copy = row.clone();
    let dynamic = copy.take_loaded_dynamic_fields();
    let snapshot = match row.indexed_load_state(l.clone()) {
        teaql_core::eval::LoadState::Indexed(s) => s,
        _ => panic!(),
    };
    json!({"map": row.clone().into_map().iter().map(|(k,v)|(k.clone(),v.to_json_value())).collect::<serde_json::Map<_,_>>(),
        "dynamic":dynamic.map(|d|d.values().iter().map(|(k,v)|(k.clone(),v.to_json_value())).collect::<serde_json::Map<_,_>>()),
        "relation_marked":row.is_loaded_relation("lines"),"relations":row.has_loaded_relations(),
        "relation_length":row.loaded_relation("lines").map(|l|l.len()),"snapshot":state(&snapshot,&input["lookups"])})
}
fn observe(input: &Json) -> Json {
    let l = layout(input);
    if input["mode"] == "snapshot" {
        let selected: Vec<_> = input["selected"]
            .as_array()
            .unwrap()
            .iter()
            .map(|n| n.as_str().unwrap())
            .collect();
        let mut s = if input["full"].as_bool().unwrap() {
            LoadedSnapshot::fully_loaded(l)
        } else {
            LoadedSnapshot::projection(l, selected)
        }
        .into_shared();
        if let Some(d) = schema(&input["schema"]) {
            s = LoadedSnapshot::with_dynamic_property_definitions(&s, d);
        }
        let initial = state(&s, &input["lookups"]);
        let original = s.clone();
        let mut stages = Vec::new();
        for v in input["payloads"].as_array().unwrap() {
            let before = s.clone();
            match LoadedSnapshot::with_dynamic_fields(&s, &dynamic(v)) {
                Ok(next) => {
                    s = next;
                    stages.push(
                        json!({"same":Arc::ptr_eq(&before,&s),"state":state(&s,&input["lookups"])}),
                    );
                }
                Err(e) => stages.push(json!({"error":e})),
            }
        }
        return json!({"initial":initial,"original":state(&original,&input["lookups"]),"stages":stages});
    }
    let mut rows: Vec<_> = input["rows"]
        .as_array()
        .unwrap()
        .iter()
        .map(|v| {
            let mut row = CompactRow::from(record(&v["native"]));
            if let Some(d) = schema(&v["schema"]) {
                CompactRow::attach_dynamic_property_definitions(std::slice::from_mut(&mut row), d);
            }
            if !v["dynamic"].is_null() {
                row.set_loaded_dynamic_fields(dynamic(&v["dynamic"]));
            }
            if v["relation"].as_bool().unwrap() {
                row.mark_relation_loaded("lines", &mut Default::default());
                row.set_loaded_relation("lines".into(), SmartList::new(vec![]));
            }
            row
        })
        .collect();
    let initial: Vec<_> = rows.iter().map(|r| row_state(r, input, &l)).collect();
    let original = rows[0].clone();
    let mut outcomes = Vec::new();
    let mut shapes = DynamicFieldMergeShapes::default();
    for action in input["actions"].as_array().unwrap() {
        let before = rows[0].shared_layout();
        let outcome = match action.as_str().unwrap() {
            "merge" => {
                let other = rows[1].clone();
                match rows[0].try_extend(other, &mut shapes) {
                    Ok(()) => Json::Null,
                    Err(e) => json!({"error":e.code,"field":e.field}),
                }
            }
            "clear" => {
                rows[0].clear_loaded_relations();
                Json::Null
            }
            "take" => {
                rows[0].take_loaded_dynamic_fields();
                Json::Null
            }
            "remove_relation" => {
                rows[0].remove("lines");
                Json::Null
            }
            _ => panic!(),
        };
        outcomes.push(json!({"result":outcome,"same_layout":Arc::ptr_eq(&before,&rows[0].shared_layout()),"state":row_state(&rows[0],input,&l)}));
    }
    json!({"initial":initial,"original":row_state(&original,input,&l),"outcomes":outcomes})
}
fn main() {
    for line in io::stdin().lock().lines() {
        let v: Json = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", observe(&v));
    }
}
