//! Actual standard SystemTime duration arithmetic on the pinned metadata type.
use serde_json::{json, Value as Json};
use std::io::{self, BufRead};
use std::time::{Duration, SystemTime};

fn instant(input: &Json) -> SystemTime {
    let seconds = input["seconds"].as_str().unwrap().parse::<i64>().unwrap();
    let nanos = input["nanos"].as_u64().unwrap() as u32;
    let base = if seconds < 0 {
        SystemTime::UNIX_EPOCH.checked_sub(Duration::from_secs(seconds.unsigned_abs())).unwrap()
    } else {
        SystemTime::UNIX_EPOCH.checked_add(Duration::from_secs(seconds as u64)).unwrap()
    };
    base.checked_add(Duration::from_nanos(nanos.into())).unwrap()
}

fn model(value: SystemTime) -> Json {
    let signed_nanos = match value.duration_since(SystemTime::UNIX_EPOCH) {
        Ok(value) => value.as_nanos() as i128,
        Err(error) => -(error.duration().as_nanos() as i128),
    };
    json!({"debug":format!("{value:?}"),
        "chrono_nanos":i64::try_from(signed_nanos).ok().map(|v|v.to_string())})
}

fn evaluate(input: Json) -> Json {
    let value = instant(&input["time"]);
    let duration = Duration::new(input["duration"]["seconds"].as_str().unwrap().parse().unwrap(),
        input["duration"]["nanos"].as_u64().unwrap() as u32);
    let add = value.checked_add(duration);
    let sub = value.checked_sub(duration);
    let add_assign = std::panic::catch_unwind(|| { let mut copy=value; copy+=duration; copy });
    let sub_assign = std::panic::catch_unwind(|| { let mut copy=value; copy-=duration; copy });
    json!({"input":model(value), "add":add.map(model), "sub":sub.map(model),
        "add_assign":add_assign.ok().map(model), "sub_assign":sub_assign.ok().map(model),
        "after":model(value)})
}

fn main() {
    std::panic::set_hook(Box::new(|_| {}));
    for line in io::stdin().lock().lines() {
        println!("{}", evaluate(serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
