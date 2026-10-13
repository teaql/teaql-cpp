//! Call the original public stream entry; no runtime orchestration is copied.
use futures_core::Stream;
use serde_json::{Value as J, json};
use std::{
    future::Future,
    io::{self, BufRead},
    pin::Pin,
    sync::{
        Arc, Mutex,
        atomic::{AtomicUsize, Ordering},
    },
    task::{Context, Poll, Wake, Waker},
};
use teaql_core::*;
use teaql_data_service::*;
use teaql_runtime::{
    EntityDataService, InMemoryAggregationCache, PurposedSelectQuery, RuntimeModule,
};

#[derive(Default)]
struct State {
    requests: Mutex<Vec<J>>,
    closes: AtomicUsize,
    pending: AtomicUsize,
}
struct Mock {
    state: Arc<State>,
    error_at: i64,
    last_first: bool,
}
impl DataServiceExecutor for Mock {
    type Error = io::Error;
    fn capabilities(&self) -> DataServiceCapabilities {
        DataServiceCapabilities::default()
    }
}
impl QueryExecutor for Mock {
    async fn query(&self, _: QueryRequest) -> Result<QueryResult, Self::Error> {
        panic!("streaming must not materialize rows")
    }
}
impl MutationExecutor for Mock {
    async fn mutate(&self, _: MutationRequest) -> Result<MutationResult, Self::Error> {
        panic!("unexpected mutation")
    }
}
fn trace(nodes: &[TraceNode]) -> J {
    json!(
        nodes
            .iter()
            .map(|n| json!({"kind": n.kind as u8,
        "entity": n.entity_type, "comment": n.comment}))
            .collect::<Vec<_>>()
    )
}
struct Cursor {
    state: Arc<State>,
    index: usize,
    pending: bool,
    error_at: i64,
    last_first: bool,
}
impl Stream for Cursor {
    type Item = Result<StreamChunk, io::Error>;
    fn poll_next(mut self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<Option<Self::Item>> {
        if self.pending {
            self.pending = false;
            self.state.pending.fetch_add(1, Ordering::Relaxed);
            cx.waker().wake_by_ref();
            return Poll::Pending;
        }
        if self.index == 3 {
            return Poll::Ready(None);
        }
        let index = self.index;
        self.index += 1;
        if index as i64 == self.error_at {
            return Poll::Ready(Some(Err(io::Error::other("cursor failed"))));
        }
        Poll::Ready(Some(Ok(StreamChunk {
            rows: vec![CompactRow::from_map(
                [("id".into(), Value::U64(index as u64 + 1))].into(),
            )],
            chunk_index: index,
            is_last: index == 2 || self.last_first,
        })))
    }
}
impl Drop for Cursor {
    fn drop(&mut self) {
        self.state.closes.fetch_add(1, Ordering::Relaxed);
    }
}
impl Mock {
    fn stream(&self, q: QueryRequest, size: usize, observed: bool) -> QueryStream<'_, io::Error> {
        self.state.requests.lock().unwrap().push(json!({"entity": q.query.entity,
            "projection": q.query.projection, "slice": q.query.slice.map(|s| json!([s.offset,s.limit])),
            "query_trace": trace(&q.query.trace_chain), "request_trace": trace(&q.trace_chain),
            "query_comment": q.query.comment, "comment": q.comment(),
            "capture": q.capture_execution_metadata, "debug": q.capture_debug_query,
            "size": size, "observed": observed}));
        Box::pin(Cursor {
            state: self.state.clone(),
            index: 0,
            pending: true,
            error_at: self.error_at,
            last_first: self.last_first,
        })
    }
}
impl StreamQueryExecutor for Mock {
    fn query_stream(&self, q: QueryRequest, size: usize) -> QueryStream<'_, io::Error> {
        self.stream(q, size, false)
    }
    fn query_stream_observed<'a>(
        &'a self,
        q: QueryRequest,
        size: usize,
        _: ExecutionObserver<'a>,
    ) -> QueryStream<'a, io::Error> {
        self.stream(q, size, true)
    }
}
struct NoWake;
impl Wake for NoWake {
    fn wake(self: Arc<Self>) {}
}
fn wait<F: Future>(future: F) -> F::Output {
    let mut future = std::pin::pin!(future);
    let waker = Waker::from(Arc::new(NoWake));
    let mut cx = Context::from_waker(&waker);
    loop {
        if let Poll::Ready(v) = future.as_mut().poll(&mut cx) {
            return v;
        }
    }
}
fn evaluate(input: &J) -> J {
    let state = Arc::new(State::default());
    let executor = Mock {
        state: state.clone(),
        error_at: input["error_at"].as_i64().unwrap(),
        last_first: input["last_first"].as_bool().unwrap(),
    };
    let mut item = EntityDescriptor::new("Item");
    let mut id = PropertyDescriptor::new("id", DataType::U64);
    id.is_id = true;
    item.properties.push(id);
    let mut relation = RelationDescriptor::new("children", "Item");
    relation.local_key = "id".into();
    relation.foreign_key = "id".into();
    relation.many = true;
    item.relations.push(relation);
    let mut context = RuntimeModule::new()
        .descriptor(item)
        .into_context()
        .with_user_identifier("reader");
    if !input["capture"].as_bool().unwrap() {
        context.disable_sql_log();
    }
    context.insert_resource(InMemoryAggregationCache::with_namespace(
        "unused-stream-cache",
    ));
    let repo = EntityDataService::for_executor(&context, "Item", &executor);
    let mut query = repo.select().enable_aggregation_cache().comment("business");
    if let Some(size) = input["size"].as_u64() {
        query.stream_config = Some(StreamConfig {
            chunk_size: size as usize,
        });
    }
    if let Some(limit) = input["limit"].as_u64() {
        query = query.limit(limit);
    }
    if let Some(limit) = input["hard_limit"].as_u64() {
        query = query.hard_limit(limit);
    }
    match input["shape"].as_str().unwrap() {
        "relation" => query = query.relation("children"),
        "child" => query.child_enhancements.push(SelectQuery::new("Item")),
        "object" => query.object_group_bys.push(ObjectGroupBy {
            property_name: "object".into(),
            storage_field: "id".into(),
            query: SelectQuery::new("Item"),
        }),
        "aggregate" => query = query.count("count"),
        "grouped" => query = query.group_by("id").count("count"),
        "continuous" => query = query.order_asc("id").optimize_for_continuous_page_fetch(),
        _ => {}
    }
    let wrapped = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
        PurposedSelectQuery::new(query, input["purpose"].as_str().unwrap())
    }));
    let mut output = json!({"items": []});
    match wrapped {
        Err(_) => output["purpose_panic"] = json!(true),
        Ok(query) => match wait(repo.fetch_stream(&query)) {
            Err(e) => output["start_error"] = json!(e.to_string()),
            Ok(mut stream) => {
                let waker = Waker::from(Arc::new(NoWake));
                let mut cx = Context::from_waker(&waker);
                loop {
                    match stream.as_mut().poll_next(&mut cx) {
                        Poll::Pending => {}
                        Poll::Ready(None) => break,
                        Poll::Ready(Some(Err(e))) => output["items"]
                            .as_array_mut()
                            .unwrap()
                            .push(json!({"error": e.to_string()})),
                        Poll::Ready(Some(Ok(c))) => {
                            output["items"].as_array_mut().unwrap().push(json!({
                            "index": c.chunk_index, "last": c.is_last,
                            "id": c.rows[0].get("id").unwrap().try_u64().unwrap()}))
                        }
                    }
                }
            }
        },
    }
    output["requests"] = json!(*state.requests.lock().unwrap());
    output["closes"] = json!(state.closes.load(Ordering::Relaxed));
    output["pending"] = json!(state.pending.load(Ordering::Relaxed));
    output
}
fn main() {
    std::panic::set_hook(Box::new(|_| {}));
    for line in io::stdin().lock().lines() {
        let input: J = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", evaluate(&input));
    }
}
