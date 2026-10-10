//! Invoke the original standard Result, SystemTimeError and Duration formatters.
use serde_json::{json, Value as Json};
use std::{error::Error, fmt::Debug, io::{self, BufRead}, time::{Duration, SystemTime}};

fn instant(input: &Json) -> SystemTime {
    let seconds: i64 = input["seconds"].as_str().unwrap().parse().unwrap();
    let nanos = input["nanos"].as_u64().unwrap();
    let base = if seconds < 0 {
        SystemTime::UNIX_EPOCH.checked_sub(Duration::from_secs(seconds.unsigned_abs())).unwrap()
    } else {
        SystemTime::UNIX_EPOCH.checked_add(Duration::from_secs(seconds as u64)).unwrap()
    };
    base.checked_add(Duration::from_nanos(nanos)).unwrap()
}

fn formatted<T: Debug>(value: &T) -> Json {
    json!({"compact":format!("{value:?}"), "pretty":format!("{value:#?}"),
        "lower_hex":format!("{value:x?}"), "upper_hex":format!("{value:X?}"),
        "pretty_lower_hex":format!("{value:#x?}"), "pretty_upper_hex":format!("{value:#X?}"),
        "width_precision":format!("{value:>40.2?}"), "fancy":format!("{value:🧊^+18.3?}"),
        "zero_pad":format!("{value:+012?}"), "precision_zero":format!("{value:.0?}"),
        "precision_one":format!("{value:.1?}"), "precision_twelve":format!("{value:.12?}")})
}

fn evaluate(input: Json) -> Json {
    let start = instant(&input["start"]);
    let end = instant(&input["end"]);
    let result = end.duration_since(start);
    let mut output = json!({"ok":result.is_ok(), "result":formatted(&result),
        "clone_result":formatted(&result.clone()),"start_debug":format!("{start:?}"),
        "end_debug":format!("{end:?}")});
    let duration = match result {
        Ok(duration) => duration,
        Err(error) => {
            output["error"] = formatted(&error);
            output["clone_error"] = formatted(&error.clone());
            output["display"] = json!([format!("{error}"),format!("{error:>80.2}"),
                format!("{error:🧊^+80.3}"),format!("{error:+080}")]);
            output["source_none"] = json!(error.source().is_none());
            error.duration()
        }
    };
    output["duration"] = json!({"seconds":duration.as_secs().to_string(),
        "nanos":duration.subsec_nanos(),"debug":formatted(&duration)});
    output
}

fn main() {
    for line in io::stdin().lock().lines() {
        println!("{}", evaluate(serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
