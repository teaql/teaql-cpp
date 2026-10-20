// Access-only caller appended to the complete original Context module.
#[cfg(test)]
mod cpp_context_helpers_observer {
    use super::*;
    use crate::{RuntimeOperation, RuntimeTelemetry, RuntimeTelemetryScope};
    use serde_json::{Value as Json, json};
    use std::{
        future::Future, io, pin::Pin,
        sync::{Arc, Mutex, atomic::{AtomicUsize, Ordering}},
        task::{Context, Poll, Wake, Waker},
    };
    use teaql_core::*;
    use teaql_data_service::*;

    struct Executor;
    impl DataServiceExecutor for Executor {
        type Error = io::Error;
        fn capabilities(&self) -> DataServiceCapabilities { DataServiceCapabilities::default() }
    }
    impl QueryExecutor for Executor {
        async fn query(&self, _: QueryRequest) -> Result<QueryResult, Self::Error> {
            panic!("unexpected query");
        }
    }
    impl MutationExecutor for Executor {
        async fn mutate(&self, _: MutationRequest) -> Result<MutationResult, Self::Error> {
            panic!("unexpected mutation");
        }
    }
    struct Scope;
    impl RuntimeTelemetryScope for Scope {
        fn success(&mut self, _: BTreeMap<String, crate::RuntimeAttributeValue>) {}
        fn failure(&mut self, error: &str) { assert_eq!(error, "data_service_error"); }
    }
    struct Telemetry { noop: bool, families: Arc<Mutex<Vec<String>>> }
    impl RuntimeTelemetry for Telemetry {
        fn is_noop(&self) -> bool { self.noop }
        fn start(&self, operation: RuntimeOperation) -> Box<dyn RuntimeTelemetryScope> {
            self.families.lock().unwrap().push(operation.family);
            Box::new(Scope)
        }
    }
    struct NoWake;
    impl Wake for NoWake { fn wake(self: Arc<Self>) {} }
    struct Work { polls: Arc<AtomicUsize>, pending: u64, failure: bool }
    impl Future for Work {
        type Output = Result<u64, DataServiceError<io::Error>>;
        fn poll(mut self: Pin<&mut Self>, context: &mut Context<'_>) -> Poll<Self::Output> {
            self.polls.fetch_add(1, Ordering::SeqCst);
            if self.pending > 0 {
                self.pending -= 1;
                context.waker().wake_by_ref();
                Poll::Pending
            } else if self.failure {
                Poll::Ready(Err(DataServiceError::Executor(io::Error::other("work failed"))))
            } else { Poll::Ready(Ok(7)) }
        }
    }
    fn kind(value: &str) -> TraceKind {
        match value {
            "Operation" => TraceKind::Operation, "Request" => TraceKind::Request,
            "Relation" => TraceKind::Relation, "Entity" => TraceKind::Entity,
            "Provider" => TraceKind::Provider, "Sql" => TraceKind::Sql,
            "Comment" => TraceKind::Comment, "Purpose" => TraceKind::Purpose,
            "AuditReason" => TraceKind::AuditReason, _ => panic!("invalid fixture kind"),
        }
    }
    fn evaluate(input: &Json) -> Json {
        let families = Arc::new(Mutex::new(vec![]));
        let mut user_context = UserContext::new().with_runtime_telemetry(Arc::new(Telemetry {
            noop: input["noop"] == true, families: families.clone(),
        }));
        user_context.disable_sql_log();
        user_context.set_user_identifier_option(input["user"].as_str().map(str::to_owned));
        let executor = Executor;
        let repository = ContextDataService {
            metadata: UserContextMetadata { context: &user_context }, executor: &executor,
            mutation_intent: None, mutation_privacy: None,
        };
        if input["mode"] == "comment" {
            let trace: Vec<_> = input["trace"].as_array().unwrap().iter().map(|node| {
                TraceNode::typed(kind(node["kind"].as_str().unwrap()),
                                 node["entity"].as_str().unwrap(),
                                 node["id"].as_str().map(|v|v.parse::<u64>().unwrap()),
                                 node["comment"].as_str().unwrap())
            }).collect();
            return json!({"comment":repository.resolve_final_comment(
                &trace, input["comment"].as_str().map(str::to_owned))});
        }
        let names = AtomicUsize::new(0);
        let work_polls = Arc::new(AtomicUsize::new(0));
        let work = Work { polls: work_polls.clone(), pending: input["pending"].as_u64().unwrap(),
                          failure: input["failure"] == true };
        // The moved token ensures this caller supplies an actual FnOnce closure.
        let token = String::from("custom helper");
        let name = || {
            names.fetch_add(1, Ordering::SeqCst);
            if input["name_panic"] == true { panic!("name failed"); }
            token
        };
        let future = repository.observe("custom", name, "Item", work);
        let mut output = json!({"names_before_poll":names.load(Ordering::SeqCst)});
        let result = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
            let mut future = std::pin::pin!(future);
            let waker = Waker::from(Arc::new(NoWake));
            let mut context = Context::from_waker(&waker);
            for _ in 0..input["polls"].as_u64().unwrap() {
                if let Poll::Ready(result) = future.as_mut().poll(&mut context) {
                    match result {
                        Ok(value) => output["value"] = json!(value),
                        Err(error) => output["error"] = json!(error.to_string()),
                    }
                    output["cancelled"] = json!(false);
                    return;
                }
            }
            output["cancelled"] = json!(true);
        }));
        output["panic"] = json!(result.is_err());
        output["names"] = json!(names.load(Ordering::SeqCst));
        output["work_polls"] = json!(work_polls.load(Ordering::SeqCst));
        output["families"] = json!(*families.lock().unwrap());
        output
    }
    #[test]
    fn replay_comment_folding_and_fn_once_names() {
        let inputs: Vec<Json> = serde_json::from_str(include_str!("__TEAQL_CONTEXT_HELPERS_INPUTS__")).unwrap();
        let hook = std::panic::take_hook();
        std::panic::set_hook(Box::new(|_|{}));
        for input in inputs {
            println!("TEAQL_CONTEXT_HELPERS_JSON {}", evaluate(&input));
        }
        std::panic::set_hook(hook);
    }
}
