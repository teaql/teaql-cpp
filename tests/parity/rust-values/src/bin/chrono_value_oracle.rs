use std::io::{self, BufRead};
use chrono::{DateTime, Utc};
use serde_json::{json, Value as Json};
use teaql_core::Value;

fn evaluate(input: Json) -> Json {
    let ticks: i64 = input["ticks"].as_str().unwrap().parse().unwrap();
    let instant: DateTime<Utc> = match input["unit"].as_str().unwrap() {
        "seconds" => DateTime::from_timestamp(ticks, 0).unwrap(),
        "milliseconds" => DateTime::from_timestamp_millis(ticks).unwrap(),
        "microseconds" => DateTime::from_timestamp_micros(ticks).unwrap(),
        "nanoseconds" => DateTime::from_timestamp_nanos(ticks),
        _ => unreachable!(),
    };
    let value = Value::from(instant);
    json!({"debug":format!("{value:?}"),
        "millis":value.try_timestamp().unwrap().as_millis().to_string(),
        "json":value.to_json_value()})
}
fn main() {
    for line in io::stdin().lock().lines() {
        println!("{}", evaluate(serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
