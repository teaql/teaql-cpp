//! Current request intent validation before the existing original executor observer.
#[path = "../rust_values/src/fixture_value.rs"]
mod fixture_value;
mod current {
    include!(concat!(
        env!("CARGO_MANIFEST_DIR"),
        "/../rust_field_layout/src/bin/sql_execution_5_0_6.rs"
    ));

    fn validate(input: &J) -> Result<(), teaql_core::RequestIntentError> {
        teaql_core::MutationIntent::from_optional(input["comment"].as_str())?;
        if input["mode"] == "batch" {
            for child in input["children"].as_array().unwrap() {
                validate(child)?;
            }
        }
        Ok(())
    }

    pub fn observe(input: J) -> J {
        if let Err(error) = validate(&input["request"]) {
            let mut output = json!({"intent_error":error.to_string(),"calls_before_poll":0,"calls":[],"events":[]});
            if input["observe_kind"] == true {
                output["kind_calls"] = json!(0);
            }
            return output;
        }
        evaluate(input)
    }
}

fn main() {
    use std::io::{self, BufRead};
    std::panic::set_hook(Box::new(|_| {}));
    for line in io::stdin().lock().lines() {
        println!(
            "{}",
            current::observe(serde_json::from_str(&line.unwrap()).unwrap())
        );
    }
}
