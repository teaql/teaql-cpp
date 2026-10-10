//! Call the pinned executor defaults; the fixture implements mandatory SPIs only.
use futures_core::Stream;
use serde_json::{json, Value as Json};
use std::{future::{Future, poll_fn}, io::{self, BufRead}, pin::Pin,
    sync::{Arc, Mutex, atomic::{AtomicUsize, Ordering}}, task::{Context, Poll, Waker}};
use teaql_core::{Expr, InsertCommand, SelectQuery, TraceKind, TraceNode};
use teaql_data_service::*;

#[derive(Default)]
struct State {
    calls: AtomicUsize,
    polls: AtomicUsize,
    notifications: AtomicUsize,
    chunk_size: AtomicUsize,
    requests: Mutex<Vec<String>>,
}
struct Provider(Arc<State>);
impl Provider {
    fn record(&self, value: &impl std::fmt::Debug) {
        self.0.calls.fetch_add(1, Ordering::SeqCst);
        self.0.requests.lock().unwrap().push(format!("{value:?}"));
    }
    fn failure<T>(&self) -> impl Future<Output=Result<T, io::Error>> + Send + use<T> {
        let state = self.0.clone();
        poll_fn(move |_| {
            state.polls.fetch_add(1, Ordering::SeqCst);
            Poll::Ready(Err(io::Error::other("fixture failure")))
        })
    }
}
impl DataServiceExecutor for Provider {
    type Error = io::Error;
    fn capabilities(&self) -> DataServiceCapabilities { DataServiceCapabilities::default() }
}
impl QueryExecutor for Provider {
    fn query(&self, request: QueryRequest) -> impl Future<Output=Result<QueryResult, io::Error>> + Send {
        self.record(&request);
        self.failure()
    }
}
impl MutationExecutor for Provider {
    fn mutate(&self, request: MutationRequest) -> impl Future<Output=Result<MutationResult, io::Error>> + Send {
        self.record(&request);
        self.failure()
    }
}
impl GuardedMutationExecutor for Provider {
    fn mutate_guarded(&self, request: GuardedMutationRequest) -> impl Future<Output=Result<MutationResult, io::Error>> + Send {
        self.record(&request);
        self.failure()
    }
}
struct EmptyStream(Arc<State>);
impl Stream for EmptyStream {
    type Item = Result<StreamChunk, io::Error>;
    fn poll_next(self: Pin<&mut Self>, _: &mut Context<'_>) -> Poll<Option<Self::Item>> {
        self.0.polls.fetch_add(1, Ordering::SeqCst);
        Poll::Ready(None)
    }
}
impl StreamQueryExecutor for Provider {
    fn query_stream(&self, request: QueryRequest, chunk_size: usize) -> QueryStream<'_, io::Error> {
        self.record(&request);
        self.0.chunk_size.store(chunk_size, Ordering::SeqCst);
        Box::pin(EmptyStream(self.0.clone()))
    }
}
fn counters(state: &State) -> Json {
    json!({"calls":state.calls.load(Ordering::SeqCst),"polls":state.polls.load(Ordering::SeqCst),
        "notifications":state.notifications.load(Ordering::SeqCst),
        "chunk_size":state.chunk_size.load(Ordering::SeqCst)})
}
fn evaluate(input: Json) -> Json {
    let name = input["name"].as_str().unwrap();
    let flags = input["flags"].as_u64().unwrap();
    let state = Arc::new(State::default());
    let provider = Provider(state.clone());
    let observed = state.clone();
    let observer: ExecutionObserver<'_> = Arc::new(move |_| {
        observed.notifications.fetch_add(1, Ordering::SeqCst);
    });
    let optional = if input["observer"].as_bool().unwrap() {Some(observer.clone())} else {None};
    let request = QueryRequest {query:SelectQuery::new(name).comment(name),trace_chain:vec![],
        comment:None,capture_debug_query:flags&1!=0,capture_execution_metadata:flags&2!=0};
    let mut insert = InsertCommand::new(name);
    insert.trace_chain.push(TraceNode::typed(TraceKind::Comment,name,None,name));
    let mutation = MutationRequest::Insert(insert);
    let query = provider.query_observed(request.clone(), optional.clone());
    let mut stream = provider.query_stream_observed(request, input["chunk_size"].as_u64().unwrap() as usize, observer);
    let mutate = provider.mutate_observed(mutation.clone(), optional.clone());
    let guard = provider.mutate_guarded_observed(GuardedMutationRequest::new(mutation, Expr::eq("owner",7u64)), optional);
    let before = counters(&state);
    let mut results = vec![];
    if input["poll"].as_bool().unwrap() {
        let mut cx = Context::from_waker(Waker::noop());
        results.push(matches!(std::pin::pin!(query).poll(&mut cx), Poll::Ready(Err(_))));
        results.push(matches!(stream.as_mut().poll_next(&mut cx), Poll::Ready(None)));
        results.push(matches!(std::pin::pin!(mutate).poll(&mut cx), Poll::Ready(Err(_))));
        results.push(matches!(std::pin::pin!(guard).poll(&mut cx), Poll::Ready(Err(_))));
    }
    json!({"before":before,"after":counters(&state),"results":results,
        "requests":*state.requests.lock().unwrap()})
}
fn main() {
    for line in io::stdin().lock().lines() {
        println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
