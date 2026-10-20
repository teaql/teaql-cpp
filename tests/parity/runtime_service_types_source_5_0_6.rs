#[cfg(test)]
mod cpp_service_types_observer {
    use super::*;
    use serde_json::{Value as Json, json};
    use std::collections::{BTreeMap, HashMap};
    use teaql_core::{CompactRow, DataType, MutationIntent, MutationValues, PropertyDescriptor,
                     QueryIntent, TraceKind, TraceNode, Value};
    use teaql_data_service::{DataServiceCapabilities, DataServiceExecutor, ExecutionMetadata,
                            MutationExecutor, MutationRequest, MutationResult, QueryExecutor,
                            QueryRequest, QueryResult, SqlIntentRedactions, SqlLogContext,
                            SqlParameterLogPolicy};
    use crate::{EntityKey, EntityRuntimeState, GraphMutationBatch, GraphMutationKind,
                GraphMutationPlan, InMemoryMetadataStore, SqlLogOptions};

    struct Never;
    impl DataServiceExecutor for Never {
        type Error = std::io::Error;
        fn capabilities(&self) -> DataServiceCapabilities { DataServiceCapabilities::default() }
    }
    impl QueryExecutor for Never {
        async fn query(&self, _: QueryRequest) -> Result<QueryResult, Self::Error> {
            panic!("observer must not execute database queries")
        }
    }
    impl MutationExecutor for Never {
        async fn mutate(&self, _: MutationRequest) -> Result<MutationResult, Self::Error> {
            panic!("observer must not execute database mutations")
        }
    }
    fn descriptor() -> EntityDescriptor {
        let mut item = EntityDescriptor::new("Item");
        item.properties.push(PropertyDescriptor::new("id", DataType::U64).id());
        item.properties.push(PropertyDescriptor::new("version", DataType::I64).version());
        item.properties.push(PropertyDescriptor::new("name", DataType::Text));
        item.properties.push(PropertyDescriptor::new("token", DataType::Text).column_name("access_token"));
        item.properties.push(PropertyDescriptor::new("status", DataType::Text));
        item.audit_mask_fields(vec!["name".into()])
    }
    fn secrets(value: &SqlIntentRedactions) -> Json {
        let mut safe = Vec::new(); let mut plaintext = Vec::new();
        value.extend_secrets(false, &mut safe); value.extend_secrets(true, &mut plaintext);
        json!({"safe":safe,"plaintext":plaintext})
    }
    fn snapshot(service: &EntityDataService<'_, Never>) -> Json {
        service.query_intent_snapshot().as_ref().map(secrets).unwrap_or(Json::Null)
    }
    fn redaction(value: &str) -> SqlIntentRedactions {
        SqlIntentRedactions::from_bindings(&SqlLogContext { generated_sql:true,
            parameter_policies:vec![SqlParameterLogPolicy::Unknown], ..Default::default() },
            &[Value::from(value)], "SELECT ?")
    }
    fn leaf(values: [&str; 2], policies: [SqlParameterLogPolicy; 2]) -> ExecutionMetadata {
        let mut metadata = ExecutionMetadata::unrecorded_query(1);
        metadata.sql_log.generated_sql = true;
        metadata.sql_log.parameter_policies = policies.into();
        metadata.parameterized_query = Some("SELECT ?, ?".into());
        metadata.params = values.into_iter().map(Value::from).collect();
        metadata
    }
    fn pristine(metadata: &ExecutionMetadata) -> bool {
        metadata.sql_log.intent_redactions.is_empty() && metadata.statements.iter().all(pristine)
    }
    fn evaluate(input: &Json) -> Json {
        let family = input["family"].as_str().unwrap();
        let context = UserContext::new().with_metadata(InMemoryMetadataStore::new().with_entity(descriptor()));
        let executor = Never;
        let mut service = EntityDataService::for_executor(&context, "Item", &executor);
        match family {
            "intent" => {
                if input["trace"] == true {
                    service = service.with_trace_context(vec![TraceNode::typed(
                        TraceKind::AuditReason, "Injected", None, "trace cannot grant request intent")]);
                }
                if input["override"] == true {
                    service.request_intent = Some(QueryIntent::new("owned what", "owned why").unwrap());
                }
                let mut query = SelectQuery::new("Item");
                query.comment = input["comment"].as_str().map(str::to_owned);
                query.purpose = input["purpose"].as_str().map(str::to_owned);
                let result = match service.request_intent_for(&query) {
                    Ok(intent) => json!({"comment":intent.comment(),"purpose":intent.purpose()}),
                    Err(error) => json!({"error":error.to_string()}),
                };
                json!({"result":result,"owned":service.request_intent.is_some(),
                    "privacy":snapshot(&service),"mutation":service.data_service.mutation_intent.is_some()})
            }
            "metadata" => {
                let mut context = UserContext::new();
                if input["populated"] == true {
                    context.set_metadata(InMemoryMetadataStore::new().with_entity(descriptor()));
                }
                context.set_sql_log_options(SqlLogOptions { select:input["select"]==true,
                    mutation:input["mutation"]==true });
                let service = EntityDataService::for_executor(&context, "Item", &executor);
                let view = &service.data_service.metadata;
                json!({"count":view.all_entities().len(),"item":view.entity("Item").is_some(),
                    "missing":view.entity("Missing").is_some(),"debug":view.capture_query_debug(),
                    "capture":view.capture_execution_metadata(),"query_observer":service.query_diagnostic_observer().is_some(),
                    "mutation_observer":view.mutation_diagnostic_observer().is_some()})
            }
            "plan" => {
                let mut plan = GraphMutationPlan::default();
                let mut values = BTreeMap::from([("name".into(),Value::from("CURRENT-MASKED")),
                    ("status".into(),Value::from("CURRENT-PLAIN"))]);
                if input["ignored"] == true {
                    for field in ["id","version","unmapped"] { values.insert(field.into(), Value::from("IGNORED-VALUE")); }
                }
                let old = (input["old"]==true).then(|| BTreeMap::from([
                    ("name".into(),Value::from("OLD-MASKED")),("token".into(),Value::from("OLD-CREDENTIAL"))]).into());
                plan.push("Item",GraphMutationKind::Update,MutationValues::from(values),vec!["name".into()],None,old);
                plan.rebuild_batches();
                if input["missing"] == true { plan.batches.push(GraphMutationBatch {
                    entity:"Missing".into(),kind:GraphMutationKind::Create,update_fields:vec![],items:vec![] }); }
                let result = service.with_mutation_intent(MutationIntent::new("governed root reason").unwrap()).with_plan_privacy(&plan);
                match result {
                    Err(error) => json!({"error":error.to_string()}),
                    Ok(scoped) => json!({"query":snapshot(&scoped),
                        "mutation":secrets(scoped.data_service.mutation_privacy.as_ref().unwrap()),
                        "parent":snapshot(&service)}),
                }
            }
            "ledger" => {
                let mut state = EntityRuntimeState::default();
                state.set_original_compact_row("Item", CompactRow::from_map(BTreeMap::from([
                    ("id".into(),Value::U64(7)),("version".into(),Value::I64(1)),
                    ("name".into(),Value::from("LEDGER-OLD-MASKED")),
                    ("token".into(),Value::from("LEDGER-OLD-CREDENTIAL"))])));
                let definitions = teaql_core::dynamic_fields::DynamicFieldDefinitions::new(
                    "Item","v1",vec![("private".into(),DataType::Text)]).unwrap();
                let values = teaql_core::dynamic_fields::DynamicFieldValues::from_values(definitions,
                    HashMap::from([("private".into(),Value::from("DYNAMIC-ORIGINAL"))])).unwrap();
                state.install_loaded_dynamic_fields(values);
                let key = EntityKey::new("Item",Value::U64(7));
                match input["action"].as_str().unwrap() {
                    "set" => { state.update_dynamic_field(key.clone(),"private",Value::from("DYNAMIC-CURRENT")).unwrap(); },
                    "null" => { state.update_dynamic_field(key.clone(),"private",Value::Null).unwrap(); },
                    _ => { state.delete_dynamic_field(key.clone(),"private").unwrap(); },
                }
                if input["native"]==true { state.set(key.clone(),"name",Value::from("LEDGER-CURRENT-MASKED")); }
                if input["deleted"]==true { state.mark_as_delete(key); }
                let result = service.with_mutation_intent(MutationIntent::new("ledger root reason").unwrap()).with_ledger_privacy(&state).unwrap();
                json!({"query":snapshot(&result),"mutation":secrets(result.data_service.mutation_privacy.as_ref().unwrap())})
            }
            "inherit" => {
                let parent = service;
                let mut child = parent.scoped_data_service_internal("Item".into());
                if input["scope"]==true { child.query_log_intent=Some(std::sync::Mutex::new(redaction("ANCESTOR-PRIVATE"))); }
                let mut metadata = ExecutionMetadata::unrecorded_query(2);
                let first = leaf(["LEFT-SENSITIVE","LEFT-PUBLIC"],[SqlParameterLogPolicy::Masked,SqlParameterLogPolicy::Plain]);
                let second = leaf(["LEFT-SENSITIVE","RIGHT-SENSITIVE"],[SqlParameterLogPolicy::Plain,SqlParameterLogPolicy::Unknown]);
                if input["nested"]==true {
                    let mut branch=ExecutionMetadata::unrecorded_query(2);branch.statements=vec![first,second];metadata.statements=vec![branch];
                } else { metadata.statements=vec![first,second]; }
                child.record_query_metadata(&metadata);
                json!({"child":snapshot(&child),"parent":snapshot(&parent),
                    "input_pristine":pristine(&metadata),"logs":context.sql_logs().len()})
            }
            _ => panic!("unknown observer family"),
        }
    }
    #[test]
    fn replay_complete_types_calls() {
        let inputs: Vec<Json> = serde_json::from_str(include_str!("__TEAQL_TYPES_INPUTS__")).unwrap();
        for input in inputs { println!("TEAQL_SERVICE_TYPES_JSON {}",json!({"input":input,"output":evaluate(&input)})); }
    }
}
