#[cfg(test)]
mod cpp_runtime_mutation_protocol_observer {
    use super::*;
    use serde_json::{Value as Json, json};
    use std::{future::Future, io, panic::{catch_unwind, AssertUnwindSafe},
        sync::{Arc, Mutex}, task::{Context, Poll, Wake, Waker}};
    use teaql_core::*;
    use teaql_data_service::*;

    #[path = "__TEAQL_FIXTURE_VALUE__"]
    mod fixture_value;

    #[derive(Default)]
    struct State { notes: Mutex<Vec<String>>, logs: Mutex<Vec<u64>>, requests: Mutex<Vec<Json>> }
    impl State { fn note(&self, name: &str) { self.notes.lock().unwrap().push(name.into()); } }
    struct Spy<'a> { state: &'a State, observer: bool }
    impl MetadataStore for Spy<'_> {
        fn entity(&self, _: &str) -> Option<&EntityDescriptor> { None }
        fn all_entities(&self) -> Vec<&EntityDescriptor> { vec![] }
        fn capture_query_debug(&self) -> bool { panic!("mutation read query debug flag") }
        fn capture_execution_metadata(&self) -> bool { panic!("mutation read query capture flag") }
        fn record_metadata_log(&self, metadata: &ExecutionMetadata) {
            self.state.note("record_metadata");
            self.state.logs.lock().unwrap().push(metadata.affected_rows.unwrap_or(0));
        }
        fn mutation_diagnostic_observer(&self) -> Option<ExecutionObserver<'_>> {
            self.state.note("observer_factory");
            self.observer.then(|| Arc::new(move |metadata: ExecutionMetadata| {
                self.state.note("callback"); self.record_metadata_log(&metadata);
            }) as ExecutionObserver<'_>)
        }
    }
    fn record_out(values: &MutationValues) -> Json {
        json!(values.iter().map(|(k, v)| (k.clone(), format!("{v:?}")))
            .collect::<std::collections::BTreeMap<_, _>>())
    }
    fn request_out(request: &MutationRequest) -> Json {
        let mut result = match &request.command {
            MutationCommand::Insert(c) => json!({"kind":"insert", "entity":c.entity,
                "values":record_out(&c.values), "id":null, "version":null, "old":null}),
            MutationCommand::Update(c) => json!({"kind":"update", "entity":c.entity,
                "values":record_out(&c.values), "id":format!("{:?}", c.id),
                "version":c.expected_version, "old":c.old_values.as_ref().map(|v|
                    v.iter().map(|(k,v)| (k.clone(),format!("{v:?}"))).collect::<std::collections::BTreeMap<_,_>>())}),
            MutationCommand::Delete(c) => json!({"kind":"delete", "entity":c.entity,
                "values":null, "id":format!("{:?}", c.id), "version":c.expected_version, "old":null}),
            MutationCommand::Recover(c) => json!({"kind":"recover", "entity":c.entity,
                "values":null, "id":format!("{:?}", c.id), "version":c.expected_version, "old":null}),
            _ => panic!("runtime must expand batches"),
        };
        result["trace"] = json!(request.trace_chain().iter().map(|node| json!({
            "kind":format!("{:?}",node.kind), "entity":node.entity_type,
            "id":node.entity_id, "comment":node.comment,
        })).collect::<Vec<_>>());
        result["privacy"] = json!(request.diagnostic_redactions().map(SqlIntentRedactions::is_empty));
        result["comment"] = json!(request.comment());
        result
    }
    struct Executor<'a> { state: &'a State, counts: Vec<u64>, failure: i64, event: bool }
    impl DataServiceExecutor for Executor<'_> {
        type Error = io::Error;
        fn capabilities(&self) -> DataServiceCapabilities { DataServiceCapabilities::default() }
    }
    impl QueryExecutor for Executor<'_> {
        async fn query(&self, _: QueryRequest) -> Result<QueryResult, Self::Error> { panic!("mutation queried") }
    }
    impl MutationExecutor for Executor<'_> {
        async fn mutate(&self, request: MutationRequest) -> Result<MutationResult, Self::Error> {
            self.mutate_observed(request, None).await
        }
        async fn mutate_observed<'a>(&'a self, request: MutationRequest,
            observer: Option<ExecutionObserver<'a>>) -> Result<MutationResult, Self::Error> {
            let index = self.state.requests.lock().unwrap().len();
            self.state.requests.lock().unwrap().push(request_out(&request));
            self.state.note("provider");
            let count = self.counts.get(index).copied().unwrap_or(1);
            let mut metadata = ExecutionMetadata::unrecorded_query(0);
            metadata.affected_rows = Some(count);
            if self.event { if let Some(observer) = observer {
                self.state.note("observer"); observer(metadata.clone());
            }}
            if self.failure == index as i64 { return Err(io::Error::other("provider failed")); }
            Ok(MutationResult { affected_rows: count, generated_values: GeneratedValues::new(),
                persisted_snapshot: None, metadata })
        }
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
        let state = State::default();
        let spy = Spy { state: &state, observer: input["observer"].as_bool().unwrap() };
        let executor = Executor { state: &state, counts: input["counts"].as_array().unwrap().iter()
            .map(|v|v.as_u64().unwrap()).collect(), failure: input["failure"].as_i64().unwrap(),
            event: input["event"].as_bool().unwrap() };
        let mut service = RuntimeDataService::new(&spy, &executor);
        let id = fixture_value::value(&input["id"]);
        if input["intent"] == true {
            service = service.with_mutation_intent(MutationIntent::new("execute mutation protocol").unwrap());
            if input["privacy"] == true {
                let mut privacy = SqlIntentRedactions::default(); privacy.capture_target_id(&id);
                service.mutation_privacy = Some(Arc::new(privacy));
            }
        }
        let versions: Vec<Option<i64>> = input["versions"].as_array().unwrap().iter().map(Json::as_i64).collect();
        let frames = if input["trace"] == true { vec![
            TraceNode::typed(TraceKind::AuditReason,"Order",Some(99),"approve"),
            TraceNode::typed(TraceKind::Entity,"Payment",Some(123),"edit"),
        ] } else { vec![] };
        let payload = || MutationValues::from(std::collections::BTreeMap::from([
            ("id".into(),id.clone()), ("name".into(),Value::Text("new".into())),
            ("extra".into(),Value::U64(9)),
        ]));
        let old = || EntitySnapshot::from(std::collections::BTreeMap::from([
            ("name".into(),Value::Text("old".into())),
        ]));
        let n = input["length"].as_u64().unwrap() as usize;
        let m = input["ids"].as_u64().unwrap() as usize;
        let mut insert = InsertCommand::new("Payment"); insert.values = payload(); insert.trace_chain = frames.clone();
        let mut update = UpdateCommand::new("Payment",id.clone()); update.values = payload();
        update.trace_chain = frames.clone(); update.expected_version = versions.first().copied().flatten();
        if input["old"] == true { update.old_values = Some(old()); }
        let mut delete = DeleteCommand::new("Payment",id.clone()); delete.trace_chain = frames.clone();
        delete.expected_version = versions.first().copied().flatten();
        let mut recover = RecoverCommand::new("Payment",id.clone(),1); recover.trace_chain = frames.clone();
        let before = format!("{insert:?}{update:?}{delete:?}{recover:?}");
        let result = catch_unwind(AssertUnwindSafe(|| match input["entry"].as_str().unwrap() {
            "insert" => wait(service.insert(&insert)), "update" => wait(service.update(&update)),
            "delete" => wait(service.delete(&delete)), "recover" => wait(service.recover(&recover)),
            "inserts" => wait(service.insert_many(&vec![insert.clone();n])),
            "updates" => wait(service.update_many(&vec![update.clone();n])),
            "deletes" => wait(service.delete_many(&vec![delete.clone();n])),
            "recovers" => wait(service.recover_many(&vec![recover.clone();n])),
            "batch_insert" => {
                let mut batch = BatchInsertCommand::new("Payment"); batch.batch_values = (0..n).map(|_|payload()).collect();
                if input["trace"] == true { batch.trace_chains = vec![frames.clone()]; }
                wait(service.batch_insert(&batch))
            }
            "batch_update" => {
                let mut batch = BatchUpdateCommand::new("Payment",vec!["name".into(),"missing".into()]);
                batch.batch_ids = vec![id.clone();m];
                batch.batch_values = (0..n).map(|_|payload()).collect(); batch.batch_expected_versions = versions;
                if input["old"] == true { batch.batch_old_values = vec![Some(old())]; }
                if input["trace"] == true { batch.trace_chains = vec![frames.clone()]; }
                wait(service.batch_update(&batch))
            }
            _ => panic!("invalid fixture entry"),
        }));
        assert_eq!(format!("{insert:?}{update:?}{delete:?}{recover:?}"),before);
        let outcome = match result {
            Ok(Ok(rows)) => json!({"rows":rows}),
            Ok(Err(error)) => json!({"error":error.to_string(),"kind":match error {
                DataServiceError::Runtime(_) => "Runtime", DataServiceError::Executor(_) => "Executor",
                _ => panic!("mutation unexpectedly decoded entity"),
            }}),
            Err(_) => json!({"panic":true}),
        };
        json!({"outcome":outcome,"notes":*state.notes.lock().unwrap(),
            "logs":*state.logs.lock().unwrap(),"requests":*state.requests.lock().unwrap()})
    }
    #[test]
    fn replay_mutation_errors_batches_and_counters() {
        let inputs: Vec<Json> = serde_json::from_str(include_str!("__TEAQL_MUTATION_PROTOCOL_INPUTS__")).unwrap();
        let hook = std::panic::take_hook(); std::panic::set_hook(Box::new(|_|{}));
        for input in inputs { println!("TEAQL_MUTATION_PROTOCOL_JSON {}", evaluate(&input)); }
        std::panic::set_hook(hook);
    }
}
