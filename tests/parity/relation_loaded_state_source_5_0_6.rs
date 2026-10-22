#[cfg(test)]
mod cpp_relation_loaded_state_observer {
    use super::*;
    use serde_json::json;
    use teaql_data_service::*;

    struct NoIo;
    impl DataServiceExecutor for NoIo {
        type Error = std::io::Error;
        fn capabilities(&self) -> DataServiceCapabilities {
            DataServiceCapabilities::default()
        }
    }
    impl QueryExecutor for NoIo {
        async fn query(&self, _: QueryRequest) -> Result<QueryResult, Self::Error> {
            panic!("attachment must not execute a query")
        }
    }
    impl MutationExecutor for NoIo {
        async fn mutate(&self, _: MutationRequest) -> Result<MutationResult, Self::Error> {
            panic!("attachment must not execute a mutation")
        }
    }

    #[test]
    fn attachment_states() {
        let context = crate::RuntimeModule::new().into_context();
        let executor = NoIo;
        let service = EntityDataService::for_executor(&context, "School", &executor);
        let mut case = 0;
        for many in [false, true] {
            for nested in [0, 1, 2] {
                for count in [0, 2] {
                    for facet in [false, true] {
                        let fields = BTreeMap::from([("id".into(), Value::U64(7))]);
                        let mut parents = vec![CompactRow::from_map(fields.clone())];
                        let keys = parents.clone();
                        let mut children = vec![];
                        for _ in 0..count {
                            let mut child = CompactRow::from_map(fields.clone());
                            if nested != 0 {
                                let mut list = SmartList::new(vec![]);
                                list.is_loaded = nested == 2;
                                child.set_loaded_relation("descendants".into(), list);
                            }
                            children.push(child);
                        }
                        let mut query = SelectQuery::new("School");
                        if facet {
                            query.facets.push(teaql_core::request::FacetRequest::new(
                                "summary", "peers", SelectQuery::new("School"), false));
                        }
                        let plan = RelationLoadPlan {
                            parent_entity: "School".into(), target_entity: "School".into(),
                            relation_name: "peers".into(), path: "peers".into(),
                            local_key: "id".into(), foreign_key: "id".into(), many,
                            query: Some(query), children: vec![],
                        };
                        service.attach_relation_rows(&mut parents, &keys, &plan, children,
                                                     vec![Some(Value::U64(7)); count]);
                        let list = parents[0].loaded_relation("peers");
                        let states = list.map(|list| list.data.iter().map(|row|
                            row.loaded_relation("descendants").map(|child| child.is_loaded)
                        ).collect::<Vec<_>>());
                        println!("RELATION_LOADED_STATE_OBSERVATION {}", json!({
                            "case": case, "many": many, "nested": nested, "count": count,
                            "facet": facet, "loaded": list.map(|list| list.is_loaded),
                            "size": list.map(|list| list.data.len()), "nested_states": states
                        }));
                        case += 1;
                    }
                }
            }
        }
        assert_eq!(case, 24);
    }
}
