// Appended to a disposable SQL lib.rs. Every operation calls the original guards.
#[doc(hidden)]
pub fn __cpp_diagnostic_guards_506(input: serde_json::Value) -> serde_json::Value {
    use diagnostic_execution::{FailureJournal, StatementDiagnostic};
    use serde_json::{Value as Json, json};
    use std::{sync::{Arc, Mutex}, time::{Duration, SystemTime}};
    use teaql_data_service::{ExecutionMetadata, ExecutionObserver, SqlExecutionOutcome};
    fn metadata(id: usize, compiled: bool) -> ExecutionMetadata {
        let mut value = ExecutionMetadata::unrecorded_query(id);
        value.started_at = SystemTime::UNIX_EPOCH+Duration::from_secs(7);
        value.ended_at = value.started_at;
        value.comment = Some(format!("statement-{id}"));
        value.backend = "fixture".to_owned();
        value.params.push(teaql_core::Value::Text(format!("value-{id}")));
        if compiled { value.parameterized_query = Some("SELECT ?".to_owned()); }
        value
    }
    fn describe(value: &ExecutionMetadata) -> Json {
        let epoch = SystemTime::UNIX_EPOCH+Duration::from_secs(7);
        json!({"count":value.result_count,"comment":value.comment,"sql":value.parameterized_query,
            "backend":value.backend,"params":value.params.iter().map(|v|v.to_json_value()).collect::<Vec<_>>(),
            "outcome":value.sql_log.execution_outcome.map(|v|match v {
                SqlExecutionOutcome::Success=>"Success", SqlExecutionOutcome::Failure=>"Failure",
                SqlExecutionOutcome::Cancelled=>"Cancelled"}),
            "started_preserved":value.started_at==epoch,"ended_updated":value.ended_at>epoch,
            "debug":value.debug_query})
    }
    let entries = Arc::new(Mutex::new(Vec::<Json>::new()));
    let observer = if input["observer"] == true {
        let target = entries.clone();
        let panic_sink = input["panic_sink"] == true;
        Some(Arc::new(move |value| {
            target.lock().unwrap().push(describe(&value));
            assert!(!panic_sink, "controlled observer failure");
        }) as ExecutionObserver<'static>)
    } else { None };
    let compiled = input["compiled"] == true;
    let mut returned = Vec::new();
    let recorder;
    if input["mode"] == "statement" {
        recorder = false;
        let mut guard = Some(StatementDiagnostic::new(
            (input["metadata"] == true).then(||metadata(7, compiled)), observer));
        for action in input["actions"].as_array().unwrap() {
            match action.as_str().unwrap() {
                "success" => returned.push(guard.as_mut().unwrap().success().as_ref().map(describe)),
                "fail" => guard.as_mut().unwrap().fail(),
                "drop" | "close" => drop(guard.take()),
                "unwind" | "unwind_close" => {
                    let owned = guard.take();
                    let _ = std::panic::catch_unwind(std::panic::AssertUnwindSafe(move || {
                        let _guard = owned;
                        panic!("controlled consumer failure");
                    }));
                },
                _ => panic!("Unknown statement diagnostic action"),
            }
        }
        drop(guard);
    } else {
        let mut journal = Some(FailureJournal::new(observer));
        let record = journal.as_ref().unwrap().recorder();
        recorder = record.is_some();
        let mut id = 0;
        for action in input["actions"].as_array().unwrap() {
            match action.as_str().unwrap() {
                "record" => {
                    id += 1;
                    if let Some(callback) = &record { callback(metadata(id, compiled)); }
                },
                "disarm" => journal.as_mut().unwrap().disarm(),
                "drop" | "close" => drop(journal.take()),
                "unwind" | "unwind_close" => {
                    let owned = journal.take();
                    let _ = std::panic::catch_unwind(std::panic::AssertUnwindSafe(move || {
                        let _journal = owned;
                        panic!("controlled consumer failure");
                    }));
                },
                _ => panic!("Unknown journal diagnostic action"),
            }
        }
        drop(journal);
    }
    let notifications = entries.lock().unwrap().clone();
    json!({"entries":notifications,"returned":returned,"recorder":recorder})
}
