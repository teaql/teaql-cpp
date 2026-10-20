//! Execute the original compiler and SQLite provider; no grouping implementation here.
use rusqlite::Connection;
use serde_json::{Value as J, json};
use std::io::{self, BufRead};
use teaql_provider_sqlite::{SqliteDialect, SqliteMutationExecutor};
use teaql_sql::SqlDialect;
#[path = "../rust_values/src/fixture_sql.rs"]
mod fixture_sql;
#[path = "../rust_values/src/fixture_value.rs"]
mod fixture_value;

fn evaluate(input: J) -> J {
    let connection = Connection::open_in_memory().unwrap();
    connection
        .execute_batch(input["seed_sql"].as_str().unwrap())
        .unwrap();
    let entity = fixture_sql::entity(&input["entity"]);
    let query = fixture_sql::query(&input["query"]);
    let compiled = match SqliteDialect.compile_select(&entity, &query) {
        Ok(value) => value,
        Err(error) => return json!({"compile_error": error.to_string()}),
    };
    let provider = SqliteMutationExecutor::from_connection(connection);
    let rows = provider.fetch_all_compact(&compiled).unwrap();
    json!({
        "sql": compiled.sql,
        "params": compiled.params.iter().map(teaql_core::Value::to_json_value).collect::<Vec<_>>(),
        "policies": compiled.log_context.parameter_policies.iter().map(|value| *value as usize).collect::<Vec<_>>(),
        "rows": rows.iter().map(|row| row.iter().map(|(name, value)| (name.clone(), value.to_json_value())).collect::<serde_json::Map<_,_>>()).collect::<Vec<_>>(),
    })
}

fn main() {
    for line in io::stdin().lock().lines() {
        println!(
            "{}",
            evaluate(serde_json::from_str(&line.unwrap()).unwrap())
        );
    }
}
