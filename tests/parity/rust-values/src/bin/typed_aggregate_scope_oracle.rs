//! Compile the pinned private typed-aggregate body with controlled row input.
use teaql_runtime::EntityRuntimeState;
use std::io::{self, BufRead};

mod original {
    use super::EntityRuntimeState;
    use serde_json::{json, Value as J};
    use std::{future::Future, marker::PhantomData, sync::{Arc, Mutex}, task::{Context, Poll, Wake, Waker}};
    use teaql_core::*;
    use teaql_data_service::*;
    use teaql_runtime::{DataServiceError, RuntimeModule, UserContext};

    struct Metadata<'a> { context: &'a UserContext }
    struct DataService<'a> { metadata: Metadata<'a> }
    struct SourceRepo<'a, E: DataServiceExecutor> {
        data_service: DataService<'a>,
        rows: Vec<CompactRow>,
        projection: Mutex<Vec<String>>,
        failure: bool,
        error: fn() -> E::Error,
        executor: PhantomData<&'a E>,
    }
    impl<E: DataServiceExecutor> SourceRepo<'_, E> {
        async fn fetch_all_with_relation_aggregates_internal(&self, query: &SelectQuery, _: &[RelationAggregate]) -> Result<Vec<CompactRow>, DataServiceError<E::Error>> {
            *self.projection.lock().unwrap() = query.projection.clone();
            if self.failure { return Err(DataServiceError::Executor((self.error)())); }
            Ok(self.rows.clone())
        }
    }
    include!(concat!(env!("OUT_DIR"), "/typed_aggregate_scope.rs"));

    #[derive(Clone)]
    struct Model { row: CompactRow, state: Option<EntityRuntimeState> }
    impl TeaqlEntity for Model {
        const ENTITY_NAME: &'static str = "Parent";
        fn entity_descriptor() -> EntityDescriptor { EntityDescriptor::new(Self::ENTITY_NAME) }
    }
    impl Entity for Model {
        fn from_compact_row(row: CompactRow) -> Result<Self, EntityError> {
            if row.get("fail").and_then(Value::try_bool) == Some(true) {
                return Err(EntityError::new(Self::ENTITY_NAME, "model decode failed"));
            }
            Ok(Self { row, state: None })
        }
        fn into_values(self) -> MutationValues { self.row.into_map().into() }
        fn on_loaded(&mut self, context: &dyn std::any::Any) {
            if let Some(root) = context.downcast_ref::<EntityRuntimeState>() {
                let mut state = root.clone();
                state.set_original_compact_row(Self::ENTITY_NAME, self.row.clone());
                self.state = Some(state);
            }
        }
    }
    struct Executor;
    impl DataServiceExecutor for Executor {
        type Error = std::io::Error;
        fn capabilities(&self) -> DataServiceCapabilities { DataServiceCapabilities::default() }
    }
    impl QueryExecutor for Executor {
        async fn query(&self, _: QueryRequest) -> Result<QueryResult, Self::Error> { panic!("controlled row helper bypasses executor") }
    }
    impl MutationExecutor for Executor {
        async fn mutate(&self, _: MutationRequest) -> Result<MutationResult, Self::Error> { panic!("unused mutation") }
    }
    struct NoWake;
    impl Wake for NoWake { fn wake(self: Arc<Self>) {} }
    fn wait<F: Future>(future: F) -> F::Output {
        let mut future = std::pin::pin!(future);
        let waker = Waker::from(Arc::new(NoWake));
        let mut task = Context::from_waker(&waker);
        loop { if let Poll::Ready(result) = future.as_mut().poll(&mut task) { return result; } }
    }
    pub fn evaluate(input: J) -> J {
        let mut descriptor = EntityDescriptor::new("Parent");
        let mut id = PropertyDescriptor::new("id", DataType::U64); id.is_id = true;
        let mut version = PropertyDescriptor::new("version", DataType::I64); version.is_version = true;
        descriptor.properties = vec![id, version, PropertyDescriptor::new("label", DataType::Text)];
        let context = RuntimeModule::new().descriptor(descriptor).into_context();
        let mut rows = Vec::new();
        for index in 0..input["rows"].as_u64().unwrap() {
            let mut row = CompactRow::from_map([("id".into(), Value::U64(index + 1)), ("version".into(), Value::I64(1)), ("label".into(), Value::Text(format!("retained-row-{index}")))].into());
            if input["fail_model"] == index { row.insert("fail".into(), Value::Bool(true)); }
            rows.push(row);
        }
        let repo = SourceRepo::<Executor> { data_service: DataService { metadata: Metadata { context: &context } }, rows, projection: Mutex::new(vec![]), failure: input["failure"].as_bool().unwrap(), error: || std::io::Error::other("provider failed"), executor: PhantomData };
        let mut query = SelectQuery::new("Parent");
        for field in input["projection"].as_array().unwrap() { query = query.project(field.as_str().unwrap()); }
        let result = wait(repo.fetch_entities_with_relation_aggregates_internal::<Model>(&query, &[]));
        let outcome = match result {
            Ok(list) => json!({"rows":list.data.len(),"total":list.total_count,"loaded":list.data.iter().all(|model|model.state.is_some()),"ids":list.data.iter().map(|model|model.row.get("id").unwrap().try_u64().unwrap()).collect::<Vec<_>>()}),
            Err(error) => json!({"error":error.to_string()}),
        };
        json!({"projection":repo.projection.lock().unwrap().clone(),"outcome":outcome})
    }
}
fn main() {
    for line in io::stdin().lock().lines() {
        println!("{}", original::evaluate(serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
