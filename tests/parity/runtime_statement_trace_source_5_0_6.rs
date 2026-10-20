#[cfg(test)]
mod cpp_runtime_statement_trace_observer {
    use super::*;
    use serde_json::{Value as Json, json};
    use std::{future::Future, io, sync::{Arc, Mutex}, task::{Context, Poll, Wake, Waker}};
    use teaql_core::*;
    use teaql_data_service::*;

    #[path = "__TEAQL_FIXTURE_VALUE__"]
    mod fixture_value;

    struct Metadata;
    impl MetadataStore for Metadata {
        fn entity(&self, _: &str) -> Option<&EntityDescriptor> { None }
        fn all_entities(&self) -> Vec<&EntityDescriptor> { vec![] }
    }

    fn trace(input: &Json) -> Vec<TraceNode> {
        input.as_array().unwrap().iter().map(|frame| {
            let kind = match frame["kind"].as_str().unwrap() {
                "AuditReason" => TraceKind::AuditReason,
                "Entity" => TraceKind::Entity,
                "Comment" => TraceKind::Comment,
                _ => panic!("unexpected fixture trace kind"),
            };
            TraceNode::typed(kind, frame["entity"].as_str().unwrap(),
                frame["id"].as_u64(), frame["comment"].as_str().unwrap())
        }).collect()
    }

    fn trace_out(frames: &[TraceNode]) -> Json {
        json!(frames.iter().map(|frame| json!({
            "kind": format!("{:?}", frame.kind), "entity": frame.entity_type,
            "id": frame.entity_id, "comment": frame.comment,
        })).collect::<Vec<_>>())
    }

    #[derive(Default)]
    struct Executor(Mutex<Vec<Json>>);
    impl DataServiceExecutor for Executor {
        type Error = io::Error;
        fn capabilities(&self) -> DataServiceCapabilities { DataServiceCapabilities::default() }
    }
    impl QueryExecutor for Executor {
        async fn query(&self, _: QueryRequest) -> Result<QueryResult, Self::Error> {
            panic!("mutation must not query")
        }
    }
    impl MutationExecutor for Executor {
        async fn mutate(&self, request: MutationRequest) -> Result<MutationResult, Self::Error> {
            let frames = match &request.command {
                MutationCommand::Insert(c) => &c.trace_chain,
                MutationCommand::Update(c) => &c.trace_chain,
                MutationCommand::Delete(c) => &c.trace_chain,
                MutationCommand::Recover(c) => &c.trace_chain,
                _ => panic!("batch must be expanded by runtime"),
            };
            self.0.lock().unwrap().push(json!({
                "raw": trace_out(frames), "execution": trace_out(&request.execution_trace_chain()),
            }));
            Ok(MutationResult { affected_rows: 1, generated_values: GeneratedValues::new(),
                persisted_snapshot: None, metadata: ExecutionMetadata::unrecorded_query(0) })
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

    #[test]
    fn replay_physical_statement_identities() {
        let inputs: Vec<Json> = serde_json::from_str(include_str!("__TEAQL_TRACE_INPUTS__")).unwrap();
        for input in inputs {
            let executor = Executor::default();
            let metadata = Metadata;
            let service = RuntimeDataService::new(&metadata, &executor)
                .with_mutation_intent(MutationIntent::new("execute physical command").unwrap());
            let frames = trace(&input["trace"]);
            let id = fixture_value::value(&input["id"]);
            let mut insert = InsertCommand::new("Payment");
            if input["has_id"].as_bool().unwrap() { insert.values.insert("id".into(), id.clone()); }
            insert.trace_chain = frames.clone();
            let mut update = UpdateCommand::new("Payment", id.clone());
            update.trace_chain = frames.clone();
            let mut delete = DeleteCommand::new("Payment", id.clone());
            delete.trace_chain = frames.clone();
            let mut recover = RecoverCommand::new("Payment", id.clone(), 1);
            recover.trace_chain = frames.clone();
            let before = trace_out(&frames);
            let result = match input["entry"].as_str().unwrap() {
                "insert" => wait(service.insert(&insert)),
                "update" => wait(service.update(&update)),
                "delete" => wait(service.delete(&delete)),
                "recover" => wait(service.recover(&recover)),
                "inserts" => wait(service.insert_many(&[insert.clone(), insert.clone()])),
                "updates" => wait(service.update_many(&[update.clone(), update.clone()])),
                "deletes" => wait(service.delete_many(&[delete.clone(), delete.clone()])),
                "recovers" => wait(service.recover_many(&[recover.clone(), recover.clone()])),
                "batch_insert" => {
                    let mut batch = BatchInsertCommand::new("Payment");
                    batch.batch_values = vec![insert.values.clone(), insert.values.clone()];
                    batch.trace_chains = vec![frames.clone()];
                    wait(service.batch_insert(&batch))
                }
                "batch_update" => {
                    let mut batch = BatchUpdateCommand::new("Payment", vec![]);
                    batch.batch_ids = vec![id.clone(), id];
                    batch.batch_values = vec![MutationValues::new(), MutationValues::new()];
                    batch.trace_chains = vec![frames.clone()];
                    wait(service.batch_update(&batch))
                }
                _ => panic!("unexpected mutation entry"),
            }.unwrap();
            assert_eq!(trace_out(&insert.trace_chain), before);
            assert_eq!(trace_out(&update.trace_chain), before);
            assert_eq!(trace_out(&delete.trace_chain), before);
            assert_eq!(trace_out(&recover.trace_chain), before);
            println!("TEAQL_STATEMENT_TRACE_JSON {}", json!({
                "rows": result, "requests": *executor.0.lock().unwrap(), "input": before,
            }));
        }
    }
}
