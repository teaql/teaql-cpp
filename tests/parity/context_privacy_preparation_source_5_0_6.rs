#[cfg(test)]
mod cpp_context_privacy_preparation_observer {
    use super::*;
    use crate::{GraphMutationKind, GraphMutationPlan, InMemoryMetadataStore, UserContext};
    use serde_json::{Value as Json, json};
    use std::{
        future::Future,
        io,
        sync::{Arc, Mutex},
        task::{Context, Poll, Wake, Waker},
    };
    use teaql_core::*;
    use teaql_data_service::*;
    struct Executor(Mutex<Vec<Option<bool>>>);
    impl DataServiceExecutor for Executor {
        type Error = io::Error;
        fn capabilities(&self) -> DataServiceCapabilities {
            DataServiceCapabilities::default()
        }
    }
    impl QueryExecutor for Executor {
        async fn query(&self, _: QueryRequest) -> Result<QueryResult, Self::Error> {
            panic!("unexpected query")
        }
    }
    impl MutationExecutor for Executor {
        async fn mutate(&self, request: MutationRequest) -> Result<MutationResult, Self::Error> {
            self.0.lock().unwrap().push(
                request
                    .diagnostic_redactions()
                    .map(SqlIntentRedactions::is_empty),
            );
            Ok(MutationResult {
                affected_rows: 1,
                generated_values: GeneratedValues::new(),
                persisted_snapshot: None,
                metadata: ExecutionMetadata::unrecorded_query(0),
            })
        }
    }
    struct NoWake;
    impl Wake for NoWake {
        fn wake(self: Arc<Self>) {}
    }
    fn wait<F: Future>(future: F) -> F::Output {
        let mut future = std::pin::pin!(future);
        let waker = Waker::from(Arc::new(NoWake));
        let mut context = Context::from_waker(&waker);
        match future.as_mut().poll(&mut context) {
            Poll::Ready(value) => value,
            Poll::Pending => panic!("unexpected pending"),
        }
    }
    #[test]
    fn prepare_forward_clear_and_execute_without_inventing_intent() {
        let inputs: Vec<Json> =
            serde_json::from_str(include_str!("__TEAQL_PRIVACY_PREPARATION_INPUTS__")).unwrap();
        for input in inputs {
            let mut descriptor = EntityDescriptor::new("Payment")
                .property(PropertyDescriptor::new("secret", DataType::Text));
            descriptor = descriptor.audit_mask_fields(vec!["secret".into()]);
            let mut user_context = UserContext::new()
                .with_metadata(InMemoryMetadataStore::new().with_entity(descriptor));
            user_context.disable_sql_log();
            let executor = Executor(Mutex::new(vec![]));
            let service = EntityDataService::for_executor(&user_context, "Payment", &executor);
            let mut plan = GraphMutationPlan::default();
            if input["plan"] == "secret" {
                let values = std::collections::BTreeMap::from([(
                    "secret".into(),
                    Value::Text("PRIVATE-VALUE".into()),
                )]);
                plan.push(
                    "Payment",
                    GraphMutationKind::Update,
                    values.into(),
                    vec![],
                    None,
                    None,
                );
                plan.rebuild_batches();
            }
            let service = service.with_plan_privacy(&plan).unwrap();
            let mut output = json!({"prepared":true,"query_privacy":service.query_log_intent.is_some(),
                "prepared_intent":service.data_service.mutation_intent.is_some(),
                "prepared_privacy":service.data_service.mutation_privacy.as_ref().map(|p|p.is_empty())});
            let mut context = service.data_service;
            if input["clear"] == true {
                context.mutation_privacy = None;
            }
            let intent = MutationIntent::new("persist payment").unwrap();
            let bind = input["bind"] == true;
            let mut command = InsertCommand::new("Payment");
            command.values.insert("id".into(), Value::U64(7));
            let result = match input["route"].as_str().unwrap() {
                "context" => {
                    if bind {
                        context = context.with_mutation_intent(intent);
                    }
                    wait(context.insert(&command))
                }
                "bind_then_forward" => {
                    if bind {
                        context = context.with_mutation_intent(intent);
                    }
                    wait(context.data_service().insert(&command))
                }
                "forward_then_bind" => {
                    let mut base = context.data_service();
                    if bind {
                        base = base.with_mutation_intent(intent);
                    }
                    wait(base.insert(&command))
                }
                _ => panic!("invalid route"),
            };
            match result {
                Ok(rows) => output["rows"] = json!(rows),
                Err(error) => output["error"] = json!(error.to_string()),
            }
            let requests = executor.0.lock().unwrap();
            output["requests"] = json!(requests.len());
            output["request_privacy"] = json!(requests.first().copied().flatten());
            println!("TEAQL_PRIVACY_PREPARATION_JSON {}", output);
        }
    }
}
