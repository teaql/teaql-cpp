#[cfg(test)]
mod cpp_resolved_scope_tree_observer {
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
    fn leaf(name: &str) -> SelectQuery {
        let mut query=SelectQuery::new(name);
        query.child_enhancements.push(SelectQuery::new(format!("{name}Nested")));
        query
    }
    fn tree(shape: &str) -> SelectQuery {
        let mut query=SelectQuery::new("RootDiagnostic");
        if shape=="relation" || shape=="mixed" {
            query=query.relation("Unqueried").relation_query("edge",leaf("Relation"));
        }
        if shape=="child" || shape=="mixed" { query.child_enhancements.push(leaf("Child")); }
        if shape=="aggregate" || shape=="mixed" {
            query.relation_aggregates.push(RelationAggregate::new("edge","total",leaf("Aggregate"),false));
        }
        if shape=="group" || shape=="mixed" { query=query.object_group_by("object","id",leaf("Group")); }
        if shape=="facet" || shape=="mixed" {
            query.facets.push(teaql_core::request::FacetRequest::new("facet","edge",leaf("Facet"),false));
        }
        query
    }
    #[test]
    fn scope_tree() {
        let mut case=0;
        for shape in ["empty","relation","child","aggregate","group","facet","mixed"] {
            for capture in [false,true] {
                for valid in [false,true] {
                    let calls=Arc::new(Mutex::new(vec![]));
                    let mut context=crate::RuntimeModule::new().into_context();
                    context.set_sql_log_options(crate::SqlLogOptions{select:capture,mutation:false});
                    let executor=Executor(calls.clone());
                    let mut parent=EntityDataService::for_executor(&context,"Parent",&executor);
                    add(&mut parent,"parent-secret");
                    let mut query=tree(shape);
                    if valid { query.comment=Some("tree comment".into()); query.purpose=Some("tree purpose".into()); }
                    let original=query.clone();
                    let scoped=parent.query_scoped_service(&query);
                    let (query_scope,error)=match scoped {
                        Ok(value)=>(snapshot(&value),None),
                        Err(error)=>(Json::Null,Some(error.to_string())),
                    };
                    println!("RESOLVED_SCOPE_TREE_OBSERVATION {}",json!({"case":case,"shape":shape,"capture":capture,"valid":valid,
                        "parent":snapshot(&parent),"query_scope":query_scope,"error":error,
                        "unchanged":query==original,"classified":*calls.lock().unwrap()}));
                    case+=1;
                }
            }
        }
        assert_eq!(case,28);
    }
}
