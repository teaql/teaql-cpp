#[cfg(test)]
mod cpp_context_mutation_cache_observer {
    use super::*;
    use serde_json::{Value as Json, json};
    use std::{
        future::Future,
        io,
        panic::{AssertUnwindSafe, catch_unwind},
        sync::{Arc, Mutex},
        task::{Context, Poll, Wake, Waker},
    };
    use teaql_core::*;
    use teaql_data_service::*;

    #[derive(Default)]
    struct State {
        notes: Mutex<Vec<String>>,
        requests: Mutex<usize>,
    }
    struct Cache(Arc<State>);
    impl AggregationCacheBackend for Cache {
        fn namespace(&self) -> &str {
            "custom"
        }
        fn get(&self, _: &str, _: u64) -> Option<Vec<CompactRow>> {
            None
        }
        fn put(&self, _: String, _: Vec<CompactRow>) {}
        fn invalidate_namespace(&self, name: &str) {
            self.0.notes.lock().unwrap().push(format!("cache:{name}"));
        }
    }
    struct Executor {
        state: Arc<State>,
        counts: Vec<u64>,
        failure: i64,
    }
    impl DataServiceExecutor for Executor {
        type Error = io::Error;
        fn capabilities(&self) -> DataServiceCapabilities {
            DataServiceCapabilities::default()
        }
    }
    impl QueryExecutor for Executor {
        async fn query(&self, _: QueryRequest) -> Result<QueryResult, Self::Error> {
            panic!("mutation queried")
        }
    }
    impl MutationExecutor for Executor {
        async fn mutate(&self, _: MutationRequest) -> Result<MutationResult, Self::Error> {
            let mut size = self.state.requests.lock().unwrap();
            let index = *size;
            *size += 1;
            drop(size);
            self.state.notes.lock().unwrap().push("provider".into());
            if index as i64 == self.failure {
                return Err(io::Error::other("provider failed"));
            }
            let count = self.counts.get(index).copied().unwrap_or(1);
            let mut metadata = ExecutionMetadata::unrecorded_query(0);
            metadata.affected_rows = Some(count);
            Ok(MutationResult {
                affected_rows: count,
                generated_values: GeneratedValues::new(),
                persisted_snapshot: None,
                metadata,
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
    fn evaluate(input: &Json) -> Json {
        let state = Arc::new(State::default());
        let mut user_context = UserContext::new();
        user_context.disable_sql_log();
        let caches = input["caches"].as_u64().unwrap();
        if caches & 1 != 0 {
            let cache: Arc<dyn AggregationCacheBackend> = Arc::new(Cache(state.clone()));
            user_context.insert_resource(cache);
        }
        let memory = InMemoryAggregationCache::with_namespace("mem");
        let keys = [
            "mem::entity:Payment::row",
            "mem::entity:Payments::row",
            "mem::entity:Other::row",
            "foreign::entity:Payment::row",
            "mem::entity:Payment",
        ];
        for key in keys {
            memory.put(key.into(), vec![]);
        }
        if caches & 2 != 0 {
            user_context.insert_resource(memory);
        }
        let executor = Executor {
            state: state.clone(),
            counts: input["counts"]
                .as_array()
                .unwrap()
                .iter()
                .map(|x| x.as_u64().unwrap())
                .collect(),
            failure: input["failure"].as_i64().unwrap(),
        };
        let mut service = ContextDataService {
            metadata: UserContextMetadata {
                context: &user_context,
            },
            executor: &executor,
            mutation_intent: None,
            mutation_privacy: None,
        };
        if input["intent"] == true {
            service = service.with_mutation_intent(MutationIntent::new("change payment").unwrap());
        }
        let versions: Vec<Option<i64>> = input["versions"]
            .as_array()
            .unwrap()
            .iter()
            .map(Json::as_i64)
            .collect();
        let n = input["length"].as_u64().unwrap() as usize;
        let m = input["ids"].as_u64().unwrap() as usize;
        let result = catch_unwind(AssertUnwindSafe(|| {
            match input["entry"].as_str().unwrap() {
                "batch_insert" => {
                    let mut command = BatchInsertCommand::new("Payment");
                    command.batch_values = vec![MutationValues::new(); n];
                    wait(service.batch_insert(&command))
                }
                "batch_update" => {
                    let mut command = BatchUpdateCommand::new("Payment", vec![]);
                    command.batch_values = vec![MutationValues::new(); n];
                    command.batch_ids = vec![Value::U64(7); m];
                    command.batch_expected_versions = versions;
                    wait(service.batch_update(&command))
                }
                "insert" => wait(service.insert(&InsertCommand::new("Payment"))),
                "update" => {
                    let mut command = UpdateCommand::new("Payment", Value::U64(7));
                    command.expected_version = versions.first().copied().flatten();
                    wait(service.update(&command))
                }
                "delete" => {
                    let mut command = DeleteCommand::new("Payment", Value::U64(7));
                    command.expected_version = versions.first().copied().flatten();
                    wait(service.delete(&command))
                }
                "recover" => {
                    wait(service.recover(&RecoverCommand::new("Payment", Value::U64(7), 1)))
                }
                _ => panic!("invalid fixture entry"),
            }
        }));
        let outcome = match result {
            Ok(Ok(rows)) => json!({"rows":rows}),
            Ok(Err(error)) => json!({"error":error.to_string()}),
            Err(_) => json!({"panic":true}),
        };
        let cache = if caches & 2 != 0 {
            let memory = user_context
                .get_resource::<InMemoryAggregationCache>()
                .unwrap();
            json!(
                keys.iter()
                    .map(|key| memory.get(key, 0).is_some())
                    .collect::<Vec<_>>()
            )
        } else {
            Json::Null
        };
        json!({"outcome":outcome,"requests":*state.requests.lock().unwrap(),"notes":*state.notes.lock().unwrap(),"cache":cache})
    }
    #[test]
    fn replay_success_error_and_cache_boundaries() {
        let inputs: Vec<Json> =
            serde_json::from_str(include_str!("__TEAQL_CONTEXT_CACHE_INPUTS__")).unwrap();
        let hook = std::panic::take_hook();
        std::panic::set_hook(Box::new(|_| {}));
        for input in inputs {
            println!("TEAQL_CONTEXT_CACHE_JSON {}", evaluate(&input));
        }
        std::panic::set_hook(hook);
    }
}
