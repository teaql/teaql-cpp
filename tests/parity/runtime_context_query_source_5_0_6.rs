// Fixture-only caller for the actual complete Context/base/telemetry modules.
// The older fragment fixture is retained separately; no runtime type is shadowed.
#[cfg(test)]
mod cpp_runtime_context_query_observer {
    use super::*;
    use crate::{
        RuntimeAttributeValue, RuntimeOperation, RuntimeTelemetry,
        RuntimeTelemetryScope,
    };
    use serde_json::{Value as J, json};
    use std::{
        collections::BTreeMap,
        future::Future,
        io,
        pin::Pin,
        sync::{Arc, Mutex},
        task::{Context, Poll, Wake, Waker},
    };
    use teaql_core::*;
    use teaql_data_service::*;
    #[derive(Default)]
    struct State {
        notes: Mutex<Vec<String>>,
        active: Mutex<Vec<String>>,
        operations: Mutex<Vec<RuntimeOperation>>,
        request: Mutex<J>,
    }
    impl State {
        fn note(&self, s: String) {
            self.notes.lock().unwrap().push(s);
        }
        fn path(&self) -> String {
            let a = self.active.lock().unwrap();
            if a.is_empty() {
                "outside".into()
            } else {
                a.join("/")
            }
        }
    }
    struct Leave<'a>(&'a State, &'a str);
    impl Drop for Leave<'_> {
        fn drop(&mut self) {
            self.0.active.lock().unwrap().pop();
            self.0.note(format!("leave:{}", self.1));
        }
    }
    struct Scope {
        state: Arc<State>,
        family: String,
        config: J,
    }
    impl Drop for Scope {
        fn drop(&mut self) {
            self.state.note(format!("drop:{}", self.family));
        }
    }
    impl RuntimeTelemetryScope for Scope {
        fn with_context(&self, callback: &mut dyn FnMut()) {
            self.state.note(format!("enter:{}", self.family));
            self.state.active.lock().unwrap().push(self.family.clone());
            let _leave = Leave(&self.state, &self.family);
            let mode = self.config["activation"].as_str().unwrap();
            if mode == "before" {
                panic!("telemetry before")
            }
            if mode == "skip" {
                return;
            }
            if mode == "swallow" {
                let _ = std::panic::catch_unwind(std::panic::AssertUnwindSafe(callback));
                return;
            }
            callback();
            if mode == "twice" {
                callback()
            }
            if mode == "after" {
                panic!("telemetry after")
            }
        }
        fn success(&mut self, attrs: BTreeMap<String, RuntimeAttributeValue>) {
            assert!(attrs.is_empty());
            self.state.note(format!("success:{}", self.family));
            if self.config["finish_fail"] == true {
                panic!("finish")
            }
        }
        fn failure(&mut self, error: &str) {
            self.state.note(format!("failure:{}:{error}", self.family));
            if self.config["finish_fail"] == true {
                panic!("finish")
            }
        }
    }
    struct Telemetry {
        state: Arc<State>,
        config: J,
    }
    impl RuntimeTelemetry for Telemetry {
        fn is_noop(&self) -> bool {
            self.config["noop"] == true
        }
        fn start(&self, operation: RuntimeOperation) -> Box<dyn RuntimeTelemetryScope> {
            let family = operation.family.clone();
            self.state
                .note(format!("start:{family}:{}", self.state.path()));
            self.state.operations.lock().unwrap().push(operation);
            if self.config["start_fail"] == true {
                panic!("start")
            }
            Box::new(Scope {
                state: self.state.clone(),
                family,
                config: self.config.clone(),
            })
        }
    }
    struct DropWork(Arc<State>);
    impl Drop for DropWork {
        fn drop(&mut self) {
            self.0.note("work.drop".into());
        }
    }
    struct Delay {
        state: Arc<State>,
        pending: u64,
    }
    impl Future for Delay {
        type Output = ();
        fn poll(mut self: Pin<&mut Self>, cx: &mut Context<'_>) -> Poll<()> {
            self.state.note(format!("poll:{}", self.state.path()));
            if self.pending > 0 {
                self.pending -= 1;
                cx.waker().wake_by_ref();
                Poll::Pending
            } else {
                Poll::Ready(())
            }
        }
    }
    struct Mock {
        state: Arc<State>,
        config: J,
    }
    impl DataServiceExecutor for Mock {
        type Error = io::Error;
        fn capabilities(&self) -> DataServiceCapabilities {
            DataServiceCapabilities::default()
        }
    }
    impl QueryExecutor for Mock {
        async fn query(&self, q: QueryRequest) -> Result<QueryResult, Self::Error> {
            self.query_observed(q, None).await
        }
        async fn query_observed<'a>(
            &'a self,
            q: QueryRequest,
            observer: Option<ExecutionObserver<'a>>,
        ) -> Result<QueryResult, Self::Error> {
            let _drop = DropWork(self.state.clone());
            *self.state.request.lock().unwrap() = json!({"comment":q.comment(),"trace":q.trace_chain.len(),"capture":q.capture_execution_metadata,"debug":q.capture_debug_query});
            self.state.note(format!("request:{}", self.state.path()));
            if self.config["event"] == true {
                if let Some(observer) = observer {
                    observer(ExecutionMetadata::unrecorded_query(99));
                }
            }
            Delay {
                state: self.state.clone(),
                pending: self.config["pending"].as_u64().unwrap(),
            }
            .await;
            if self.config["failure"] == true {
                return Err(io::Error::other("fetch failed"));
            }
            Ok(QueryResult {
                rows: vec![CompactRow::from_map(
                    [
                        ("id".into(), Value::U64(1)),
                        ("fail".into(), Value::Bool(self.config["bad_model"] == true)),
                    ]
                    .into(),
                )],
                metadata: ExecutionMetadata::unrecorded_query(1),
            })
        }
    }
    impl MutationExecutor for Mock {
        async fn mutate(&self, _: MutationRequest) -> Result<MutationResult, Self::Error> {
            panic!("unused")
        }
    }
    struct Item;
    impl TeaqlEntity for Item {
        const ENTITY_NAME: &'static str = "Item";
        fn entity_descriptor() -> EntityDescriptor {
            EntityDescriptor::new("Item")
        }
    }
    impl Entity for Item {
        fn from_compact_row(row: CompactRow) -> Result<Self, EntityError> {
            if row.get("fail").and_then(Value::try_bool).unwrap_or(false) {
                Err(EntityError::new("Item", "handwritten model decode failed"))
            } else {
                Ok(Self)
            }
        }
        fn into_values(self) -> MutationValues {
            MutationValues::default()
        }
    }
    struct Noop;
    impl Wake for Noop {
        fn wake(self: Arc<Self>) {}
    }
    fn drive<T>(
        f: impl Future<Output = Result<T, DataServiceError<io::Error>>>,
        polls: u64,
        loaded: bool,
    ) -> J {
        let mut f = Box::pin(f);
        let waker = Waker::from(Arc::new(Noop));
        let mut cx = Context::from_waker(&waker);
        for _ in 0..polls {
            if let Poll::Ready(result) = f.as_mut().poll(&mut cx) {
                return match result {
                    Ok(_) => json!({"size":1,"loaded":loaded}),
                    Err(e) => json!({"error":e.to_string()}),
                };
            }
        }
        json!({"cancelled":true})
    }
    fn evaluate(j: &J) -> J {
        let state = Arc::new(State::default());
        let mut context = UserContext::new().with_runtime_telemetry(Arc::new(Telemetry {
            state: state.clone(),
            config: j.clone(),
        }));
        context.disable_sql_log();
        if j["capture"] == true {
            context.enable_select_sql_log()
        }
        context.set_user_identifier_option(j["user"].as_str().map(str::to_owned));
        let mock = Mock {
            state: state.clone(),
            config: j.clone(),
        };
        let repo = ContextDataService {
            metadata: UserContextMetadata { context: &context },
            executor: &mock,
            mutation_intent: None,
            mutation_privacy: None,
        };
        let mut query = SelectQuery::new("Item");
        query.comment = j["comment"].as_str().map(str::to_owned);
        query.purpose = j["purpose"].as_str().map(str::to_owned);
        if j["trace"] == true {
            query
                .trace_chain
                .push(TraceNode::typed(TraceKind::Request, "Item", None, "reason"));
            query.trace_chain.push(TraceNode::typed(
                TraceKind::Relation,
                "Child",
                Some(7),
                "nested",
            ));
        }
        let n = j["polls"].as_u64().unwrap();
        let mut out = match j["mode"].as_str().unwrap() {
            "all" => drive(repo.fetch_all(query), n, false),
            "list" => drive(repo.fetch_smart_list(&query), n, true),
            "entities" => drive(repo.fetch_entities::<Item>(&query), n, true),
            _ => drive(repo.fetch_enhanced_entities::<Item>(&query), n, true),
        };
        out["notes"] = json!(*state.notes.lock().unwrap());
        out["request"] = state.request.lock().unwrap().clone();
        out["logs"] = json!(
            context
                .sql_logs()
                .iter()
                .map(|entry| entry.result_count.unwrap_or(0))
                .collect::<Vec<_>>()
        );
        for operation in state.operations.lock().unwrap().iter() {
            if operation.family == "provider" {
                let kind = std::any::type_name::<Mock>();
                let expected_name = format!("{kind}.query");
                assert_eq!(operation.name, expected_name);
                assert!(
                    matches!(operation.attributes.get("teaql.provider.kind"),Some(RuntimeAttributeValue::String(v)) if v==kind)
                );
                assert!(
                    matches!(operation.attributes.get("teaql.provider.operation"),Some(RuntimeAttributeValue::String(v)) if v=="query")
                );
                assert!(
                    matches!(operation.attributes.get("teaql.operation.name"),Some(RuntimeAttributeValue::String(v)) if v==&expected_name)
                );
            }
        }
        out["operations"]=json!(state.operations.lock().unwrap().iter().map(|o|{let attrs=o.attributes.iter().map(|(k,v)|{let v=if k=="teaql.provider.kind"{json!("Mock")}else if k=="teaql.operation.name" && o.family=="provider"{json!("Mock.query")}else{match v{RuntimeAttributeValue::String(s)=>json!(s),RuntimeAttributeValue::Integer(n)=>json!(n),RuntimeAttributeValue::Float(n)=>json!(n),RuntimeAttributeValue::Boolean(v)=>json!(v)}};(k.clone(),v)}).collect::<BTreeMap<_,_>>();json!({"family":o.family,"name":if o.family=="provider"{"Mock.query"}else{&o.name},"attributes":attrs})}).collect::<Vec<_>>());
        out
    }
    #[test]
    fn replay_actual_context_query_and_telemetry() {
        let inputs: Vec<J> =
            serde_json::from_str(include_str!("__TEAQL_CONTEXT_QUERY_INPUTS__")).unwrap();
        let hook = std::panic::take_hook();
        std::panic::set_hook(Box::new(|_| {}));
        for input in inputs {
            println!("TEAQL_CONTEXT_QUERY_JSON {}", evaluate(&input));
        }
        std::panic::set_hook(hook);
    }
}
