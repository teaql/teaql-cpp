use serde_json::{Value as Json, json};
use std::{io::{self, BufRead}, sync::Arc};
use teaql_core::{CompactRow, CompactRowLayout, DataType, Value};
use teaql_core::dynamic_properties::DynamicPropertyDefinitions;
fn row(name: &str, kind: usize) -> CompactRow {
    let kinds = [DataType::Bool,DataType::I64,DataType::U64,DataType::F64,DataType::Decimal,
                 DataType::Text,DataType::LargeText,DataType::Json,DataType::Date,DataType::Timestamp];
    let definitions = DynamicPropertyDefinitions::new([(name.to_owned(),kinds[kind])]).unwrap();
    let names: Arc<[String]> = vec![name.to_owned()].into();
    let layout = CompactRowLayout::new(names).with_dynamic_property_definitions(definitions);
    CompactRow::with_layout(layout,vec![Value::Null])
}
fn formatted<T: std::fmt::Debug>(value: &T) -> Json {
    json!({"compact":format!("{value:?}"),"pretty":format!("{value:#?}"),
        "lower_hex":format!("{value:x?}"),"upper_hex":format!("{value:X?}"),
        "pretty_lower_hex":format!("{value:#x?}"),"pretty_upper_hex":format!("{value:#X?}"),
        "width_precision":format!("{value:>40.2?}"),"fancy":format!("{value:🧊^+18.3?}"),
        "zero_pad":format!("{value:+012?}")})
}
fn observe(input: &Json) -> Json {
    if input["kind"] == "error_debug" {
        let codes = ["DYNAMIC_PROPERTY_TYPE_MISMATCH", "DYNAMIC_FIELD_OWNER_MISMATCH",
            "DYNAMIC_FIELD_SCHEMA_MISMATCH", "", "custom\0code", "code\n雪", "\\\"code", "_"];
        let error = teaql_core::dynamic_fields::DynamicFieldError {
            code:codes[input["code"].as_u64().unwrap() as usize],
            field:input["name"].as_str().unwrap().to_owned()};
        let copied = error.clone();
        return json!({"debug":formatted(&error),"clone":formatted(&copied),
            "equal_clone":error==copied,"display":error.to_string()});
    }
    let name = input["name"].as_str().unwrap();
    let mut left = row(name,input["left"].as_u64().unwrap() as usize);
    let right = row(name,input["right"].as_u64().unwrap() as usize);
    let before = left.clone();
    let outcome = std::panic::catch_unwind(std::panic::AssertUnwindSafe(||left.extend(right)));
    let error = outcome.err().map(|value|value.downcast_ref::<String>().cloned()
        .or_else(||value.downcast_ref::<&str>().map(|s|s.to_string())).unwrap());
    json!({"error":error,"unchanged":left==before,"before":format!("{before:?}"),"after":format!("{left:?}")})
}
fn main() {
    std::panic::set_hook(Box::new(|_|{}));
    for line in io::stdin().lock().lines() {
        println!("{}",observe(&serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
