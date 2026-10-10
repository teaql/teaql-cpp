use std::io::{self, BufRead};
use rusqlite::{Connection, types::ValueRef};
use serde_json::{json, Value as Json};
use teaql_provider_sqlite::{SqliteMutationExecutor, MutationExecutorError};
use teaql_sql::CompiledQuery;
#[path = "../fixture_value.rs"] mod fixture_value;

fn evaluate(input: Json) -> Json {
    let connection = Connection::open_in_memory().unwrap();
    let affinity = input["affinity"].as_str().unwrap();
    connection.execute(&format!("CREATE TABLE data(v {affinity})"), []).unwrap();
    let provider = SqliteMutationExecutor::from_connection(connection);
    let query = CompiledQuery {
        log_context: Default::default(),
        sql: input["sql"].as_str().unwrap_or("INSERT INTO data VALUES(?)").into(),
        params: input["params"].as_array().unwrap().iter().map(fixture_value::value).collect(),
        comment: None,
    };
    match provider.execute(&query) {
        Err(error) => {
            let kind = match &error {
                MutationExecutorError::Sqlite(_) => "Sqlite",
                MutationExecutorError::SqlCompile(_) => "SqlCompile",
                MutationExecutorError::UnsupportedValue(_) => "UnsupportedValue",
                MutationExecutorError::UnsupportedColumnType(_) => "UnsupportedColumnType",
                MutationExecutorError::Bind(_) => "Bind",
                MutationExecutorError::Lock(_) => "Lock",
            };
            json!({"error": error.to_string(), "kind": kind,
                "source_none":std::error::Error::source(&error).is_none()})
        },
        Ok(affected) => {
            let connection = provider.connection(); let connection = connection.lock().unwrap();
            let observed = connection.query_row("SELECT v FROM data", [], |row| {
                Ok(match row.get_ref(0)? {
                    ValueRef::Null => json!({"type": "null", "value": null}),
                    ValueRef::Integer(v) => json!({"type": "integer", "value": v.to_string()}),
                    ValueRef::Real(v) => json!({"type": "real", "value": if v.is_infinite() {json!(if v < 0.0 {"-inf"} else {"inf"})} else {json!(v)}}),
                    ValueRef::Text(v) => json!({"type": "text", "value": std::str::from_utf8(v).unwrap()}),
                    ValueRef::Blob(_) => panic!("binding unexpectedly produced blob"),
                })
            }).unwrap();
            json!({"affected": affected, "stored": observed})
        }
    }
}
fn main() {
    for line in io::stdin().lock().lines() {
        println!("{}", evaluate(serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
