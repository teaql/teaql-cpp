use std::collections::BTreeMap;
use std::io::{self, BufRead};
use serde_json::{json, Value as Json};
use teaql_core::Value;
#[path = "../fixture_value.rs"]
mod fixture_value;

fn evaluate(input: Json) -> Json {
    let (value, transferred) = match input["kind"].as_str().unwrap() {
        "text" => {
            let text = input["text"].as_str().unwrap().to_owned();
            let pointer = text.as_ptr();
            let value = Value::from(text);
            let transferred = value.try_text().unwrap().as_ptr() == pointer;
            (value, transferred)
        },
        "list" => {
            let items: Vec<Value> = input["values"].as_array().unwrap().iter().map(fixture_value::value).collect();
            let pointer = items.as_ptr();
            let value = Value::List(items);
            let transferred = matches!(&value, Value::List(v) if v.as_ptr() == pointer);
            (value, transferred)
        },
        "object" => {
            let items: BTreeMap<String,Value> = input["values"].as_object().unwrap().iter()
                .map(|(key,input)|(key.clone(),fixture_value::value(input))).collect();
            let pointer = items.first_key_value().map(|(_,v)|v as *const Value);
            let value = Value::object(items);
            let transferred = matches!(&value, Value::Object(v) if v.first_key_value().map(|(_,v)|v as *const Value) == pointer);
            (value, transferred)
        },
        _ => unreachable!(),
    };
    json!({"debug":format!("{value:?}"),"json":value.to_json_value(),
        "clone_equal":value == value.clone(),"transferred":transferred})
}
fn main() {
    for line in io::stdin().lock().lines() {
        println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
