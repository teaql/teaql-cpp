//! Observe the complete original private helper through an append-only access bridge.
use serde_json::{Value as J, json};
use std::{
    future::Future,
    io::{self, BufRead},
    sync::{Arc, Mutex},
    task::{Context, Poll, Wake, Waker},
};
use teaql_core::*;
use teaql_data_service::*;
use teaql_runtime::{EntityDataService, EntityRuntimeState, RuntimeModule};

#[derive(Clone)]
struct Model {
    row: CompactRow,
    state: Option<EntityRuntimeState>,
}
impl TeaqlEntity for Model {
    const ENTITY_NAME: &'static str = "Parent";
    fn entity_descriptor() -> EntityDescriptor {
        EntityDescriptor::new(Self::ENTITY_NAME)
    }
}
impl Entity for Model {
    fn from_compact_row(row: CompactRow) -> Result<Self, EntityError> {
        if row.get("fail").and_then(Value::try_bool) == Some(true) {
            return Err(EntityError::new(Self::ENTITY_NAME, "model decode failed"));
        }
        Ok(Self { row, state: None })
    }
    fn into_values(self) -> MutationValues {
        self.row.into_map().into()
    }
    fn on_loaded(&mut self, context: &dyn std::any::Any) {
        if let Some(root) = context.downcast_ref::<EntityRuntimeState>() {
            let mut state = root.clone();
            state.set_original_compact_row(Self::ENTITY_NAME, self.row.clone());
            self.state = Some(state);
        }
    }
}
struct Executor {
    input: J,
    projection: Mutex<Vec<String>>,
}
impl DataServiceExecutor for Executor {
    type Error = io::Error;
    fn capabilities(&self) -> DataServiceCapabilities {
        DataServiceCapabilities::default()
    }
}
impl QueryExecutor for Executor {
    async fn query(&self, request: QueryRequest) -> Result<QueryResult, Self::Error> {
        *self.projection.lock().unwrap() = request.query.projection;
        if self.input["failure"].as_bool().unwrap() {
            return Err(io::Error::other("provider failed"));
        }
        let mut rows = Vec::new();
        for index in 0..self.input["rows"].as_u64().unwrap() {
            let mut row = CompactRow::from_map(
                [
                    ("id".into(), Value::U64(index + 1)),
                    ("version".into(), Value::I64(1)),
                    ("label".into(), Value::Text(format!("retained-row-{index}"))),
                ]
                .into(),
            );
            if self.input["fail_model"] == index {
                row.insert("fail".into(), Value::Bool(true));
            }
            rows.push(row);
        }
        Ok(QueryResult {
            metadata: ExecutionMetadata::unrecorded_query(rows.len()),
            rows,
        })
    }
}
impl MutationExecutor for Executor {
    async fn mutate(&self, _: MutationRequest) -> Result<MutationResult, Self::Error> {
        panic!("unexpected aggregate mutation")
    }
}
struct NoWake;
impl Wake for NoWake {
    fn wake(self: Arc<Self>) {}
}
fn wait<F: Future>(future: F) -> F::Output {
    let mut future = std::pin::pin!(future);
    let waker = Waker::from(Arc::new(NoWake));
    let mut task = Context::from_waker(&waker);
    loop {
        if let Poll::Ready(result) = future.as_mut().poll(&mut task) {
            return result;
        }
    }
}
fn evaluate(input: J) -> J {
    let mut descriptor = EntityDescriptor::new("Parent");
    let mut id = PropertyDescriptor::new("id", DataType::U64);
    id.is_id = true;
    let mut version = PropertyDescriptor::new("version", DataType::I64);
    version.is_version = true;
    descriptor.properties = vec![
        id,
        version,
        PropertyDescriptor::new("label", DataType::Text),
    ];
    let mut user_context = RuntimeModule::new().descriptor(descriptor).into_context();
    user_context.disable_sql_log();
    let executor = Executor {
        input: input.clone(),
        projection: Mutex::new(vec![]),
    };
    let service = EntityDataService::for_executor(&user_context, "Parent", &executor);
    let mut query = SelectQuery::new("Parent");
    for field in input["projection"].as_array().unwrap() {
        query = query.project(field.as_str().unwrap());
    }
    if let Some(comment) = input["comment"].as_str() {
        query = query.comment(comment);
    }
    if let Some(purpose) = input["purpose"].as_str() {
        query = query.purpose(purpose);
    }
    if input["dynamic"].as_bool().unwrap_or(false) {
        query = query.select_dynamic_fields(teaql_core::dynamic_fields::DynamicFieldSelection::All);
    }
    let result = wait(service.__cpp_typed_aggregate::<Model>(&query, &[]));
    let outcome = match result {
        Ok(list) => json!({"rows":list.data.len(),"total":list.total_count,
            "loaded":list.data.iter().all(|model|model.state.is_some()),
            "ids":list.data.iter().map(|model|model.row.get("id").unwrap().try_u64().unwrap()).collect::<Vec<_>>()}),
        Err(error) => json!({"error":error.to_string()}),
    };
    json!({"projection":executor.projection.lock().unwrap().clone(),"outcome":outcome})
}
fn main() {
    for line in io::stdin().lock().lines() {
        println!(
            "{}",
            evaluate(serde_json::from_str(&line.unwrap()).unwrap())
        );
    }
}
