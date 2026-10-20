#[cfg(test)]
mod cpp_runtime_query_protocol_observer {
    use super::*;
    use serde_json::{Value as Json, json};
    use std::{future::Future, io, sync::{Arc, Mutex}, task::{Context, Poll, Wake, Waker}};
    use teaql_core::*;
    use teaql_data_service::*;

    struct Spy {
        capture: bool,
        debug: bool,
        protocol: bool,
        observer: bool,
        logs: Mutex<Vec<usize>>,
        notes: Mutex<Vec<String>>,
    }

    impl Spy {
        fn note(&self, name: &str) {
            if self.protocol { self.notes.lock().unwrap().push(name.into()); }
        }
    }

    impl MetadataStore for Spy {
        fn entity(&self, _: &str) -> Option<&EntityDescriptor> { None }
        fn all_entities(&self) -> Vec<&EntityDescriptor> { vec![] }
        fn capture_execution_metadata(&self) -> bool {
            self.note("capture_metadata");
            self.capture
        }
        fn capture_query_debug(&self) -> bool {
            self.note("capture_debug");
            self.debug
        }
        fn record_metadata_log(&self, metadata: &ExecutionMetadata) {
            self.note("record_metadata");
            self.logs.lock().unwrap().push(metadata.result_count.unwrap_or(0));
        }
        fn query_diagnostic_observer(&self) -> Option<ExecutionObserver<'_>> {
            // Custom observer policy is independent of capture flags. Legacy
            // vectors retain the default trait policy and output surface.
            if self.protocol {
                self.note("query_observer");
                if !self.observer { return None; }
            } else if !self.capture_execution_metadata() {
                return None;
            }
            Some(Arc::new(move |metadata| {
                self.note("callback");
                self.record_metadata_log(&metadata);
            }))
        }
    }

    struct Executor<'a> {
        spy: &'a Spy,
        failure: bool,
        bad_model: bool,
        event: bool,
        seen: Mutex<Json>,
    }

    impl DataServiceExecutor for Executor<'_> {
        type Error = io::Error;
        fn capabilities(&self) -> DataServiceCapabilities { DataServiceCapabilities::default() }
    }
    impl QueryExecutor for Executor<'_> {
        async fn query(&self, request: QueryRequest) -> Result<QueryResult, Self::Error> {
            self.query_observed(request, None).await
        }
        async fn query_observed<'a>(&'a self, mut request: QueryRequest,
            observer: Option<ExecutionObserver<'a>>) -> Result<QueryResult, Self::Error> {
            self.spy.note("provider");
            let mut seen = json!({"entity": request.query.entity,
                "trace": request.trace_chain.len(), "comment": request.comment(),
                "capture": request.capture_execution_metadata,
                "debug": request.capture_debug_query, "observer": observer.is_some()});
            if self.spy.protocol {
                seen["purpose"] = json!(request.purpose());
                request.query.comment = None;
                request.query.trace_chain.clear();
            }
            *self.seen.lock().unwrap() = seen;
            if self.event { if let Some(observer) = observer {
                observer(ExecutionMetadata::unrecorded_query(99));
            }}
            if self.failure { return Err(io::Error::other("fetch failed")); }
            Ok(QueryResult {
                rows: vec![CompactRow::from_map([
                    ("id".into(), Value::U64(1)),
                    ("fail".into(), Value::Bool(self.bad_model)),
                ].into())],
                metadata: ExecutionMetadata::unrecorded_query(1),
            })
        }
    }
    impl MutationExecutor for Executor<'_> {
        async fn mutate(&self, _: MutationRequest) -> Result<MutationResult, Self::Error> {
            panic!("query must not mutate")
        }
    }

    struct Item(CompactRow);
    impl TeaqlEntity for Item {
        const ENTITY_NAME: &'static str = "Item";
        fn entity_descriptor() -> EntityDescriptor { EntityDescriptor::new("Item") }
    }
    impl Entity for Item {
        fn from_compact_row(row: CompactRow) -> Result<Self, EntityError> {
            if row.get("fail").and_then(Value::try_bool).unwrap_or(false) {
                Err(EntityError::new("Item", "handwritten model decode failed"))
            } else { Ok(Self(row)) }
        }
        fn into_values(self) -> MutationValues { self.0.into_map().into() }
    }

    struct NoWake;
    impl Wake for NoWake { fn wake(self: Arc<Self>) {} }
    fn wait<F: Future>(future: F) -> F::Output {
        let mut future = std::pin::pin!(future);
        let waker = Waker::from(Arc::new(NoWake));
        let mut context = Context::from_waker(&waker);
        match future.as_mut().poll(&mut context) {
            Poll::Ready(result) => result,
            Poll::Pending => panic!("immediate executor unexpectedly suspended"),
        }
    }

    fn evaluate(input: &Json) -> Json {
        let spy = Spy { capture: input["capture"].as_bool().unwrap(),
            debug: input["debug"].as_bool().unwrap(),
            protocol: input["protocol"].as_bool().unwrap_or(false),
            observer: input["observer"].as_bool().unwrap_or(false),
            logs: Mutex::new(vec![]), notes: Mutex::new(vec![]) };
        let executor = Executor { spy: &spy, failure: input["failure"].as_bool().unwrap(),
            bad_model: input["bad_model"].as_bool().unwrap(), event: input["event"].as_bool().unwrap(),
            seen: Mutex::new(Json::Null) };
        let service = RuntimeDataService::new(&spy, &executor);
        let mut query = SelectQuery::new("Item");
        query.comment = if input.get("comment").is_some() {
            input["comment"].as_str().map(str::to_owned)
        } else { Some("comment".into()) };
        query.purpose = input["purpose"].as_str().map(str::to_owned);
        if input["trace"] == true {
            query.trace_chain.push(TraceNode::typed(TraceKind::Request, "Item", None, "trace"));
        }
        let result: Result<(Vec<String>, bool), DataServiceError<io::Error>> =
            match input["mode"].as_str().unwrap() {
                "all" => wait(service.fetch_all(&query)).map(|rows| (
                    rows.iter().map(|row| format!("{:?}", row.get("id").unwrap())).collect(), false)),
                "list" => wait(service.fetch_smart_list(&query)).map(|rows| (
                    rows.as_slice().iter().map(|row| format!("{:?}", row.get("id").unwrap())).collect(), rows.is_loaded)),
                mode => {
                    let result = if mode == "entities" { wait(service.fetch_entities::<Item>(&query)) }
                        else { wait(service.fetch_enhanced_entities::<Item>(&query)) };
                    result.map(|rows| (rows.as_slice().iter()
                        .map(|row| format!("{:?}", row.0.get("id").unwrap())).collect(), rows.is_loaded))
                }
            };
        let mut output = match result {
            Ok((rows, loaded)) => json!({"rows": rows, "loaded": loaded}),
            Err(error) => json!({"error": error.to_string()}),
        };
        output["logs"] = json!(*spy.logs.lock().unwrap());
        output["request"] = executor.seen.lock().unwrap().clone();
        if spy.protocol {
            output["notes"] = json!(*spy.notes.lock().unwrap());
            output["input_comment"] = json!(query.comment);
            output["input_purpose"] = json!(query.purpose);
            output["input_trace"] = json!(query.trace_chain.len());
        }
        output
    }

    #[test]
    fn replay_real_query_methods_and_metadata_order() {
        let inputs: Vec<Json> = serde_json::from_str(include_str!("__TEAQL_QUERY_PROTOCOL_INPUTS__")).unwrap();
        for input in inputs {
            println!("TEAQL_QUERY_PROTOCOL_JSON {}", evaluate(&input));
        }
    }
}
