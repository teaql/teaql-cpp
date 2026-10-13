use std::io::{self, BufRead};
use serde_json::{json, Value as Json};
use teaql_core::{DataType, Value};
#[path = "../../../rust_values/src/fixture_value.rs"]
mod fixture_value;

fn make(input: &Json) -> Value {
    let data = &input["value"];
    let s = || data.as_str().unwrap();
    match input["kind"].as_str().unwrap() {
        "from_i16" => Value::from(s().parse::<i16>().unwrap()),
        "from_i32" => Value::from(s().parse::<i32>().unwrap()),
        "from_i64" => Value::from(s().parse::<i64>().unwrap()),
        "from_u16" => Value::from(s().parse::<u16>().unwrap()),
        "from_u32" => Value::from(s().parse::<u32>().unwrap()),
        "from_u64" => Value::from(s().parse::<u64>().unwrap()),
        "from_f32_bits" => Value::from(f32::from_bits(u32::from_str_radix(s(),16).unwrap())),
        "from_f64_bits" => Value::from(f64::from_bits(u64::from_str_radix(s(),16).unwrap())),
        "from_borrowed_text" => Value::from(s()),
        "from_owned_text" => Value::from(s().to_owned()),
        "from_json" => Value::from(data.clone()),
        "from_bool" => Value::from(data.as_bool().unwrap()),
        "from_decimal" => Value::from(s().parse::<teaql_core::Decimal>().unwrap()),
        "from_date" => Value::from(s().parse::<chrono::NaiveDate>().unwrap()),
        "from_timestamp" => Value::from(teaql_core::time::Timestamp(s().parse().unwrap())),
        "typed_null" => Value::TypedNull([
            DataType::Bool, DataType::I64, DataType::U64, DataType::F64, DataType::Decimal,
            DataType::Text, DataType::LargeText, DataType::Json, DataType::Date, DataType::Timestamp
        ][input["type"].as_u64().unwrap() as usize]),
        _ => fixture_value::value(input),
    }
}

fn evaluate(input: Json) -> Json {
    let value = make(&input["value"]);
    let float = value.try_f64().map(|v| {
        // Width-changing casts do not promise a portable NaN sign/payload.
        // Keep exact bits for finite values and unchanged-width f64 input.
        if input["value"]["kind"] == "from_f32_bits" && v.is_nan() {
            "NaN".to_owned()
        } else {
            format!("{:016x}", v.to_bits())
        }
    });
    let equalities: Vec<bool> = input["others"].as_array().unwrap().iter()
        .map(|other| value == make(other)).collect();
    json!({"debug":format!("{value:?}"), "json":value.to_json_value(),
        "self_equal":value == value, "clone_equal": value == value.clone(), "equalities":equalities,
        "i64":value.try_i64().map(|v|v.to_string()), "u64":value.try_u64().map(|v|v.to_string()),
        "decimal":value.try_decimal().map(|v|v.to_string()),
        "f64_bits":float,
        "bool":value.try_bool(), "text":value.try_text(),
        "date":value.try_date().map(|v|v.to_string()),
        "timestamp":value.try_timestamp().map(|v|v.as_millis().to_string())})
}

fn main() {
    for line in io::stdin().lock().lines() {
        println!("{}", evaluate(serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
