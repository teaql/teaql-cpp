//! Public executor fixture: count metadata hooks and actual cursor lifetime.
use serde_json::{json, Value as J};
use std::{io::{self, BufRead}, pin::Pin, sync::{Arc, Mutex, atomic::{AtomicUsize, Ordering}}, task::{Context, Poll, Waker}};
use teaql_core::{DataType, EntityDescriptor, PropertyDescriptor, QueryIntent, SelectQuery};
use teaql_core::dynamic_fields::*;
use teaql_data_service::{DataServiceExecutor, ExecutionObserver, QueryExecutor, QueryRequest, QueryStream, SchemaProvider, StreamChunk, StreamQueryExecutor};
use teaql_data_service::dynamic_fields::*;
use teaql_sql::*;

#[derive(Default)]
struct State { kinds: AtomicUsize, lookups: AtomicUsize, opened: AtomicUsize, closed: AtomicUsize, size: Mutex<Option<usize>>, events: Mutex<Vec<J>> }
struct Dialect<'a> { state: &'a State, fail_after: Option<usize> }
impl SqlDialect for Dialect<'_> {
    fn kind(&self) -> DatabaseKind {
        let n = self.state.kinds.fetch_add(1, Ordering::Relaxed) + 1;
        assert!(!self.fail_after.is_some_and(|limit| n > limit), "controlled kind failure");
        DatabaseKind::Sqlite
    }
    fn quote_ident(&self, name: &str) -> String { format!("\"{name}\"") }
    fn placeholder(&self, _: usize) -> String { "?".into() }
}
struct Schema<'a>(&'a State);
impl SchemaProvider for Schema<'_> {
    fn get_entity(&self, name: &str) -> Option<Arc<EntityDescriptor>> {
        self.0.lookups.fetch_add(1, Ordering::Relaxed);
        (name == "Order").then(|| Arc::new(EntityDescriptor::new("Order")
            .property(PropertyDescriptor::new("id", DataType::U64).id())))
    }
}
struct Store;
impl DynamicFieldStore for Store {
    fn is_transaction_bound(&self) -> bool { false }
    fn binding_key(&self) -> usize { 17 }
    fn source_identity(&self) -> Option<u64> { Some(71) }
    fn ensure_schema(&self) -> DynamicStorageFuture<'_, ()> { unreachable!() }
    fn load_values<'a>(&'a self, _: &'a str, _: &'a DynamicFieldDefinitions, _: &'a [u64], _: &'a DynamicFieldSelection, _: &'a QueryIntent) -> DynamicStorageFuture<'a, DynamicStorageRows> { unreachable!() }
    fn validate_writes(&self, _: &[DynamicFieldWrite]) -> Result<(), DynamicFieldError> { unreachable!() }
    fn apply_writes<'a>(&'a self, _: &'a [DynamicFieldWrite], _: &'a teaql_core::MutationIntent) -> DynamicStorageFuture<'a, ()> { unreachable!() }
}
struct Cursor<'a>(&'a State);
impl futures_core::Stream for Cursor<'_> {
    type Item = Result<StreamChunk, io::Error>;
    fn poll_next(self: Pin<&mut Self>, _: &mut Context<'_>) -> Poll<Option<Self::Item>> { Poll::Ready(None) }
}
impl Drop for Cursor<'_> { fn drop(&mut self) { self.0.closed.fetch_add(1, Ordering::Relaxed); } }
struct PlainTransport<'a>(&'a State);
impl SqlTransport for PlainTransport<'_> {
    type Error = io::Error;
    async fn fetch_all_compact_sql(&self, _: &CompiledQuery) -> Result<Vec<teaql_core::CompactRow>, io::Error> { unreachable!() }
    async fn execute_sql(&self, _: &CompiledQuery) -> Result<u64, io::Error> { unreachable!() }
}
impl StreamingSqlTransport for PlainTransport<'_> {
    fn stream_sql(&self, _: CompiledQuery, size: usize) -> QueryStream<'_, io::Error> {
        self.0.opened.fetch_add(1, Ordering::Relaxed);
        *self.0.size.lock().unwrap() = Some(size);
        Box::pin(Cursor(self.0))
    }
}
struct SupportedTransport<'a>(PlainTransport<'a>, Store);
impl SqlTransport for SupportedTransport<'_> {
    type Error = io::Error;
    fn dynamic_field_store(&self) -> Option<&dyn DynamicFieldStore> { Some(&self.1) }
    async fn fetch_all_compact_sql(&self, _: &CompiledQuery) -> Result<Vec<teaql_core::CompactRow>, io::Error> { unreachable!() }
    async fn execute_sql(&self, _: &CompiledQuery) -> Result<u64, io::Error> { unreachable!() }
}
impl StreamingSqlTransport for SupportedTransport<'_> {
    fn stream_sql(&self, query: CompiledQuery, size: usize) -> QueryStream<'_, io::Error> { self.0.stream_sql(query, size) }
    fn stream_sql_with_dynamic_fields(&self, query: CompiledQuery, size: usize, _: DynamicFieldStreamPlan) -> Result<QueryStream<'_, io::Error>, DynamicFieldError> { Ok(self.stream_sql(query, size)) }
}

