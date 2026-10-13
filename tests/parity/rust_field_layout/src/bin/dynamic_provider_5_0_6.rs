use serde_json::{Value as Json, json};
use std::{
    collections::{BTreeMap, HashMap},
    future::Future,
    io::{self, BufRead},
    sync::Arc,
    task::{Context, Poll, Wake, Waker},
};
use teaql_core::dynamic_fields::{
    DynamicFieldDefinitions, DynamicFieldError, DynamicFieldMutation, DynamicFieldSelection,
};
use teaql_core::{DataType, QueryIntent, Value};
use teaql_data_service::dynamic_fields::DynamicFieldStore;
use teaql_provider_sqlite::SqliteDialect;
use teaql_provider_sqlite::SqliteMutationExecutor;
use teaql_runtime::{
    UserContext,
    dynamic_fields::{
        DatabaseDynamicFieldsProvider, DynamicFieldsProvider, InMemoryDynamicFieldsProvider,
    },
};
use teaql_sql::{SqlDataServiceExecutor, SqlTransaction, SqlTransactionTransport, SqlTransport};
struct Schema;
impl teaql_data_service::SchemaProvider for Schema {
    fn get_entity(&self, _: &str) -> Option<Arc<teaql_core::EntityDescriptor>> {
        None
    }
}
type Executor = SqlDataServiceExecutor<SqliteDialect, SqliteMutationExecutor, Schema>;
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
fn dt(n: u64) -> DataType {
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
    ][n as usize]
}
fn error(e: DynamicFieldError) -> Json {
    json!({"error":e.code,"field":e.field})
}
fn observe(input: &Json) -> Json {
    let mut context = UserContext::new();
    let definitions = DynamicFieldDefinitions::new(
        "Item",
        "v1",
        [
            ("title".into(), DataType::Text),
            ("count".into(), DataType::I64),
        ],
    )
    .unwrap();
    let selection = if input["selection"].is_null() {
        Ok(DynamicFieldSelection::All)
    } else {
        DynamicFieldSelection::fields(input["selection"].as_array().unwrap().iter().map(|p| {
            (
                p[0].as_str().unwrap().to_owned(),
                dt(p[1].as_u64().unwrap()),
            )
        }))
    };
    let selection = match selection {
        Ok(v) => v,
        Err(e) => return error(e),
    };
    let executor =
        SqliteMutationExecutor::from_connection(rusqlite::Connection::open_in_memory().unwrap());
    if input["registered"] == true {
        context.insert_resource(SqlDataServiceExecutor::new(
            SqliteDialect,
            executor.clone(),
            Schema,
        ));
    }
    let duplicate = input["duplicate"] == true;
    let provider: Box<dyn DynamicFieldsProvider>;
    if input["mode"] == "fixture" {
        let rows: HashMap<u64, HashMap<String, Value>> = input["stored"]
            .as_object()
            .unwrap()
            .iter()
            .map(|(id, values)| {
                (
                    id.parse().unwrap(),
                    values
                        .as_object()
                        .unwrap()
                        .iter()
                        .map(|(code, v)| (code.clone(), fixture_value::value(v)))
                        .collect(),
                )
            })
            .collect();
        let mut owners = vec![(definitions.clone(), rows.clone())];
        if duplicate {
            owners.push((definitions.clone(), rows));
        }
        provider = match InMemoryDynamicFieldsProvider::from_owners(owners) {
            Ok(v) => Box::new(v),
            Err(e) => return error(e),
        };
    } else {
        let mut owners = vec![definitions.clone()];
        if duplicate {
            owners.push(definitions.clone());
        }
        provider = match DatabaseDynamicFieldsProvider::<Executor>::new(
            input["namespace"].as_str().unwrap(),
            owners,
        ) {
            Ok(v) => Box::new(v),
            Err(e) => return error(e),
        };
    }
    let owner = input["owner"].as_str().unwrap();
    let intent = QueryIntent::new("inspect extensions", "hydrate views").unwrap();
    let ids: Vec<u64> = input["ids"]
        .as_array()
        .unwrap()
        .iter()
        .map(|v| v.as_u64().unwrap())
        .collect();
    let schema = match wait(provider.ensure_schema(&context)) {
        Ok(()) => Json::Null,
        Err(e) => error(e),
    };
    let loaded = match wait(provider.load_values(&context, owner, &ids, &selection, &intent)) {
        Err(e) => error(e),
        Ok(batch) => {
            json!({"bound":batch.definitions.storage_binding_matches(input["namespace"].as_str().unwrap(),executor.source_identity().unwrap()),"rows":batch.rows.iter().map(|(id,row)|(id.to_string(),Json::Object(row.iter().map(|(code,v)|(code.clone(),v.to_json_value())).collect()))).collect::<serde_json::Map<_,_>>()})
        }
    };
    let foreign =
        SqliteMutationExecutor::from_connection(rusqlite::Connection::open_in_memory().unwrap());
    let stream = match provider.prepare_stream_read(
        &context,
        if input["foreign"] == true {
            &foreign
        } else {
            &executor
        },
        owner,
        &selection,
        &intent,
    ) {
        Ok(_) => Json::Null,
        Err(e) => error(e),
    };
    let transaction = wait(executor.begin_sql()).unwrap();
    let expected = match input["binding"].as_str().unwrap() {
        "unbound" => definitions.clone(),
        "wrong_profile" => {
            definitions.with_storage_binding("other", executor.source_identity().unwrap())
        }
        "wrong_schema" => DynamicFieldDefinitions::new(
            "Item",
            "v2",
            [
                ("title".into(), DataType::Text),
                ("count".into(), DataType::I64),
            ],
        )
        .unwrap(),
        _ => definitions.with_storage_binding(
            input["namespace"].as_str().unwrap(),
            executor.source_identity().unwrap(),
        ),
    };
    let changes = BTreeMap::from([(
        "title".into(),
        DynamicFieldMutation::Set(Value::Text("proposed".into())),
    )]);
    let graph = match provider.prepare_graph_write(
        &context,
        transaction.dynamic_field_store().unwrap(),
        &expected,
        7,
        &changes,
    ) {
        Ok(v) => json!({"namespace":v.namespace,"id":v.owner_id,"changes":v.changes.len()}),
        Err(e) => error(e),
    };
    wait(transaction.rollback_sql()).unwrap();
    json!({"schema":schema,"load":loaded,"stream":stream,"graph":graph,"supports_graph":provider.supports_graph_save()})
}
fn main() {
    for line in io::stdin().lock().lines() {
        let v: Json = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", observe(&v));
    }
}
