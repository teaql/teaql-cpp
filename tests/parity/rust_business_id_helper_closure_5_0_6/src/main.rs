use serde_json::{json, Value};
use std::io::{self, BufRead};
include!(concat!(env!("OUT_DIR"), "/original_observers.rs"));
fn main() {
    for line in io::stdin().lock().lines() {
        let input: Value = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", business_id_probe(&input));
    }
}
