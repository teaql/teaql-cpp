//! Actual direct-config NUL boundaries, independent of the retained env observer.
use cpp_dynamic_json_core::ServiceRuntimeConfig;
use serde_json::json;
use std::error::Error;

#[path = "../../../rust_field_layout/src/bin/original_like_privacy_5_0_6.rs"]
mod driver;

fn main() {
    let input = std::env::args().nth(1).expect("fixture");
    let url = match input.as_str() {
        "parent" => "folder\0name/data.db",
        "file" => "folder/data\0name.db",
        _ => panic!("unknown fixture"),
    };
    let config = ServiceRuntimeConfig { database_url: url.into() };
    let output = match driver::block_on(cpp_dynamic_json_core::service_runtime(config)) {
        Ok(_) => json!({"ok": true}),
        Err(error) => json!({"ok": false, "display": error.to_string(),
                             "debug": format!("{error:?}"),
                             "source": error.source().map(ToString::to_string)}),
    };
    println!("{output}");
}
