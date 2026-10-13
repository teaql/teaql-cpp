use serde_json::{Value as Json, json};
use std::io::{self, BufRead};
use teaql_core::{BaseEntityData, DataType, EntityError, Value};
#[path = "../../../rust_values/src/fixture_value.rs"]
mod fixture_value;

fn formatted<T: std::fmt::Debug>(v: &T) -> Json {
    json!({"compact":format!("{v:?}"),"pretty":format!("{v:#?}"),
        "lower_hex":format!("{v:x?}"),"upper_hex":format!("{v:X?}"),
        "pretty_lower_hex":format!("{v:#x?}"),"pretty_upper_hex":format!("{v:#X?}"),
        "width_precision":format!("{v:>40.2?}"),"fancy":format!("{v:🧊^+18.3?}"),
        "zero_pad":format!("{v:+012?}")})
}

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

fn evaluate(input: Json) -> Json {
    let mut base = BaseEntityData::new()
        .with_id(input["id"].as_str().unwrap().parse().unwrap())
        .with_version(input["version"].as_str().unwrap().parse().unwrap());
    for (key, item) in input["dynamic"].as_object().unwrap() {
        base.put_dynamic(key, value(item));
    }
    let error = EntityError::new(input["entity"].as_str().unwrap(), input["message"].as_str().unwrap());
    json!({"base":formatted(&base),"error":formatted(&error)})
}

fn main() {
    for line in io::stdin().lock().lines() {
        println!("{}", evaluate(serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
