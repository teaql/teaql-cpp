//! Original Unix SystemTime range and execution-metadata diagnostics.
use serde_json::{Value as Json, json};
use std::{io::{self, BufRead}, time::{Duration, SystemTime}};
use teaql_data_service::ExecutionMetadata;

fn time(value: &Json) -> Option<SystemTime> {
    let seconds: i64 = value["seconds"].as_str().unwrap().parse().unwrap();
    let nanos = value["nanos"].as_u64().unwrap() as u32;
    let base = if seconds < 0 {
        SystemTime::UNIX_EPOCH.checked_sub(Duration::from_secs(seconds.unsigned_abs()))
    } else {
        SystemTime::UNIX_EPOCH.checked_add(Duration::from_secs(seconds as u64))
    }?;
    base.checked_add(Duration::from_nanos(nanos.into()))
}
fn model(value: SystemTime) -> Json {
    let nanos: i128 = match value.duration_since(SystemTime::UNIX_EPOCH) {
        Ok(d)=>d.as_nanos() as i128, Err(e)=>-(e.duration().as_nanos() as i128)
    };
    json!({"debug":format!("{value:?}"),"chrono_nanos":i64::try_from(nanos).ok().map(|v|v.to_string())})
}
fn formatted<T: std::fmt::Debug>(value: &T) -> Json {
    json!({"compact":format!("{value:?}"),"pretty":format!("{value:#?}"),
        "lower_hex":format!("{value:x?}"),"upper_hex":format!("{value:X?}"),
        "pretty_lower_hex":format!("{value:#x?}"),"pretty_upper_hex":format!("{value:#X?}"),
        "width_precision":format!("{value:>40.2?}"),"fancy":format!("{value:🧊^+18.3?}"),
        "zero_pad":format!("{value:+012?}")})
}
fn evaluate(input: Json) -> Json {
    let Some(start)=time(&input["start"]) else {return json!({"valid":false});};
    let Some(end)=time(&input["end"]) else {return json!({"valid":false});};
    let count: u64=input["delta"].as_str().unwrap().parse().unwrap();
    let mut metadata=ExecutionMetadata::unrecorded_query(1);
    metadata.started_at=start;metadata.ended_at=end;
    let elapsed=end.duration_since(start).ok().map(|d|json!({"seconds":d.as_secs().to_string(),"nanos":d.subsec_nanos()}));
    json!({"valid":true,"start":model(start),"end":model(end),"elapsed":elapsed,
        "add":start.checked_add(Duration::from_secs(count)).map(model),
        "sub":start.checked_sub(Duration::from_secs(count)).map(model),
        "order":match start.cmp(&end){std::cmp::Ordering::Less=>-1,std::cmp::Ordering::Equal=>0,std::cmp::Ordering::Greater=>1},
        "metadata":formatted(&metadata)})
}
fn main() {
    for line in io::stdin().lock().lines() {
        println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
