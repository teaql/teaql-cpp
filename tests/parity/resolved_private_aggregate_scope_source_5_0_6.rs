#[cfg(test)]
mod cpp_resolved_private_aggregate_scope_observer {
    use super::*;
    use teaql_core::QueryIntent;
    use serde_json::{json, Value as Json};
    use std::sync::{Arc, Mutex};
    use teaql_data_service::*;

    #[derive(Debug, Clone, PartialEq, teaql_macros::TeaqlEntity)]
    #[teaql(entity="Parent",table="parents")]
    struct Parent { #[teaql(id)] id:u64, #[teaql(version)] version:i64 }
    struct Executor(Arc<Mutex<Vec<String>>>);
    impl DataServiceExecutor for Executor {
        type Error = std::io::Error;
        fn capabilities(&self) -> DataServiceCapabilities { DataServiceCapabilities::default() }
    }
    impl QueryExecutor for Executor {
        fn query_log_intent(&self, query: &SelectQuery) -> SqlIntentRedactions {
            self.0.lock().unwrap().push(query.entity.clone());
            let mut intent = SqlIntentRedactions::default();
            intent.capture_target_id(&Value::from(query.entity.clone()));
            intent
        }
        async fn query(&self, _: QueryRequest) -> Result<QueryResult, Self::Error> {
            Ok(QueryResult{rows:vec![],metadata:ExecutionMetadata::unrecorded_query(0)})
        }
    }
    impl MutationExecutor for Executor {
        async fn mutate(&self, _: MutationRequest) -> Result<MutationResult, Self::Error> {
            panic!("classification must not mutate")
        }
    }
    fn add(service: &mut EntityDataService<'_, Executor>, value: &str) {
        let intent = service.query_log_intent.get_or_insert_with(Default::default);
        intent.lock().unwrap().capture_target_id(&Value::from(value));
    }
    fn snapshot(service: &EntityDataService<'_, Executor>) -> Json {
        let privacy = service.query_intent_snapshot().map(|intent| {
            let mut secrets = vec![];
            intent.extend_secrets(false, &mut secrets);
            secrets.sort();
            secrets
        });
        json!({"entity":service.entity(),"privacy":privacy,
            "intent":service.request_intent.as_ref().map(|v|json!([v.comment(),v.purpose()]))})
    }
    use std::future::Future;
    use std::task::{Context,Poll,Wake,Waker};
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
    fn private_aggregate_scope() {
        let mut case=0;
        for dynamic in [false,true] {
            for capture in [false,true] {
                for bound in [false,true] {
                    let calls=Arc::new(Mutex::new(vec![]));
                    let mut context=crate::RuntimeModule::new().into_context();
                    context.set_sql_log_options(crate::SqlLogOptions{select:capture,mutation:false});
                    let executor=Executor(calls.clone());
                    let mut parent=EntityDataService::for_executor(&context,"Parent",&executor);
                    if bound { parent.request_intent=Some(QueryIntent::new("bound comment","bound purpose").unwrap()); }
                    let mut query=SelectQuery::new("Parent"); query.comment=Some("read parent".into());
                    query.purpose=Some("verify aggregate scope".into());
                    if dynamic { query.dynamic_field_selection=Some(Arc::new(teaql_core::dynamic_fields::DynamicFieldSelection::All)); }
                    let aggregates=vec![RelationAggregate::new("children","total",SelectQuery::new("AggregateSecret"),true)];
                    let result=ready(parent.fetch_entities_with_relation_aggregates_internal::<Parent>(&query,&aggregates));
                    println!("RESOLVED_PRIVATE_AGGREGATE_SCOPE_OBSERVATION {}",json!({"case":case,"dynamic":dynamic,"capture":capture,"bound":bound,
                        "error":result.as_ref().err().map(|v|v.to_string()),"classified":*calls.lock().unwrap(),"parent":snapshot(&parent)}));
                    case+=1;
                }
            }
        }
        assert_eq!(case,8);
    }
}
