//! Direct Debug/Display/Error observations of the pinned SQL executor error.
use serde_json::{json, Value};
use std::io::{self, BufRead};
use teaql_sql::{SqlCompileError, SqlExecutorError};

#[derive(Debug)]
struct Error(String);
impl std::fmt::Display for Error {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_str(&self.0)
    }
}
impl std::error::Error for Error {}

fn formatted<T: std::fmt::Debug>(value: &T) -> Value {
    json!({"compact":format!("{value:?}"),"pretty":format!("{value:#?}"),
        "lower_hex":format!("{value:x?}"),"upper_hex":format!("{value:X?}"),
        "pretty_lower_hex":format!("{value:#x?}"),"pretty_upper_hex":format!("{value:#X?}"),
        "width_precision":format!("{value:>40.2?}"),"fancy":format!("{value:🧊^+18.3?}"),
        "zero_pad":format!("{value:+012?}")})
}
fn evaluate(input: Value) -> Value {
    let text = input["text"].as_str().unwrap().to_owned();
    let version = input["version"].as_str().unwrap().parse::<i64>().unwrap();
    let error: SqlExecutorError<Error> = match input["kind"].as_str().unwrap() {
        "field" => SqlExecutorError::Compile(SqlCompileError::UnknownField(text)),
        "empty" => SqlExecutorError::Compile(SqlCompileError::EmptyInList),
        "version" => SqlExecutorError::Compile(SqlCompileError::InvalidRecoverVersion(version)),
        "shape" => SqlExecutorError::Compile(SqlCompileError::InvalidSchemaShape {
            property: text.clone(), reason: text,
        }),
        "transport" => SqlExecutorError::Transport(Error(text)),
        _ => SqlExecutorError::PersistedRecord(text),
    };
    json!({"debug":formatted(&error),"display":error.to_string(),
        "source_none":std::error::Error::source(&error).is_none()})
}
fn main() {
    for line in io::stdin().lock().lines() {
        println!("{}", evaluate(serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
