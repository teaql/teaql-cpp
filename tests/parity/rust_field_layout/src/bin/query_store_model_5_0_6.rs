//! Original public pagination/cache model diagnostics and Clone/equality.
use serde_json::{json, Value};
use std::{io::{self, BufRead}, sync::Arc, time::{Duration, Instant, SystemTime, UNIX_EPOCH}};
use teaql_core::SortDirection;
use teaql_runtime::{ContinuousPageCursor, RetainedIdSet, InMemoryAggregationCache};
#[path = "../../../rust_values/src/fixture_value.rs"] mod fixture_value;
fn formatted<T: std::fmt::Debug>(value: &T) -> Value {
    json!({"compact":format!("{value:?}"),"pretty":format!("{value:#?}"),
        "lower_hex":format!("{value:x?}"),"upper_hex":format!("{value:X?}"),
        "pretty_lower_hex":format!("{value:#x?}"),"pretty_upper_hex":format!("{value:#X?}"),
        "width_precision":format!("{value:>40.2?}"),"fancy":format!("{value:🧊^+18.3?}"),
        "zero_pad":format!("{value:+012?}")})
}
fn number(input: &Value, key: &str) -> u64 {input[key].as_str().unwrap().parse().unwrap()}
fn expiry(input: &Value) -> SystemTime {
    let seconds = input["seconds"].as_str().unwrap().parse::<i64>().unwrap();
    let nanos = input["nanos"].as_u64().unwrap() as u32;
    if seconds >= 0 {UNIX_EPOCH + Duration::new(seconds as u64,nanos)}
    else {UNIX_EPOCH - Duration::from_secs(seconds.unsigned_abs()) + Duration::new(0,nanos)}
}
fn cursor(input: &Value) -> ContinuousPageCursor {
    ContinuousPageCursor {cursor_id: input["cursor"].as_str().unwrap().into(),
        query_key: input["key"].as_str().unwrap().into(), entity: input["entity"].as_str().unwrap().into(),
        direction: if input["direction"] == "Asc" {SortDirection::Asc} else {SortDirection::Desc},
        boundary: fixture_value::value(&input["boundary"]), page_size: number(input,"page_size"),
        next_offset: number(input,"offset"), expires_at: expiry(input)}
}
fn instant_ticks(text: &str) -> u128 {
    let integer = |field: &str| -> u128 {
        text.split_once(field).unwrap().1.trim_start().chars().take_while(|c| c.is_ascii_digit())
            .collect::<String>().parse().unwrap()
    };
    integer("tv_sec:") * 1_000_000_000 + integer("tv_nsec:")
}
fn evaluate(input: Value) -> Value {
    match input["kind"].as_str().unwrap() {
        "cursor" => {let value = cursor(&input); let other = cursor(&input["other"]);
            json!({"debug":formatted(&value),"clone":formatted(&value.clone()),
                "clone_equal":value == value.clone(), "other_equal": value == other})},
        "ids" => {let value = RetainedIdSet {query_key: input["key"].as_str().unwrap().into(),
            ids: Arc::new(input["ids"].as_array().unwrap().iter().map(|v|v.as_str().unwrap().parse().unwrap()).collect()),
            expires_at: expiry(&input)};
            json!({"debug":formatted(&value),"clone":formatted(&value.clone())})},
        "cache_live" => {
            let cache = InMemoryAggregationCache::with_namespace("ns");
            let row = teaql_core::CompactRow::new(vec!["value".to_owned()].into(), vec![teaql_core::Value::Text("clock payload".into())]);
            let row_debug = format!("{row:?}");
            let before = Instant::now();
            cache.put("ns::item".into(), vec![row]);
            let after = Instant::now();
            let debug = format!("{cache:?}");
            let ticks = instant_ticks(&debug);
            let cloned = cache.get("ns::item", 0).unwrap();
            json!({"within_put_bracket": ticks >= instant_ticks(&format!("{before:?}")) && ticks <= instant_ticks(&format!("{after:?}")),
                "row_debug_included": debug.contains(&row_debug),
                "read_preserves_timestamp": debug == format!("{cache:?}"),
                "rows_equal": format!("{:?}", cloned[0]) == row_debug})
        },
        "cache" => {let default = InMemoryAggregationCache::default();
            let named = InMemoryAggregationCache::with_namespace(input["namespace"].as_str().unwrap());
            json!({"default":formatted(&default),"default_namespace":default.namespace(),
                "named":formatted(&named),"namespace":named.namespace()})},
        _ => panic!("unknown query-store model"),
    }
}
fn main() {for line in io::stdin().lock().lines() {
    println!("{}", evaluate(serde_json::from_str(&line.unwrap()).unwrap()));
}}
