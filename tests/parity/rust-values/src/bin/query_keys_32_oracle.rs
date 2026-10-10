//! Observe unchanged original key methods with the fixture's 32-bit usize inputs.
#[allow(dead_code)]
#[path = "query_keys_oracle.rs"]
mod original;
// The original fixture modules resolve these crate-qualified imports.
#[path = "../fixture_value.rs"]
mod fixture_value;
#[allow(dead_code)]
#[path = "../fixture_sql.rs"]
mod fixture_sql;
use std::io::{self, BufRead};

fn main() {
    for line in io::stdin().lock().lines() {
        let input: serde_json::Value = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", original::observe_target(&input, 32));
    }
}
