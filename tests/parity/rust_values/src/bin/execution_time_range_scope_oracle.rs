//! Observe the exact original metadata time domain before its C++ repair.
use serde_json::json;
use std::time::{Duration, SystemTime};
use teaql_data_service::ExecutionMetadata;

fn instant(seconds: i64, nanos: u32) -> Option<SystemTime> {
    if seconds >= 0 {
        SystemTime::UNIX_EPOCH.checked_add(Duration::new(seconds as u64, nanos))
    } else if nanos == 0 {
        SystemTime::UNIX_EPOCH.checked_sub(Duration::new(seconds.unsigned_abs(), 0))
    } else {
        SystemTime::UNIX_EPOCH.checked_sub(Duration::new(seconds.unsigned_abs() - 1, 1_000_000_000 - nanos))
    }
}

fn main() {
    for (seconds, nanos) in [(10_000_000_000, 123), (-10_000_000_000, 456),
        (i64::MAX, 999_999_999), (i64::MIN, 0)] {
        let value = instant(seconds, nanos);
        let mut metadata = ExecutionMetadata::unrecorded_query(0);
        if let Some(value) = value { metadata.started_at = value; metadata.ended_at = value; }
        println!("{}", json!({"seconds": seconds.to_string(), "nanos": nanos,
            "accepted": value.is_some(), "diagnostic": value.map(|v| format!("{v:?}")),
            "metadata_clone_preserves": metadata.clone().started_at == metadata.started_at}));
    }
}
