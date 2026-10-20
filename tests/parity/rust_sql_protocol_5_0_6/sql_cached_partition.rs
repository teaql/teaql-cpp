//! Read real SQLite through the original cache/executor with changed HAVING values.
use serde_json::{Value as J, json};
use std::{
    future::Future,
    io::{self, BufRead},
    sync::Arc,
    task::{Context, Poll, Waker},
};
use teaql_core::{EntityDescriptor, QueryIntent};
use teaql_data_service::{QueryExecutor, QueryRequest, SchemaProvider};
use teaql_provider_sqlite::{SqliteDialect, SqliteMutationExecutor};
use teaql_sql::SqlDataServiceExecutor;
#[path = "../rust_values/src/fixture_sql.rs"]
mod fixture_sql;
#[path = "../rust_values/src/fixture_value.rs"]
mod fixture_value;

struct Schema(Arc<EntityDescriptor>);
impl SchemaProvider for Schema {
    fn get_entity(&self, name: &str) -> Option<Arc<EntityDescriptor>> {
        (name == self.0.name).then(|| self.0.clone())
    }
}

fn evaluate(input: J) -> J {
    let connection = rusqlite::Connection::open_in_memory().unwrap();
    connection
        .execute_batch(input["seed_sql"].as_str().unwrap())
        .unwrap();
    let entity = fixture_sql::entity(&input["entity"]);
    let name = entity.name.clone();
    let executor = SqlDataServiceExecutor::new(
        SqliteDialect,
        SqliteMutationExecutor::from_connection(connection),
        Schema(Arc::new(entity)),
    );
    let mut results = vec![];
    for value in input["queries"].as_array().unwrap() {
        let mut query = fixture_sql::query(value);
        query.entity = name.clone();
        let request = QueryRequest::new(
            query,
            QueryIntent::new(
                "Inspect cached grouped rows",
                "Verify SQLite HAVING rebinding",
            )
            .unwrap(),
        );
        let mut future = std::pin::pin!(executor.query(request));
        let output = match future
            .as_mut()
            .poll(&mut Context::from_waker(Waker::noop()))
        {
            Poll::Ready(Ok(result)) => json!({
                "rows": result.rows.iter().map(|row| row.iter().map(|(name,value)| (name.clone(),value.to_json_value())).collect::<serde_json::Map<_,_>>()).collect::<Vec<_>>(),
                "params": result.metadata.params.iter().map(teaql_core::Value::to_json_value).collect::<Vec<_>>(),
                "policies": result.metadata.sql_log.parameter_policies.iter().map(|value| *value as usize).collect::<Vec<_>>(),
                "sql": result.metadata.parameterized_query,
            }),
            Poll::Ready(Err(error)) => json!({"error":error.to_string()}),
            Poll::Pending => panic!("SQLite fixture unexpectedly pending"),
        };
        results.push(output);
    }
    json!({"results":results})
}

fn main() {
    for line in io::stdin().lock().lines() {
        println!(
            "{}",
            evaluate(serde_json::from_str(&line.unwrap()).unwrap())
        );
    }
}