fn run<T: StreamingSqlTransport<Error=io::Error>>(input: &J, state: &State, transport: T) -> J {
    let dialect = Dialect { state, fail_after: input["fail_after"].as_u64().map(|n| n as usize) };
    let executor = SqlDataServiceExecutor::new(dialect, transport, Schema(state));
    let capabilities = executor.capabilities();
    let mut out = json!({"store": executor.dynamic_field_store().is_some(),
        "capabilities": [capabilities.query,capabilities.mutation,capabilities.transaction,capabilities.schema,capabilities.id_generation,capabilities.batch_mutation,capabilities.returning,capabilities.small_parent_relation_probes],
        "opened_result":false,"items":[]});
    let mode = input["gate"].as_str().unwrap();
    let owner = if mode == "unknown" { "Unknown" } else { "Order" };
    let definitions = DynamicFieldDefinitions::new(if mode == "owner" { "Other" } else { owner }, "r1", [("extra".into(),DataType::Text)]).unwrap();
    let intent = QueryIntent::new("Inspect dynamic SQL stream", "Verify executor diagnostics").unwrap();
    let plan = DynamicFieldStreamPlan::new("fixture".into(), definitions, DynamicFieldSelection::All, &Store, intent.clone()).unwrap();
    let mut query = SelectQuery::new(owner).limit(1);
    if mode != "selection" { query.dynamic_field_selection = Some(Arc::new(DynamicFieldSelection::All)); }
    if mode == "compile" { query = query.project_expr("bad", teaql_core::Expr::Function { function:teaql_core::ExprFunction::Soundex, args:vec![] }); }
    let request = QueryRequest { trace_chain:vec![],intent,query,capture_debug_query:false,capture_execution_metadata:input["capture"]==true };
    let observer = (input["observe"]==true).then(|| Arc::new(|metadata:teaql_data_service::ExecutionMetadata| {
        state.events.lock().unwrap().push(json!({"outcome":metadata.sql_log.execution_outcome.map(|v|v as usize),"count":metadata.result_count,"closed_before":state.opened.load(Ordering::Relaxed)==state.closed.load(Ordering::Relaxed)}));
    }) as ExecutionObserver<'_>);
    let result = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
        match executor.query_stream_with_dynamic_fields(request,input["size"].as_u64().unwrap() as usize,plan,observer) {
            Err(error) => out["error"]=json!(error.to_string()),
            Ok(mut cursor) => {
                out["opened_result"]=json!(true);
                if input["poll"]==true {
                    let mut items=vec![];
                    while let Poll::Ready(Some(item)) = cursor.as_mut().poll_next(&mut Context::from_waker(Waker::noop())) {
                        items.push(match item {Err(error)=>json!({"error":error.to_string()}),Ok(chunk)=>json!({"rows":chunk.rows.len()})});
                    }
                    out["items"]=json!(items);
                }
                drop(cursor);
            }
        }
    }));
    if result.is_err() { out["panic"]=json!(true); }
    out["kinds"]=json!(state.kinds.load(Ordering::Relaxed));
    out["lookups"]=json!(state.lookups.load(Ordering::Relaxed));
    out["opened"]=json!(state.opened.load(Ordering::Relaxed));
    out["closed"]=json!(state.closed.load(Ordering::Relaxed));
    out["size"]=json!(*state.size.lock().unwrap());
    out["events"]=json!(*state.events.lock().unwrap());
    out
}
fn main() {
    std::panic::set_hook(Box::new(|_|{}));
    for line in io::stdin().lock().lines() {
        let input:J=serde_json::from_str(&line.unwrap()).unwrap();
        let state=State::default();
        let out=if input["support"]==true {run(&input,&state,SupportedTransport(PlainTransport(&state),Store))} else {run(&input,&state,PlainTransport(&state))};
        println!("{out}");
    }
}
