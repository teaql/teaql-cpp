//! Actual telemetry model conversions, Clone, PartialEq and derived Debug.
use serde_json::{json, Value as Json};
use std::{fmt::Debug, io::{self, BufRead}};
use teaql_runtime::{RuntimeAttributeValue, RuntimeOperation};

fn formatted<T: Debug>(value: &T) -> Json {
    json!({"compact":format!("{value:?}"),"pretty":format!("{value:#?}"),
        "lower_hex":format!("{value:x?}"),"upper_hex":format!("{value:X?}"),
        "pretty_lower_hex":format!("{value:#x?}"),"pretty_upper_hex":format!("{value:#X?}"),
        "width_precision":format!("{value:>40.2?}"),"fancy":format!("{value:🧊^+18.3?}"),
        "zero_pad":format!("{value:+012?}")})
}

fn attribute(input: &Json) -> RuntimeAttributeValue {
    match input["kind"].as_str().unwrap() {
        "string" => RuntimeAttributeValue::from(input["value"].as_str().unwrap().to_owned()),
        "i64" => RuntimeAttributeValue::from(input["value"].as_str().unwrap().parse::<i64>().unwrap()),
        "usize" => RuntimeAttributeValue::from(input["value"].as_str().unwrap().parse::<usize>().unwrap()),
        "bool" => RuntimeAttributeValue::from(input["value"].as_bool().unwrap()),
        _ => RuntimeAttributeValue::Float(f64::from_bits(u64::from_str_radix(input["value"].as_str().unwrap(),16).unwrap())),
    }
}
fn evaluate(input: Json) -> Json {
    let value = attribute(&input);
    let mut operation = RuntimeOperation::new(input["family"].as_str().unwrap(),input["name"].as_str().unwrap());
    operation = operation.attribute(input["key"].as_str().unwrap(),value.clone());
    let cloned = operation.clone();
    json!({"value":formatted(&value),"clone":formatted(&value.clone()),
        "value_equal":value==value.clone(),"operation":formatted(&operation),
        "operation_clone":formatted(&cloned),"operation_equal":operation==cloned})
}
fn main() {
    for line in io::stdin().lock().lines() {
        println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
