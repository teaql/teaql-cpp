use chrono::{Datelike, Timelike, Utc};
use serde_json::{json, Value as Json};
use std::io::{self, BufRead};
use teaql_core::time::Timestamp;

fn debug_modes(value: Timestamp) -> Json {
    json!({
        "compact": format!("{value:?}"), "pretty": format!("{value:#?}"),
        "lower_hex": format!("{value:x?}"), "upper_hex": format!("{value:X?}"),
        "pretty_lower_hex": format!("{value:#x?}"), "pretty_upper_hex": format!("{value:#X?}"),
        "width_precision": format!("{value:>40.2?}"),
        "fancy": format!("{value:🧊^+18.3?}"), "zero_pad": format!("{value:+012?}")
    })
}

fn evaluate(input: Json) -> Json {
    if input["kind"] == "wire" {
        // Observe the actual transparent derive, including raw JSON number kinds.
        return match serde_json::from_str::<Timestamp>(input["wire"].as_str().unwrap()) {
            Ok(value) => json!({"accepted": true, "millis": value.as_millis().to_string(),
                "serialized": serde_json::to_string(&value).unwrap()}),
            Err(_) => json!({"accepted": false}),
        };
    }
    let mut value = if input["kind"] == "unsigned" {
        Timestamp::from(input["value"].as_str().unwrap().parse::<u64>().unwrap())
    } else {
        Timestamp::from(input["value"].as_str().unwrap().parse::<i64>().unwrap())
    };
    if input["kind"] == "field" {
        // Exercise the actual public tuple field, including mutable borrowing.
        let millis = &mut value.0;
        *millis = input["replacement"].as_str().unwrap().parse::<i64>().unwrap();
    }
    let copied = value;
    let cloned = value.clone();
    let datetime = value.to_datetime();
    let before = Utc::now().timestamp_millis();
    let now = Timestamp::now();
    let after = Utc::now().timestamp_millis();
    json!({"millis": value.as_millis().to_string(), "default": Timestamp::default().as_millis(),
        "copy_equal": copied == value, "clone_equal": cloned == value,
        "datetime": [datetime.year(), datetime.month() as i32, datetime.day() as i32,
            datetime.hour() as i32, datetime.minute() as i32, datetime.second() as i32,
            datetime.timestamp_subsec_millis() as i32],
        "serialized": serde_json::to_string(&value).unwrap(), "debug": debug_modes(value),
        "now_in_bounds": before <= now.as_millis() && now.as_millis() <= after})
}

fn main() {
    for line in io::stdin().lock().lines() {
        println!("{}", evaluate(serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
