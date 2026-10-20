//! Observe unchanged generated configuration and SQLite assembly functions.
//! Environment and working directory are supplied by an isolated test process.
use cpp_dynamic_json_core::{ServiceRuntimeConfig, Q};
use serde_json::json;
use std::error::Error;

#[path = "../../../rust_field_layout/src/bin/original_like_privacy_5_0_6.rs"]
mod driver;

fn main() {
    let mode = std::env::args().nth(1).expect("configuration observation mode");
    let output = match mode.as_str() {
        "config" => match ServiceRuntimeConfig::from_env() {
            Ok(config) => json!({
                "ok": true,
                "database_url": config.database_url,
                "debug": format!("{config:?}"),
                "clone_equal": config == config.clone(),
            }),
            Err(error) => json!({
                "ok": false,
                "display": error.to_string(),
                "debug": format!("{error:?}"),
                "source": error.source().map(ToString::to_string),
            }),
        },
        "connect" => match driver::block_on(cpp_dynamic_json_core::service_runtime_from_env()) {
            Ok(user_context) => {
                // Startup deliberately leaves schema creation to the application.
                let query = Q::people_minimal()
                    .comment("Observe generated runtime assembly")
                    .purpose("Check explicit schema initialization boundary");
                let query_error = driver::block_on(query.execute_for_count(&user_context))
                    .err().map(|error| error.to_string());
                json!({"ok": true, "query_error": query_error})
            }
            Err(error) => json!({
                "ok": false,
                "display": error.to_string(),
                "debug": format!("{error:?}"),
                "source": error.source().map(ToString::to_string),
            }),
        },
        _ => panic!("unknown configuration observation mode"),
    };
    println!("{output}");
}
