#[cfg(test)]
mod cpp_resolved_cache_intent_observer {
    use super::*;
    use serde_json::json;
    use std::sync::{Arc, Mutex};
    use std::future::Future;
    use std::task::{Context, Poll, Wake, Waker};
    use teaql_data_service::*;

    #[derive(Default)]
    struct Counts { get: usize, put: usize, query: usize }
    struct Cache { counts: Arc<Mutex<Counts>>, hit: bool }
    impl crate::AggregationCacheBackend for Cache {
        fn namespace(&self) -> &str { "intent-order" }
        fn get(&self, _: &str, _: u64) -> Option<Vec<CompactRow>> {
            self.counts.lock().unwrap().get += 1;
            self.hit.then(Vec::new)
        }
        fn put(&self, _: String, _: Vec<CompactRow>) { self.counts.lock().unwrap().put += 1; }
        fn invalidate_namespace(&self, _: &str) {}
    }
    struct Executor(Arc<Mutex<Counts>>);
    impl DataServiceExecutor for Executor {
        type Error = std::io::Error;
        fn capabilities(&self) -> DataServiceCapabilities { DataServiceCapabilities::default() }
    }
    impl QueryExecutor for Executor {
        async fn query(&self, _: QueryRequest) -> Result<QueryResult, Self::Error> {
            self.0.lock().unwrap().query += 1;
            Ok(QueryResult{rows:vec![],metadata:ExecutionMetadata::unrecorded_query(0)})
        }
    }
    impl MutationExecutor for Executor {
        async fn mutate(&self, _: MutationRequest) -> Result<MutationResult, Self::Error> {
            panic!("unexpected mutation")
        }
    }
    struct Noop;
    impl Wake for Noop { fn wake(self: Arc<Self>) {} }
    fn ready<T>(future: impl Future<Output=T>) -> T {
        let mut future=Box::pin(future);
        let waker=Waker::from(Arc::new(Noop));
        let mut context=Context::from_waker(&waker);
        match future.as_mut().poll(&mut context) {
            Poll::Ready(value)=>value,
            Poll::Pending=>panic!("ready fixture unexpectedly suspended"),
        }
    }
    #[test]
    fn cache_intent_order() {
        let mut case=0;
        for entry in ["borrowed", "owned", "compact"] {
            for hit in [false,true] {
                for intent in ["missing_comment","missing_purpose","valid"] {
                    let counts=Arc::new(Mutex::new(Counts::default()));
                    let mut context=crate::RuntimeModule::new().into_context();
                    context.insert_resource(Arc::new(Cache{counts:counts.clone(),hit}) as Arc<dyn crate::AggregationCacheBackend>);
                    let executor=Executor(counts.clone());
                    let service=EntityDataService::for_executor(&context,"Item",&executor);
                    let mut query=SelectQuery::new("Item").enable_aggregation_cache();
                    if intent!="missing_comment" { query.comment=Some("read cache boundary".into()); }
                    if intent!="missing_purpose" { query.purpose=Some("verify prepared intent order".into()); }
                    let result=match entry {
                        "borrowed"=>ready(service.fetch_prepared_query(&query)),
                        "owned"=>ready(service.fetch_prepared_query_owned(query)),
                        _=>ready(service.fetch_prepared_compact_owned(query)),
                    };
                    let counts=counts.lock().unwrap();
                    println!("RESOLVED_CACHE_INTENT_OBSERVATION {}",json!({"case":case,"entry":entry,"hit":hit,"intent":intent,
                        "error":result.as_ref().err().map(|e|e.to_string()),"rows":result.as_ref().ok().map(|v|v.len()),
                        "get":counts.get,"put":counts.put,"query":counts.query}));
                    case+=1;
                }
            }
        }
        assert_eq!(case,18);
    }
}
