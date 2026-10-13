use serde_json::{json, Value};
use std::{
    error::Error,
    io::{self, BufRead},
};
use teaql_runtime::RuntimeError;
fn main() {
    for line in io::stdin().lock().lines() {
        let input: Value = serde_json::from_str(&line.unwrap()).unwrap();
        let mut error = RuntimeError::Event(input["message"].as_str().unwrap().to_owned());
        for _ in 0..input["depth"].as_u64().unwrap() {
            error = RuntimeError::AuditAfterCommit {
                source: Box::new(error),
            };
        }
        println!(
            "{}",
            json!({"display":error.to_string(),"debug":format!("{error:?}"),"pretty":format!("{error:#?}"),"source_none":error.source().is_none()})
        );
    }
}
