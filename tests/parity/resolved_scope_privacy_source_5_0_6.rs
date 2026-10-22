#[cfg(test)]
mod cpp_resolved_scope_privacy_observer {
    use super::*;
    use teaql_core::QueryIntent;
    use serde_json::{json, Value as Json};
    use std::sync::{Arc, Mutex};
    use teaql_data_service::*;

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
            panic!("classification must not execute SQL")
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
    #[test]
    fn scope_privacy() {
        let mut case = 0;
        for seed in [false,true] {
            for bound in [false,true] {
                for valid in [false,true] {
                    let calls=Arc::new(Mutex::new(vec![]));
                    let context=crate::RuntimeModule::new().into_context();
                    let executor=Executor(calls.clone());
                    let mut parent=EntityDataService::for_executor(&context,"Parent",&executor);
                    if seed { add(&mut parent,"seed-secret"); }
                    if bound { parent.request_intent=Some(QueryIntent::new("bound comment","bound purpose").unwrap()); }
                    let mut child=parent.scoped_data_service_internal("Child".into());
                    let sibling=parent.scoped_data_service_internal("Sibling".into());
                    add(&mut parent,"parent-late-secret");
                    add(&mut child,"child-only-secret");
                    let mut query=SelectQuery::new("RootDiagnostic");
                    if valid {
                        query.comment=Some("query comment".into());
                        query.purpose=Some("query purpose".into());
                    }
                    query.child_enhancements.push(SelectQuery::new("FirstChild"));
                    query.child_enhancements.push(SelectQuery::new("LastChild"));
                    let scoped=parent.query_scoped_service(&query);
                    let (query_scope,error)=match scoped {
                        Ok(mut value)=>{add(&mut value,"query-only-secret");(snapshot(&value),None)},
                        Err(error)=>(Json::Null,Some(error.to_string())),
                    };
                    println!("RESOLVED_SCOPE_PRIVACY_OBSERVATION {}",json!({"case":case,"seed":seed,"bound":bound,"valid":valid,
                        "parent":snapshot(&parent),"child":snapshot(&child),"sibling":snapshot(&sibling),
                        "query_scope":query_scope,"error":error,"classified":*calls.lock().unwrap()}));
                    case+=1;
                }
            }
        }
        assert_eq!(case,8);
    }
}
