use serde_json::{json, Value};
use std::io::{self, BufRead};
use teaql_runtime::RuntimeError;

include!("../bootstrap_classifier_original.rs");

fn main() {
    for line in io::stdin().lock().lines() {
        let input: Value = serde_json::from_str(&line.unwrap()).unwrap();
        let message = input["message"].as_str().unwrap().to_string();
        let error = match input["kind"].as_str().unwrap() {
            "graph" => RuntimeError::Graph(message),
            "schema" => RuntimeError::Schema(message),
            "policy" => RuntimeError::Policy(message),
            "id" => RuntimeError::IdGeneration(message),
            "conflict" => RuntimeError::OptimisticLockConflict { entity: "Status".into(), id: "11".into() },
            _ => unreachable!(),
        };
        println!("{}", json!(is_generated_bootstrap_retryable_conflict(&error)));
    }
}
