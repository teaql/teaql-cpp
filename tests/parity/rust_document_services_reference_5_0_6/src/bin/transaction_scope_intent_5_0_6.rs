//! Observe public TransactionScope requests through the frozen runtime and SQLite.
use serde_json::{json, Value as J};
use std::{
    future::Future,
    io::{self, BufRead},
    sync::{Arc, Mutex},
    task::{Context, Poll, Waker},
};
use teaql_core::{DataType, EntityDescriptor, PropertyDescriptor, SelectQuery, UpdateCommand, Value};
use teaql_data_service::{MutationCommand, MutationRequest, QueryRequest, SchemaProvider};
use teaql_provider_sqlite::{SqliteDialect, SqliteMutationExecutor};
use teaql_runtime::{RuntimeError, RuntimeModule, SqlLogOptions, TransactionScope};
use teaql_sql::SqlDataServiceExecutor;

#[derive(Clone)]
struct Schema(Arc<EntityDescriptor>);

impl SchemaProvider for Schema {
    fn get_entity(&self, name: &str) -> Option<Arc<EntityDescriptor>> {
        (name == self.0.name).then(|| self.0.clone())
    }
}

type Executor = SqlDataServiceExecutor<SqliteDialect, SqliteMutationExecutor, Schema>;

fn wait<F: Future>(future: F) -> F::Output {
    let mut future = std::pin::pin!(future);
    match future.as_mut().poll(&mut Context::from_waker(Waker::noop())) {
        Poll::Ready(result) => result,
        Poll::Pending => panic!("Synchronous SQLite fixture unexpectedly suspended"),
    }
}

fn record(values: &std::collections::BTreeMap<String, Value>) -> J {
    json!(values.iter().map(|(name, value)| (name.clone(), format!("{value:?}")))
        .collect::<std::collections::BTreeMap<_, _>>())
}

async fn body(
    scope: &TransactionScope<'_, Executor>,
    input: &J,
    steps: &Mutex<Vec<J>>,
) -> Result<u64, RuntimeError> {
    for action in input["actions"].as_array().unwrap() {
        let item = if action == "query" {
            let query = SelectQuery::new("Order")
                .comment(input["comment"].as_str().unwrap())
                .purpose(input["purpose"].as_str().unwrap());
            let mut request = QueryRequest::from_query(query).map_err(RuntimeError::from)?;
            request.capture_debug_query = true;
            request.capture_execution_metadata = true;
            let result = scope.query(request).await?;
            json!({"kind":"query","rows":result.rows.iter()
                .map(|row| record(&row.clone().into_map())).collect::<Vec<_>>()})
        } else {
            let command = UpdateCommand::new("Order", Value::U64(7))
                .value("name", "changed")
                .expected_version(input["version"].as_i64().unwrap());
            let request = MutationRequest::new(
                MutationCommand::Update(command), input["reason"].as_str().unwrap(),
            ).map_err(RuntimeError::from)?;
            let result = scope.mutate(request).await?;
            json!({"kind":"mutate","affected":result.affected_rows,
                "snapshot":result.persisted_snapshot.as_ref().map(|values| record(values))})
        };
        steps.lock().unwrap().push(item);
    }
    if input["body_fail"] == true {
        Err(RuntimeError::Event("business failed".into()))
    } else {
        Ok(17)
    }
}

fn evaluate(input: J) -> J {
    let connection = rusqlite::Connection::open_in_memory().unwrap();
    connection.execute_batch(
        "CREATE TABLE orders(id INTEGER PRIMARY KEY,version INTEGER,name TEXT);\
         INSERT INTO orders VALUES(7,3,'original');",
    ).unwrap();
    let connection = Arc::new(Mutex::new(connection));
    let descriptor = EntityDescriptor::new("Order")
        .table_name("orders")
        .property(PropertyDescriptor::new("id", DataType::U64).id())
        .property(PropertyDescriptor::new("version", DataType::I64).version())
        .property(PropertyDescriptor::new("name", DataType::Text));
    let executor = SqlDataServiceExecutor::new(
        SqliteDialect, SqliteMutationExecutor::new(connection.clone()),
        Schema(Arc::new(descriptor.clone())),
    );
    let mut user_context = RuntimeModule::new().descriptor(descriptor).into_context();
    user_context.set_user_identifier("fixture");
    user_context.set_trace_id("fixture");
    user_context.set_sql_log_options(SqlLogOptions {
        select:input["select_logs"].as_bool().unwrap(),
        mutation:input["mutation_logs"].as_bool().unwrap(),
    });
    user_context.insert_resource(executor);
    let steps = Mutex::new(vec![]);
    let result = wait(user_context.execute_in_transaction::<Executor, u64, _>(|scope| {
        Box::pin(body(scope, &input, &steps))
    }));
    let outcome = match result {
        Ok(value) => json!({"value":value}),
        Err(error) => json!({"error":error.to_string()}),
    };
    let persisted = {
        let connection = connection.lock().unwrap();
        let (id, version, name): (i64, i64, String) = connection.query_row(
            "SELECT id,version,name FROM orders", [],
            |row| Ok((row.get(0)?, row.get(1)?, row.get(2)?)),
        ).unwrap();
        json!([{"id":format!("I64({id})"), "version":format!("I64({version})"),
            "name":format!("{:?}", Value::Text(name))}])
    };
    let logs = user_context.sql_logs().iter().map(|entry|json!({
        "context":format!("{:?}",entry.log_context),"operation":format!("{:?}",entry.operation),
        "comment":entry.comment,"purpose":entry.purpose,"audit_reason":entry.audit_reason,
        "trace":entry.trace_path.iter().map(|node|json!({"kind":node.kind as u8,
            "entity":node.entity_type,"id":node.entity_id,"comment":node.comment})).collect::<Vec<_>>(),"sql":entry.sql,
        "params":entry.params.iter().map(|value|format!("{value:?}")).collect::<Vec<_>>(),
        "debug_sql":entry.debug_sql,"pretty_sql":entry.pretty_sql,
        "result_count":entry.result_count,"result_type":entry.result_type,
        "affected_rows":entry.affected_rows,"summary":entry.result_summary,
    })).collect::<Vec<_>>();
    json!({"outcome":outcome,"steps":steps.into_inner().unwrap(),"persisted":persisted,"logs":logs})
}

fn main() {
    unsafe { std::env::set_var("TEAQL_LOG_ENDPOINT", "off"); }
    for line in io::stdin().lock().lines() {
        println!("{}", evaluate(serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
