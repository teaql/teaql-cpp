use serde_json::{Value as Json, json};
use std::{
    collections::BTreeMap,
    future::Future,
    io::{self, BufRead},
    pin::Pin,
    sync::Arc,
    task::{Context, Poll, Wake, Waker},
};
use teaql_core::dynamic_fields::{
    DynamicFieldDefinitions, DynamicFieldError, DynamicFieldMutation, DynamicFieldSelection,
};
use teaql_core::{DataType, MutationIntent, QueryIntent, Value};
use teaql_data_service::dynamic_fields::{
    DynamicFieldStore, DynamicFieldWrite, DynamicStorageRows,
};
use teaql_provider_sqlite::SqliteMutationExecutor;
use teaql_sql::{SqlTransaction, SqlTransactionTransport};
#[path = "../../../rust_values/src/fixture_value.rs"]
mod fixture_value;
struct NoWake;
impl Wake for NoWake {
    fn wake(self: Arc<Self>) {}
}
fn wait<T>(f: impl Future<Output = T>) -> T {
    let mut f = Box::pin(f);
    let w = Waker::from(Arc::new(NoWake));
    let mut c = Context::from_waker(&w);
    loop {
        if let Poll::Ready(v) = f.as_mut().poll(&mut c) {
            return v;
        }
    }
}
fn dt(i: u64) -> DataType {
    [
        DataType::Bool,
        DataType::I64,
        DataType::U64,
        DataType::F64,
        DataType::Decimal,
        DataType::Text,
        DataType::LargeText,
        DataType::Json,
        DataType::Date,
        DataType::Timestamp,
    ][i as usize]
}
fn operand(v: &Json) -> Value {
    if v["kind"] == "typed_null" {
        Value::TypedNull(dt(v["type"].as_u64().unwrap_or(5)))
    } else {
        fixture_value::value(v)
    }
}
fn error(e: DynamicFieldError) -> Json {
    json!({"error":e.code,"field":e.field})
}
fn output(rows: Result<DynamicStorageRows, DynamicFieldError>) -> Json {
    match rows {
        Err(e) => error(e),
        Ok(rows) => {
            json!({"rows":rows.iter().map(|(id,fields)|(id.to_string(),Json::Object(fields.iter().map(|(name,v)|(name.clone(),v.to_json_value())).collect::<serde_json::Map<_,_>>()))).collect::<serde_json::Map<_,_>>()})
        }
    }
}
fn observe(input: &Json) -> Json {
    let provider =
        SqliteMutationExecutor::from_connection(rusqlite::Connection::open_in_memory().unwrap());
    wait(provider.ensure_schema()).unwrap();
    let definitions = DynamicFieldDefinitions::new(
        "Item",
        input["revision"].as_str().unwrap(),
        input["fields"]
            .as_array()
            .unwrap()
            .iter()
            .map(|p| (p[0].as_str().unwrap().into(), dt(p[1].as_u64().unwrap()))),
    )
    .unwrap();
    let writes: Vec<_> = input["writes"]
        .as_array()
        .unwrap()
        .iter()
        .map(|v| DynamicFieldWrite {
            namespace: v["namespace"].as_str().unwrap().into(),
            definitions: definitions.clone(),
            owner_id: v["id"].as_str().unwrap().parse().unwrap(),
            changes: v["changes"]
                .as_object()
                .unwrap()
                .iter()
                .map(|(name, v)| {
                    (
                        name.clone(),
                        if v.is_null() {
                            DynamicFieldMutation::Delete
                        } else {
                            DynamicFieldMutation::Set(operand(v))
                        },
                    )
                })
                .collect::<BTreeMap<_, _>>(),
        })
        .collect();
    let selection = if input["selection"].is_null() {
        DynamicFieldSelection::All
    } else {
        DynamicFieldSelection::fields(
            input["selection"]
                .as_array()
                .unwrap()
                .iter()
                .map(|p| (p[0].as_str().unwrap().into(), dt(p[1].as_u64().unwrap()))),
        )
        .unwrap()
    };
    let ids: Vec<_> = input["ids"]
        .as_array()
        .unwrap()
        .iter()
        .map(|v| v.as_str().unwrap().parse::<u64>().unwrap())
        .collect();
    let namespace = input["namespace"].as_str().unwrap();
    let read = QueryIntent::new("read dynamic fields", "verify storage").unwrap();
    let intent = MutationIntent::new("write dynamic fields").unwrap();
    let tx = wait(provider.begin_sql()).unwrap();
    let validation = match tx.validate_writes(&writes) {
        Ok(()) => Json::Null,
        Err(e) => error(e),
    };
    let applied = match wait(tx.apply_writes(&writes, &intent)) {
        Ok(()) => Json::Null,
        Err(e) => error(e),
    };
    let inside = output(wait(tx.load_values(
        namespace,
        &definitions,
        &ids,
        &selection,
        &read,
    )));
    let mut queued = provider.load_values(namespace, &definitions, &ids, &selection, &read);
    let w = Waker::from(Arc::new(NoWake));
    let mut c = Context::from_waker(&w);
    let pending = matches!(Pin::as_mut(&mut queued).poll(&mut c), Poll::Pending);
    if input["rollback"].as_bool().unwrap() {
        wait(tx.rollback_sql()).unwrap();
    } else {
        wait(tx.commit_sql()).unwrap();
    }
    let after = output(wait(queued));
    if let Some(sql) = input["corrupt"].as_str() {
        provider
            .connection()
            .lock()
            .unwrap()
            .execute_batch(sql)
            .unwrap();
    }
    let final_rows = output(wait(provider.load_values(
        namespace,
        &definitions,
        &ids,
        &selection,
        &read,
    )));
    json!({"validation":validation,"apply":applied,"inside":inside,"pending":pending,"after":after,"final":final_rows})
}
fn main() {
    for line in io::stdin().lock().lines() {
        let v: Json = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", observe(&v));
    }
}
