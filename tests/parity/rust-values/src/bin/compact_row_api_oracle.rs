//! Observe original CompactRow public calls, including Arc layout snapshots.
use serde_json::{Value as Json, json};
use std::collections::BTreeMap;
use std::io::{self, BufRead};
use std::sync::Arc;
use teaql_core::{CompactRow, DataType, Value};

fn value(input: &Json) -> Value {
    let v = &input["value"];
    match input["kind"].as_str().unwrap() {
        "null" => Value::Null,
        "typed_null" => Value::TypedNull(DataType::Text),
        "bool" => Value::Bool(v.as_bool().unwrap()),
        "text" => Value::Text(v.as_str().unwrap().into()),
        "i64" => Value::I64(v.as_str().unwrap().parse().unwrap()),
        "u64" => Value::U64(v.as_str().unwrap().parse().unwrap()),
        "f64" => Value::F64(v.as_str().unwrap().parse().unwrap()),
        "decimal" => Value::Decimal(v.as_str().unwrap().parse().unwrap()),
        "date" => Value::Date(Value::from(v.as_str().unwrap()).try_date().unwrap()),
        "timestamp" => Value::Timestamp(teaql_core::time::Timestamp::from(
            v.as_str().unwrap().parse::<i64>().unwrap(),
        )),
        "json" => Value::Json(v.clone()),
        "list" => Value::List(v.as_array().unwrap().iter().map(value).collect()),
        "object" => Value::Object(
            v.as_object()
                .unwrap()
                .iter()
                .map(|(k, v)| (k.clone(), value(v)))
                .collect(),
        ),
        _ => panic!("invalid value fixture"),
    }
}
fn row(input: &Json) -> CompactRow {
    let names: Vec<String> = input["names"]
        .as_array()
        .unwrap()
        .iter()
        .map(|v| v.as_str().unwrap().into())
        .collect();
    let values: Vec<Value> = input["values"]
        .as_array()
        .unwrap()
        .iter()
        .map(value)
        .collect();
    if input["from_map"].as_bool().unwrap_or(false) {
        CompactRow::from_map(names.into_iter().zip(values).collect())
    } else {
        CompactRow::new(names.into(), values)
    }
}
fn pairs(row: &CompactRow) -> Json {
    json!(
        row.iter()
            .map(|(k, v)| (k, format!("{v:?}")))
            .collect::<Vec<_>>()
    )
}
fn map_output(map: BTreeMap<String, Value>) -> Json {
    json!(
        map.iter()
            .map(|(k, v)| (k, format!("{v:?}")))
            .collect::<Vec<_>>()
    )
}
fn state(
    row: &CompactRow,
    clone: &CompactRow,
    saved: &[Arc<[String]>],
    before: &Arc<[String]>,
    previous: Option<Value>,
) -> Json {
    json!({"pairs":pairs(row),"map":map_output(row.clone().into_map()),"length":row.len(),
           "empty":row.is_empty(),"clone":pairs(clone),"equal_clone":row==clone,
           "same_clone_layout":Arc::ptr_eq(&row.shared_columns(),&clone.shared_columns()),
           "layout_changed":!Arc::ptr_eq(&row.shared_columns(),before),
           "layout":row.shared_columns().as_ref(),"saved":saved.iter().map(|v|v.to_vec()).collect::<Vec<_>>(),
           "previous":previous.map(|v|format!("{v:?}"))})
}
fn observe(input: Json) -> Json {
    if input["kind"] == "json_projection" {
        let current = row(&input);
        let map = current.clone().into_map();
        return json!({"row":teaql_core::compact_row_to_json_value(&current),
            "record":teaql_core::record_to_json_value(&map),"rank":teaql_core::PARTITION_RANK_PROPERTY,
            "keys":current.keys().collect::<Vec<_>>(),
            "values":current.values().map(|value|format!("{value:?}")).collect::<Vec<_>>()});
    }
    let mut current = row(&input);
    let mut clone = current.clone();
    let mut saved = Vec::new();
    let initial = current.shared_columns();
    let mut outputs = vec![state(&current, &clone, &saved, &initial, None)];
    for op in input["ops"].as_array().unwrap() {
        let before = current.shared_columns();
        let name = op["name"].as_str().unwrap_or("");
        let previous = match op["op"].as_str().unwrap() {
            "save" => {
                saved.push(current.shared_columns());
                None
            }
            "clone" => {
                clone = current.clone();
                None
            }
            "insert" => current.insert(name.to_owned(), value(&op["value"])),
            "remove" => current.remove(name),
            "get" => {
                assert_eq!(current.contains_key(name), current.get(name).is_some());
                current.get(name).cloned()
            }
            "set" => current
                .get_mut(name)
                .map(|v| std::mem::replace(v, value(&op["value"]))),
            "extend" => {
                current.extend(row(op));
                None
            }
            "self_extend" => {
                current.extend(current.clone());
                None
            }
            "consume" => {
                current = CompactRow::from_map(current.into_map());
                None
            }
            _ => panic!("invalid operation"),
        };
        outputs.push(state(&current, &clone, &saved, &before, previous));
    }
    json!(outputs)
}
fn main() {
    for line in io::stdin().lock().lines() {
        println!("{}", observe(serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
