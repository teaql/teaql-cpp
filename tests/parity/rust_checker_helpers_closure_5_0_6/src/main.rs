//! Compile existing observer bodies verbatim with the current reference lock.
use serde_json::{json, Value};
use std::io::{self, BufRead};

include!(concat!(env!("OUT_DIR"), "/original_observers.rs"));

fn main() {
    for line in io::stdin().lock().lines() {
        let input: Value = serde_json::from_str(&line.unwrap()).unwrap();
        let output = if input["kind"] == "checker_helpers" {
            checker_helper_probe(&input)
        } else {
            comment_probe(&input)
        };
        println!("{output}");
    }
}
