//! Actual five-branch generated service errors and their preserved sources.
use cpp_dynamic_json_core::ServiceRuntimeError;
use serde_json::json;
use std::error::Error;

fn main() {
    let errors = vec![
        ("missing", ServiceRuntimeError::MissingEnv {
            name: "CPP_DYNAMIC_JSON_CORE_DATABASE_URL", source: std::env::VarError::NotPresent,
        }),
        ("connection", ServiceRuntimeError::ConnectionError("a retained connection diagnostic".into())),
        ("configuration", ServiceRuntimeError::Configuration("an invalid configured profile".into())),
        ("sqlite", ServiceRuntimeError::from(rusqlite::Error::InvalidQuery)),
        ("runtime", ServiceRuntimeError::from(teaql_runtime::RuntimeError::Graph("a retained runtime diagnostic".into()))),
    ];
    let output = errors.into_iter().map(|(name, error)| json!({
        "name": name, "display": error.to_string(), "debug": format!("{error:?}"),
        "source": error.source().map(ToString::to_string),
        "source_debug": error.source().map(|source| format!("{source:?}")),
    })).collect::<Vec<_>>();
    println!("{}", json!(output));
}
