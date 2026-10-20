//! Keep the historical driver byte unchanged; expose original checked panics.
mod original {
    include!(concat!(
        env!("CARGO_MANIFEST_DIR"),
        "/../rust_values/src/bin/sqlite_decoding_oracle.rs"
    ));
    pub fn run(input: serde_json::Value) -> serde_json::Value {
        match std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| evaluate(input))) {
            Ok(value) => value,
            Err(error) => {
                let text = error
                    .downcast_ref::<String>()
                    .map(String::as_str)
                    .or_else(|| error.downcast_ref::<&str>().copied());
                if text == Some("attempt to subtract with overflow") {
                    serde_json::json!({"checked_failure": "arithmetic_overflow", "message": text})
                } else {
                    std::panic::resume_unwind(error)
                }
            }
        }
    }
}

fn main() {
    use std::io::{self, BufRead};
    for line in io::stdin().lock().lines() {
        let input = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", original::run(input));
    }
}
