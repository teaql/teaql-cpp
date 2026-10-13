use std::collections::BTreeMap;
use std::io::{self, BufRead};
use serde_json::{json, Value as Json};
use teaql_core::{BaseEntityData, DataType, Value};
#[path = "../../../rust_values/src/fixture_value.rs"]
mod fixture_value;

fn value(input: &Json) -> Value {
    if input["kind"] == "typed_null_type" {
        let types = [DataType::Bool, DataType::I64, DataType::U64, DataType::F64,
            DataType::Decimal, DataType::Text, DataType::LargeText, DataType::Json,
            DataType::Date, DataType::Timestamp];
        Value::TypedNull(types[input["value"].as_u64().unwrap() as usize])
    } else {
        fixture_value::value(input)
    }
}

fn record(values: &BTreeMap<String, Value>) -> Json {
    Json::Object(values.iter().map(|(key, value)| (key.clone(), json!(format!("{value:?}")))).collect())
}

fn evaluate(input: Json) -> Json {
    let fields = input["values"].as_object().unwrap().iter()
        .map(|(key, input)| (key.clone(), value(input))).collect();
    match BaseEntityData::from_values_map(&fields) {
        Ok(base) => json!({"id":base.id.to_string(),"version":base.version.to_string(),
            "dynamic":record(&base.dynamic),"values":record(&base.to_values_map()),
            "clone_equal":base == base.clone()}),
        Err(error) => json!({"entity":error.entity,"message":error.message,"display":error.to_string()}),
    }
}

fn main() {
    for line in io::stdin().lock().lines() {
        println!("{}", evaluate(serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
