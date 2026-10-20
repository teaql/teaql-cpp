#[cfg(test)]
mod cpp_context_factory_observer {
    use super::*;
    use crate::{EntityRegistry, InMemoryMetadataStore};
    use serde_json::{Value as Json, json};
    use std::io;
    use teaql_core::EntityDescriptor;
    use teaql_data_service::*;

    #[derive(Clone, Copy)]
    struct Executor<const K: u8>;
    impl<const K: u8> DataServiceExecutor for Executor<K> {
        type Error = io::Error;
        fn capabilities(&self) -> DataServiceCapabilities {
            DataServiceCapabilities::default()
        }
    }
    impl<const K: u8> QueryExecutor for Executor<K> {
        async fn query(&self, _: QueryRequest) -> Result<QueryResult, Self::Error> {
            panic!("factory queried")
        }
    }
    impl<const K: u8> MutationExecutor for Executor<K> {
        async fn mutate(&self, _: MutationRequest) -> Result<MutationResult, Self::Error> {
            panic!("factory mutated")
        }
    }
    impl<const K: u8> Transaction for Executor<K> {
        type Error = io::Error;
        async fn commit(self) -> Result<(), Self::Error> {
            panic!("factory committed")
        }
        async fn rollback(self) -> Result<(), Self::Error> {
            panic!("factory rolled back")
        }
    }
    impl<const K: u8> TransactionExecutor for Executor<K> {
        type Tx<'a> = Self;
        async fn begin(&self) -> Result<Self, Self::Error> {
            panic!("factory began transaction")
        }
    }
    struct Registry;
    impl EntityRegistry for Registry {
        fn contains(&self, entity: &str) -> bool {
            entity == "Payment"
        }
    }
    fn inspect<const K: u8>(user_context: &UserContext, entity: &str) -> Json {
        match user_context.entity_data_service::<Executor<K>>(entity) {
            Ok(service) => json!({"entity":service.entity,"trace":service.trace_context.len(),
                "query_intent":service.request_intent.is_some(),"query_privacy":service.query_log_intent.is_some(),
                "mutation_intent":service.data_service.mutation_intent.is_some(),
                "mutation_privacy":service.data_service.mutation_privacy.is_some()}),
            Err(ContextError::MissingEntityDataService(name)) => {
                assert_eq!(name, entity);
                json!({"error":"entity"})
            }
            Err(ContextError::MissingResource(name)) => {
                assert_eq!(name, "metadata");
                json!({"error":"metadata"})
            }
            Err(ContextError::MissingTypedResource(name)) => {
                assert_eq!(name, std::any::type_name::<Executor<K>>());
                json!({"error":"executor"})
            }
        }
    }
    #[test]
    fn replay_factory_error_priority_defaults_and_registration() {
        let inputs: Vec<Json> =
            serde_json::from_str(include_str!("__TEAQL_CONTEXT_FACTORY_INPUTS__")).unwrap();
        for input in inputs {
            let mut user_context = UserContext::new();
            user_context.disable_sql_log();
            if input["metadata"] == true {
                let mut metadata = InMemoryMetadataStore::new();
                if input["described"] == true {
                    metadata = metadata.with_entity(EntityDescriptor::new("Payment"));
                }
                user_context.set_metadata(metadata);
            }
            if input["registry"] == true {
                user_context.set_entity_registry(Registry);
            }
            match input["executor"].as_str().unwrap() {
                "plain" => user_context.insert_resource(Executor::<0>),
                "registered" => user_context.register_executor(Executor::<0>),
                "none" => (),
                _ => panic!("invalid mode"),
            }
            let entity = input["entity"].as_str().unwrap();
            let mut output = if input["other_type"] == true {
                inspect::<1>(&user_context, entity)
            } else {
                inspect::<0>(&user_context, entity)
            };
            output["executor0"] = json!(user_context.get_resource::<Executor<0>>().is_some());
            output["executor1"] = json!(user_context.get_resource::<Executor<1>>().is_some());
            output["saver"] = json!(
                user_context
                    .get_resource::<Arc<dyn crate::entity_save::DynGraphSaver>>()
                    .is_some()
            );
            println!("TEAQL_CONTEXT_FACTORY_JSON {}", output);
        }
    }
}
