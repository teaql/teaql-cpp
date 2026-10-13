use serde_json::{Value, json};
use std::io::{self, BufRead};
use teaql_core::FieldLayout;

fn strings(value: &Value) -> Vec<&str> {
    value
        .as_array()
        .unwrap()
        .iter()
        .map(|item| item.as_str().unwrap())
        .collect()
}

fn evaluate(input: &Value) -> Value {
    let indexes: Vec<_> = input["indexes"]
        .as_array()
        .unwrap()
        .iter()
        .map(|item| {
            (
                item[0].as_str().unwrap(),
                item[1].as_u64().unwrap() as usize,
            )
        })
        .collect();
    let mappings: Vec<_> = input["mappings"]
        .as_array()
        .unwrap()
        .iter()
        .map(|item| {
            (
                item[0].as_str().unwrap(),
                item[1].as_str().unwrap(),
                item[2].as_str().unwrap(),
            )
        })
        .collect();
    let relations = strings(&input["relations"]);
    let expected = strings(&input["expected_members"]);
    match FieldLayout::from_generated(
        input["entity"].as_str().unwrap(),
        input["revision"].as_str().unwrap(),
        &indexes,
        &mappings,
        &relations,
        &expected,
    ) {
        Err(error) => json!({"error": error}),
        Ok(layout) => {
            let queries = strings(&input["lookups"]);
            json!({"entity": layout.entity(), "revision": layout.revision(), "count": layout.field_count(),
                "indexes": queries.iter().map(|name| layout.index(name)).collect::<Vec<_>>(),
                "relations": queries.iter().map(|name| layout.is_relation(name)).collect::<Vec<_>>(),
                "shared_identity": layout.shared_entity_name().as_ptr() == layout.shared_entity_name().as_ptr()})
        }
    }
}

fn main() {
    for line in io::stdin().lock().lines() {
        let input: Value = serde_json::from_str(&line.unwrap()).unwrap();
        let output = match input["kind"].as_str() {
            Some("snapshot") | Some("row") => snapshot_probe(&input),
            Some("schema") => schema_probe(&input),
            Some("checker_helpers") => checker_helper_probe(&input),
            Some("comments") => comment_probe(&input),
            Some("generated_relations") => generated_relations_probe(&input),
            Some("business_id") => business_id_probe(&input),
            Some("mutation_envelope") => mutation_envelope_probe(&input),
            Some("request_envelope") => request_envelope_probe(&input),
            Some("request_intent") => request_intent_probe(&input),
            Some("trace_tokens") => trace_tokens_probe(&input),
            Some("original") => original_probe(&input),
            Some("load_state") => load_state_probe(&input),
            _ => evaluate(&input),
        };
        println!("{}", output);
    }
}

fn data_type(value: u64) -> teaql_core::DataType {
    use teaql_core::DataType::*;
    [
        Bool, I64, U64, F64, Decimal, Text, LargeText, Json, Date, Timestamp,
    ][value as usize]
}

fn native_value(input: &Value) -> teaql_core::Value {
    use teaql_core::Value as V;
    match input["kind"].as_str().unwrap() {
        "null" => V::Null,
        "typed_null" => V::TypedNull(data_type(input["datatype"].as_u64().unwrap())),
        "bool" => V::Bool(input["value"].as_bool().unwrap()),
        "i64" => V::I64(input["value"].as_str().unwrap().parse().unwrap()),
        "u64" => V::U64(input["value"].as_str().unwrap().parse().unwrap()),
        "f64" => V::F64(input["value"].as_str().unwrap().parse().unwrap()),
        "decimal" => V::Decimal(input["value"].as_str().unwrap().parse().unwrap()),
        "text" => V::Text(input["value"].as_str().unwrap().to_owned()),
        "date" => V::Date(
            teaql_core::Value::Text(input["value"].as_str().unwrap().to_owned())
                .try_date()
                .unwrap(),
        ),
        "timestamp" => V::Timestamp(teaql_core::time::Timestamp(
            input["value"].as_str().unwrap().parse().unwrap(),
        )),
        "json" => V::Json(input["value"].clone()),
        "list" => V::List(
            input["value"]
                .as_array()
                .unwrap()
                .iter()
                .map(native_value)
                .collect(),
        ),
        "object" => V::Object(
            input["value"]
                .as_object()
                .unwrap()
                .iter()
                .map(|(k, v)| (k.clone(), native_value(v)))
                .collect(),
        ),
        _ => panic!("unsupported readonly property fixture"),
    }
}

fn schema(
    input: &Value,
) -> Result<std::sync::Arc<teaql_core::dynamic_properties::DynamicPropertyDefinitions>, String> {
    teaql_core::dynamic_properties::DynamicPropertyDefinitions::new(
        input.as_array().unwrap().iter().map(|item| {
            (
                item[0].as_str().unwrap().to_owned(),
                data_type(item[1].as_u64().unwrap()),
            )
        }),
    )
}

fn schema_probe(input: &Value) -> Value {
    use std::sync::Arc;
    let left = match schema(&input["fields"]) {
        Ok(v) => v,
        Err(e) => return json!({"error":e}),
    };
    let validations: Vec<_> = input["values"]
        .as_array()
        .unwrap()
        .iter()
        .map(|item| {
            left.validate_value(
                item["name"].as_str().unwrap(),
                &native_value(&item["input"]),
            )
            .err()
        })
        .collect();
    let merged = match schema(&input["merge"]) {
        Err(error) => json!({"error":error}),
        Ok(right) => {
            match teaql_core::dynamic_properties::DynamicPropertyDefinitions::merge(&left, &right) {
                Err(error) => json!({"error":error}),
                Ok(next) => {
                    json!({"left":Arc::ptr_eq(&left,&next),"right":Arc::ptr_eq(&right,&next),
                "types": next.types().iter().map(|(name,ty)| json!([name,*ty as u64])).collect::<Vec<_>>()})
                }
            }
        }
    };
    json!({"types":left.types().iter().map(|(name,ty)| json!([name,*ty as u64])).collect::<Vec<_>>(),
        "validation_errors":validations, "merge":merged})
}

fn snapshot_probe(input: &Value) -> Value {
    use std::sync::Arc;
    use teaql_core::{LoadedSnapshot, eval::LoadState};
    let count = input["count"].as_u64().unwrap() as usize;
    let names: Vec<String> = (0..count)
        .map(|i| match i {
            0 => "id".to_owned(),
            1 => "version".to_owned(),
            _ => format!("field_{i}"),
        })
        .collect();
    let indexes: Vec<_> = names
        .iter()
        .enumerate()
        .map(|(i, n)| (n.as_str(), i))
        .collect();
    let mappings: Vec<_> = names
        .iter()
        .map(|n| (n.as_str(), n.as_str(), n.as_str()))
        .collect();
    let expected: Vec<_> = names.iter().map(String::as_str).collect();
    let layout =
        FieldLayout::from_generated("Item", "v1", &indexes, &mappings, &["lines"], &expected)
            .unwrap();
    if input["kind"] == "row" {
        return row_probe(input, layout);
    }
    let selected = strings(&input["selected"]);
    let build = || {
        if input["full"].as_bool().unwrap() {
            LoadedSnapshot::fully_loaded(layout.clone()).into_shared()
        } else {
            LoadedSnapshot::projection(layout.clone(), selected.iter().copied()).into_shared()
        }
    };
    let first = build();
    let duplicate = build();
    let mut current = first.clone();
    // Keep published states alive, matching C++'s explicit result-Context lifetime.
    let mut held = vec![first.clone(), duplicate.clone()];
    let mut actions = vec![];
    for action in input["actions"].as_array().unwrap() {
        match LoadedSnapshot::with_loaded(
            &current,
            action[0].as_str().unwrap(),
            action[1].as_bool().unwrap(),
        ) {
            Err(error) => actions.push(json!({"error":error})),
            Ok(next) => {
                actions.push(json!({"same_previous":Arc::ptr_eq(&current,&next),"same_initial":Arc::ptr_eq(&first,&next),"shared_overflow":match (current.overflow(),next.overflow()){(Some(a),Some(b))=>std::ptr::eq(a,b),_=>false},"shared_names":match (current.selected_names(),next.selected_names()){(Some(a),Some(b))=>std::ptr::eq(a,b),_=>false}}));
                held.push(next.clone());
                current = next;
            }
        }
    }
    if !input["schema"].is_null() {
        let definitions = match schema(&input["schema"]) {
            Ok(v) => v,
            Err(e) => return json!({"schema_error":e}),
        };
        current = LoadedSnapshot::with_dynamic_property_definitions(&current, definitions);
    }
    let lookups = strings(&input["lookups"]);
    let mut overflow: Vec<_> = current.overflow().into_iter().flatten().copied().collect();
    overflow.sort();
    let mut selected_names: Vec<_> = current
        .selected_names()
        .into_iter()
        .flatten()
        .cloned()
        .collect();
    selected_names.sort();
    let state = LoadState::Indexed(current.clone());
    json!({"same_initial_build":Arc::ptr_eq(&first,&duplicate),"actions":actions,
        "bits":current.bits().to_string(),"overflow":overflow,"overflow_present":current.overflow().is_some(),
        "selected_names":selected_names,"names_present":current.selected_names().is_some(),
        "loaded":lookups.iter().map(|n|current.is_loaded(n)).collect::<Vec<_>>(),
        "schema":current.dynamic_property_definitions().map(|d| d.types().iter().map(|(n,t)| json!([n,*t as u64])).collect::<Vec<_>>()),
        "serialize_error":serde_json::to_value(&state).err().map(|e|e.to_string())})
}

fn load_state_probe(input: &Value) -> Value {
    use teaql_core::{LoadedSnapshot, eval::LoadState};
    let count = input["count"].as_u64().unwrap() as usize;
    let names: Vec<String> = (0..count)
        .map(|i| match i {
            0 => "id".to_owned(),
            1 => "version".to_owned(),
            _ => format!("field_{i}"),
        })
        .collect();
    let indexes: Vec<_> = names
        .iter()
        .enumerate()
        .map(|(i, n)| (n.as_str(), i))
        .collect();
    let mappings: Vec<_> = names
        .iter()
        .map(|n| (n.as_str(), n.as_str(), n.as_str()))
        .collect();
    let expected: Vec<_> = names.iter().map(String::as_str).collect();
    let layout =
        FieldLayout::from_generated("Item", "v1", &indexes, &mappings, &["lines"], &expected)
            .unwrap();
    let selected = strings(&input["selected"]);
    let mut state = match input["mode"].as_str().unwrap() {
        "partial" => LoadState::Partial(selected.iter().map(|s| s.to_string()).collect()),
        "partial_compact" => LoadState::PartialCompact(
            selected
                .iter()
                .map(|s| std::borrow::Cow::Owned(s.to_string()))
                .collect(),
        ),
        "shared_columns" => LoadState::SharedColumns(
            selected
                .iter()
                .map(|s| s.to_string())
                .collect::<Vec<_>>()
                .into(),
        ),
        "full" => LoadState::FullyLoaded,
        "indexed" => LoadState::Indexed(
            LoadedSnapshot::projection(layout.clone(), selected.iter().copied()).into_shared(),
        ),
        _ => LoadState::NotLoaded,
    };
    let initial = state.clone();
    let lookups = strings(&input["lookups"]);
    let mut errors = vec![];
    for action in input["actions"].as_array().unwrap() {
        let field = action[0].as_str().unwrap();
        let result = if action[1].as_bool().unwrap() {
            state.mark_loaded(field)
        } else {
            state.mark_unloaded(field)
        };
        errors.push(result.err());
    }
    let before: Vec<_> = lookups.iter().map(|f| state.is_loaded(f)).collect();
    let original: Vec<_> = lookups.iter().map(|f| initial.is_loaded(f)).collect();
    let other = if input["different_layout"].as_bool().unwrap() {
        FieldLayout::from_generated("Item", "v1", &indexes, &mappings, &["lines"], &expected)
            .unwrap()
    } else {
        layout
    };
    match state.into_indexed(other) {
        Err(error) => {
            json!({"before":before,"original":original,"actions":errors,"conversion_error":error})
        }
        Ok(next) => {
            json!({"before":before,"original":original,"actions":errors,"conversion_error":Value::Null,
            "after":lookups.iter().map(|f|next.is_loaded(f)).collect::<Vec<_>>(),
            "serialize_error":serde_json::to_value(&next).err().map(|e|e.to_string())})
        }
    }
}

fn row_probe(input: &Value, layout: std::sync::Arc<FieldLayout>) -> Value {
    use std::sync::Arc;
    use teaql_core::{CompactRow, Value as V, eval::LoadState};
    let names: Arc<[String]> = strings(&input["selected"])
        .into_iter()
        .map(str::to_owned)
        .collect();
    let mut rows = vec![
        CompactRow::new(names.clone(), vec![V::I64(11); names.len()]),
        CompactRow::new(names.clone(), vec![V::I64(22); names.len()]),
        CompactRow::new(Arc::from(["id".to_owned()]), vec![V::I64(33)]),
    ];
    CompactRow::share_layouts(&mut rows);
    if input["relation_loaded"].as_bool().unwrap_or(false) {
        let mut shapes = teaql_core::RelationShapeCache::default();
        rows[0].mark_relation_loaded("lines", &mut shapes);
        rows[1].mark_relation_loaded("lines", &mut shapes);
    }

    let first = rows[0].indexed_load_state(layout.clone());
    let second = rows[1].indexed_load_state(layout.clone());
    let sparse = rows[2].indexed_load_state(layout.clone());
    let snapshot = |state: &LoadState| match state {
        LoadState::Indexed(state) => state.clone(),
        _ => panic!("expected indexed row state"),
    };
    // A second immutable type identity consumes the same result shape.
    let count = input["count"].as_u64().unwrap() as usize;
    let field_names: Vec<String> = (0..count)
        .map(|i| match i {
            0 => "id".to_owned(),
            1 => "version".to_owned(),
            _ => format!("field_{i}"),
        })
        .collect();
    let indexes: Vec<_> = field_names
        .iter()
        .enumerate()
        .map(|(i, n)| (n.as_str(), i))
        .collect();
    let mappings: Vec<_> = field_names
        .iter()
        .map(|n| (n.as_str(), n.as_str(), n.as_str()))
        .collect();
    let expected: Vec<_> = field_names.iter().map(String::as_str).collect();
    let other_layout =
        FieldLayout::from_generated("Other", "v1", &indexes, &mappings, &["lines"], &expected)
            .unwrap();
    let other = rows[0].indexed_load_state(other_layout.clone());
    let repeated = rows[1].indexed_load_state(other_layout);
    let lookups = strings(&input["lookups"]);
    let availability = |state: &LoadState| {
        lookups
            .iter()
            .map(|name| state.is_loaded(name))
            .collect::<Vec<_>>()
    };
    let initial = availability(&first);
    let mut changed = first.clone();
    changed.mark_unloaded("id").unwrap();
    let mut out = json!({
        "shared_shape": Arc::ptr_eq(&rows[0].shared_layout(), &rows[1].shared_layout()),
        "sparse_shape_same": Arc::ptr_eq(&rows[0].shared_layout(), &rows[2].shared_layout()),
        "shared_state": Arc::ptr_eq(&snapshot(&first), &snapshot(&second)),
        "other_shared": Arc::ptr_eq(&snapshot(&other), &snapshot(&repeated)),
        "other_distinct": !Arc::ptr_eq(&snapshot(&first), &snapshot(&other)),
        "initial": initial, "changed": availability(&changed),
        "unchanged": availability(&second), "sparse": availability(&sparse),
    });
    if let Some(name) = names.first() {
        rows[0].insert(name.clone(), V::I64(99));
        out["value_isolated"] = json!(rows[1].get(name) == Some(&V::I64(22)));
    } else {
        out["value_isolated"] = json!(true);
    }
    out
}

fn original_probe(input: &Value) -> Value {
    use std::sync::Arc;
    use teaql_core::{CompactRow, EntitySnapshot};
    use teaql_runtime::{EntityKey, EntityRuntimeState};
    let make_row = |values: &Value| {
        let object = values.as_object().unwrap();
        let names: Arc<[String]> = object.keys().cloned().collect();
        CompactRow::new(names, object.values().map(native_value).collect())
    };
    let snapshot = |state: &EntityRuntimeState| {
        state.original_snapshot().map(|row| {
            Value::Object(
                row.iter()
                    .map(|(k, v)| (k.clone(), v.to_json_value()))
                    .collect(),
            )
        })
    };
    let entity = input["entity"].as_str().unwrap();
    let key = EntityKey::new(entity, 7_u64);
    let mut root = EntityRuntimeState::default();
    root.set_original_compact_row(entity, make_row(&input["first"]));
    let before = snapshot(&root);
    let alias = root.clone();
    let mut materialized: EntitySnapshot = alias.original_snapshot().unwrap();
    materialized.insert("external".to_owned(), teaql_core::Value::Bool(true));
    let isolated = snapshot(&alias);
    root.set_original_compact_row(entity, make_row(&input["second"]));
    root.set(key.clone(), "payload", native_value(&input["dirty"]));
    let rebound = root.clone().with_loaded_view_from(&alias);
    json!({"before":before,"isolated":isolated,"alias":snapshot(&alias),"root":snapshot(&root),
        "rebound":snapshot(&rebound), "rebound_dirty":rebound.get(&key,"payload").map(|v|v.to_json_value()),
        "alias_version":alias.get_original_version(&key), "root_version":root.get_original_version(&key),
        "alias_dirty":alias.get(&key,"payload").map(|v|v.to_json_value()),
        "alias_names":alias.changed_field_names(&key)})
}

fn checker_helper_probe(input: &Value) -> Value {
    use school_management_service_core::{NoopSchoolChecker, SchoolCheckerLogic};
    use teaql_runtime::{JsonFieldNamingProfile, ObjectLocation};
    let logic = NoopSchoolChecker;
    let mut results = vec![];
    let location = ObjectLocation::array_root(2).member("account_info");
    let field = input["field"].as_str().unwrap();
    let text = input["text"].as_str().unwrap();
    logic.required(
        input["required"].as_bool().unwrap(),
        field,
        &location,
        &mut results,
    );
    let optional = input["optional"].as_bool().unwrap().then_some(0_i64);
    logic.required_option(optional.as_ref(), field, &location, &mut results);
    logic.required_text(text, field, &location, &mut results);
    logic.min_string_length(
        text,
        field,
        input["min"].as_u64().unwrap() as usize,
        &location,
        &mut results,
    );
    logic.max_string_length(
        text,
        field,
        input["max"].as_u64().unwrap() as usize,
        &location,
        &mut results,
    );
    let profiles = [
        JsonFieldNamingProfile::CamelCase,
        JsonFieldNamingProfile::SnakeCase,
        JsonFieldNamingProfile::PascalCase,
    ];
    json!(
        profiles
            .iter()
            .map(|profile| results
                .iter()
                .map(|result| result.to_wire(*profile))
                .collect::<Vec<_>>())
            .collect::<Vec<_>>()
    )
}

fn mutation_envelope_probe(input: &Value) -> Value {
    use teaql_core::{InsertCommand, UpdateCommand, DeleteCommand, RecoverCommand, TraceNode, TraceKind};
    use teaql_data_service::{MutationRequest, MutationCommand};
    let kinds = [TraceKind::Operation, TraceKind::Request, TraceKind::Relation, TraceKind::Entity,
        TraceKind::Provider, TraceKind::Sql, TraceKind::Comment, TraceKind::Purpose, TraceKind::AuditReason];
    let chain = input["trace"].as_array().unwrap().iter().map(|node| TraceNode::typed(
        kinds[node["kind"].as_u64().unwrap() as usize], node["entity"].as_str().unwrap(),
        node["id"].as_u64(), node["comment"].as_str().unwrap())).collect::<Vec<_>>();
    let entity = input["entity"].as_str().unwrap();
    let id = native_value(&input["id"]);
    let command = match input["command"].as_str().unwrap() {
        "insert" => { let mut command = InsertCommand::new(entity); command.values.insert("id".into(),id); command.trace_chain = chain; MutationCommand::Insert(command) },
        "update" => { let mut command = UpdateCommand::new(entity,id); command.trace_chain = chain; MutationCommand::Update(command) },
        "delete" => { let mut command = DeleteCommand::new(entity,id); command.trace_chain = chain; MutationCommand::Delete(command) },
        "recover" => { let mut command = RecoverCommand::new(entity,id,-1); command.trace_chain = chain; MutationCommand::Recover(command) },
        _ => MutationCommand::Batch(Vec::new()),
    };
    let request = if input["via_command"].as_bool().unwrap_or(false) {
        command.request(input["comment"].as_str().unwrap_or(""))
    } else { MutationRequest::new(command,input["comment"].as_str().unwrap_or("")) };
    match request {
        Err(error) => json!({"field":error.field,"code":error.code(),"display":error.to_string()}),
        Ok(request) => {
            let trace = |nodes: &[TraceNode]| nodes.iter().map(|node| {
                let kind = match node.kind { TraceKind::Operation => 0, TraceKind::Request => 1,
                    TraceKind::Relation => 2, TraceKind::Entity => 3, TraceKind::Provider => 4,
                    TraceKind::Sql => 5, TraceKind::Comment => 6, TraceKind::Purpose => 7,
                    TraceKind::AuditReason => 8 };
                json!({"kind":kind,"entity":node.entity_type,"id":node.entity_id,"comment":node.comment})
            }).collect::<Vec<_>>();
            json!({"comment":request.comment(),"trace":trace(request.trace_chain()),
                "execution_trace":trace(&request.execution_trace_chain()),
                "intent_debug":format!("{:?}",request.intent()),"redactions":request.diagnostic_redactions().is_some()})
        }
    }
}

fn request_envelope_probe(input: &Value) -> Value {
    use teaql_core::{QueryIntent, SelectQuery, TraceNode, TraceKind};
    use teaql_data_service::QueryRequest;
    let kinds = [TraceKind::Operation, TraceKind::Request, TraceKind::Relation, TraceKind::Entity,
        TraceKind::Provider, TraceKind::Sql, TraceKind::Comment, TraceKind::Purpose, TraceKind::AuditReason];
    let mut query = SelectQuery::new(input["entity"].as_str().unwrap());
    query.comment = input["comment"].as_str().map(str::to_owned);
    query.purpose = input["purpose"].as_str().map(str::to_owned);
    for node in input["trace"].as_array().unwrap() {
        query.trace_chain.push(TraceNode::typed(kinds[node["kind"].as_u64().unwrap() as usize],
            node["entity"].as_str().unwrap(), node["id"].as_u64(), node["comment"].as_str().unwrap()));
    }
    let request = if input["explicit"].as_bool().unwrap() {
        QueryIntent::from_optional(input["intent_comment"].as_str(), input["intent_purpose"].as_str())
            .map(|intent| QueryRequest::new(query, intent))
    } else { QueryRequest::from_query(query) };
    match request {
        Err(error) => json!({"field":error.field,"code":error.code(),"display":error.to_string()}),
        Ok(mut request) => {
            request.capture_debug_query = input["debug"].as_bool().unwrap();
            request.capture_execution_metadata = input["metadata"].as_bool().unwrap();
            let trace = |nodes: &[TraceNode]| nodes.iter().map(|node| {
                let kind = match node.kind { TraceKind::Operation => 0, TraceKind::Request => 1,
                    TraceKind::Relation => 2, TraceKind::Entity => 3, TraceKind::Provider => 4,
                    TraceKind::Sql => 5, TraceKind::Comment => 6, TraceKind::Purpose => 7,
                    TraceKind::AuditReason => 8 };
                json!({"kind":kind,"entity":node.entity_type,"id":node.entity_id,"comment":node.comment})
            }).collect::<Vec<_>>();
            json!({"comment":request.comment(),"purpose":request.purpose(),
                "query_comment":request.query.comment,"query_purpose":request.query.purpose,
                "trace":trace(&request.trace_chain),"execution_trace":trace(&request.execution_trace_chain()),
                "debug":request.capture_debug_query,"metadata":request.capture_execution_metadata})
        }
    }
}

fn request_intent_probe(input: &Value) -> Value {
    use teaql_core::{QueryIntent, MutationIntent, RequestIntentError, RequestKind};
    let comment = input["comment"].as_str();
    let purpose = input["purpose"].as_str();
    let error = |value: RequestIntentError| {
        let runtime = teaql_runtime::RuntimeError::from(value.clone());
        json!({"field":value.field,"code":value.code(),"display":value.to_string(),
            "debug":format!("{:?}",value),"runtime_display":runtime.to_string(),
            "runtime_debug":format!("{:?}",runtime)})
    };
    let query = match QueryIntent::from_optional(comment, purpose) {
        Ok(value) => json!({"comment":value.comment(),"purpose":value.purpose(),"debug":format!("{:?}",value),"clone_equal":value.clone()==value}),
        Err(value) => error(value),
    };
    let mutation = match MutationIntent::from_optional(comment) {
        Ok(value) => json!({"comment":value.comment(),"debug":format!("{:?}",value),"clone_equal":value.clone()==value}),
        Err(value) => error(value),
    };
    let extra_field = match input["extra_field"].as_str().unwrap() {
        "purpose" => "purpose", "comment" => "comment", _ => "unexpected",
    };
    json!({"query":query,"mutation":mutation,"extra":error(RequestIntentError {
        request_kind: if input["extra_query"].as_bool().unwrap() { RequestKind::Query } else { RequestKind::Mutation }, field:extra_field })})
}

fn trace_tokens_probe(input: &Value) -> Value {
    use std::sync::Arc;
    use teaql_core::{TraceNode, TraceKind};
    use teaql_runtime::TraceScopeToken;
    let mut tokens = Vec::<Arc<TraceScopeToken>>::new();
    for (index, item) in input["nodes"].as_array().unwrap().iter().enumerate() {
        let parent = item["parent"].as_u64().map(|i| tokens[i as usize].clone());
        tokens.push(Arc::new(TraceScopeToken { parent,
            track:TraceNode::typed(TraceKind::AuditReason, item["entity"].as_str().unwrap(),
                item["id"].as_u64(), item["comment"].as_str().unwrap()), node_index:index as u64 }));
    }
    json!(tokens.iter().map(|token| token.recover_trace_chain().iter().map(|track|
        json!({"entity":track.entity_type,"id":track.entity_id,"comment":track.comment}))
        .collect::<Vec<_>>()).collect::<Vec<_>>())
}

fn business_id_probe(input: &Value) -> Value {
    use cpp_business_ids_core::{Receipt, ReceiptNumber, ReceiptChecker, Ticket, TicketNumber, TicketChecker};
    use teaql_core::{Entity, CompactRow, eval::LoadState};
    use teaql_runtime::{JsonFieldNamingProfile, BusinessDate, BusinessIdService, InMemoryBusinessIdAllocator,
        BusinessIdEncodingKey, StaticBusinessIdKeyProvider, CheckObjectStatus, TypedChecker,
        ObjectLocation, UserContext};
    let mut context = UserContext::new();
    context.insert_resource(BusinessDate(chrono::NaiveDate::from_ymd_opt(2026, 10, 11).unwrap()));
    if let Some(root) = input["root"].as_u64() {
        context = context.with_active_root("Tenant", root);
    }
    if input["service"].as_bool().unwrap() {
        let mut service = BusinessIdService::new(InMemoryBusinessIdAllocator::default());
        if input["key"].as_bool().unwrap() {
            let bytes = std::array::from_fn(|i| i as u8);
            service = service.with_key_provider(StaticBusinessIdKeyProvider::new(
                BusinessIdEncodingKey::new(1, bytes).unwrap()));
        }
        context.set_business_id_service(service);
    }
    macro_rules! evaluate {
        ($entity:ty, $number:ty, $checker:ty, $field:ident, $update:ident, $name:literal) => {{
            let definition = <$number>::definition();
            let factory = match <$number>::new(input["text"].as_str().unwrap()) {
                Ok(value) => json!({"ok":value.as_str(), "debug":format!("{:?}",value)}),
                Err(error) => json!({"error":error}),
            };
            let mut row = std::collections::BTreeMap::new();
            row.insert("id".to_owned(), teaql_core::Value::U64(7));
            row.insert("name".to_owned(), teaql_core::Value::Text("entry".to_owned()));
            row.insert("tenant_id".to_owned(), teaql_core::Value::U64(input["tenant"].as_u64().unwrap()));
            row.insert("version".to_owned(), teaql_core::Value::I64(1));
            row.insert($name.to_owned(), teaql_core::Value::Text(input["before"].as_str().unwrap().to_owned()));
            let mut entity = <$entity>::from_compact_row(CompactRow::from(row)).unwrap();
            if !input["loaded"].as_bool().unwrap() {
                entity.set_load_state(LoadState::Partial(["id".to_owned()].into_iter().collect()));
            }
            if input["dirty"].as_bool().unwrap() {
                let text = input["before"].as_str().unwrap();
                entity.$update(<$number>::new(if text.is_empty() { "dirty" } else { text }).unwrap());
            }
            let mut results = Vec::new();
            let status = if input["update"].as_bool().unwrap() { CheckObjectStatus::Update } else { CheckObjectStatus::Create };
            let location = ObjectLocation::array_root(2).member("account_info");
            <$checker>::default().check_and_fix_typed(&context, &mut entity, status, &location, &mut results);
            let diagnostics: Vec<_> = [JsonFieldNamingProfile::CamelCase, JsonFieldNamingProfile::SnakeCase, JsonFieldNamingProfile::PascalCase]
                .iter().map(|profile| results.iter().map(|result| result.to_wire(*profile)).collect::<Vec<_>>()).collect();
            json!({"factory":factory,"definition":{"field":definition.field_name,"profile":definition.profile,
                "prefix":definition.prefix,"namespace":definition.namespace,"split":definition.split_by_date,
                "digits":definition.preserve_digits,"separator":definition.separator},
                "value":entity.$field().as_str(),"loaded":entity.is_loaded($name),
                "tenant":entity.tenant_id(),"diagnostics":diagnostics})
        }};
    }
    if input["entity"].as_str().unwrap() == "receipt" {
        evaluate!(Receipt, ReceiptNumber, ReceiptChecker, receipt_number, update_receipt_number, "receipt_number")
    } else {
        evaluate!(Ticket, TicketNumber, TicketChecker, ticket_number, update_ticket_number, "ticket_number")
    }
}

fn generated_relations_probe(input: &Value) -> Value {
    use school_management_service_core::{Platform, School};
    use teaql_core::{SmartList, CompactRow, Entity, eval::{EvalResult, LoadState}};
    use teaql_runtime::{EntityGraphBuilder, EntityRuntimeState};
    let root = EntityRuntimeState::default();
    let mut school = School::from_compact_row(CompactRow::from(std::collections::BTreeMap::new())).unwrap();
    school.attach_runtime_state_recursive(root.clone());
    school.update_id(1_u64);
    school.update_platform_id(input["foreign_id"].as_u64().unwrap());
    let mut fields = vec!["id".to_owned(), "platform_id".to_owned()];
    if input["selected"].as_bool().unwrap() {
        fields.push("platform".to_owned());
    }
    school.set_load_state(LoadState::Partial(fields.into_iter().collect()));
    let mut platform = Platform::from_compact_row(CompactRow::from(std::collections::BTreeMap::new())).unwrap();
    platform.update_id(9_u64);
    platform.update_name("platform");
    let mut owner = Platform::from_compact_row(CompactRow::from(std::collections::BTreeMap::new())).unwrap();
    owner.attach_runtime_state_recursive(root.clone());
    owner.update_id(9_u64);
    let mut graph = EntityGraphBuilder::default();
    if input["cached"].as_bool().unwrap() {
        graph.install(9, platform.clone());
    }
    match input["forward"].as_str().unwrap() {
        "present" => graph.install_relation_option("School", 1, "platform", Some(platform)),
        "empty" => graph.install_relation_option::<Platform>("School", 1, "platform", None),
        "wrong_type" => graph.install_relation_option("School", 1, "platform", Some(9_u64)),
        _ => {}
    }
    if let Some(count) = input["count"].as_u64() {
        let children: Vec<_> = (0..count).map(|i| {
            let mut child = School::from_compact_row(CompactRow::from(std::collections::BTreeMap::new())).unwrap();
            child.update_id(20 + i);
            child
        }).collect();
        graph.install_relation_list("Platform", 9, "school_list", SmartList::from(children));
    }
    assert!(root.freeze_graph(graph).is_ok());
    let forward = match school.eval_platform() {
        EvalResult::Value(value) => json!({"state":"Value", "id":value.id()}),
        EvalResult::Null => json!({"state":"Null"}),
        EvalResult::NotLoaded {failed_node, attempted_path} => json!({"state":"NotLoaded", "node":failed_node, "path":attempted_path}),
    };
    let list = match owner.eval_school_list() {
        EvalResult::Value(value) => json!({"state":"Value", "ids":value.data.iter().map(|v| v.id()).collect::<Vec<_>>()}),
        EvalResult::Null => json!({"state":"Null"}),
        EvalResult::NotLoaded {failed_node, attempted_path} => json!({"state":"NotLoaded", "node":failed_node, "path":attempted_path}),
    };
    let handle = owner.school_list();
    json!({"forward":forward, "list":list, "handle":format!("{:?}",handle.state()), "loaded":handle.is_loaded(), "empty":handle.is_empty()})
}

fn comment_probe(input: &Value) -> Value {
    use teaql_runtime::{EntityKey, EntityRuntimeState};
    let receiving = EntityRuntimeState::default();
    let source = EntityRuntimeState::default();
    if let Some(reason) = input["receiving_reason"].as_str() {
        receiving.set_comment(reason);
    }
    if let Some(reason) = input["source_reason"].as_str() {
        source.set_comment(reason);
    }
    let apply = |root: &EntityRuntimeState, list: &Value| {
        for item in list.as_array().unwrap() {
            root.set_entity_comment(
                EntityKey::new(item[0].as_str().unwrap(), item[1].as_u64().unwrap()),
                item[2].as_str().unwrap(),
            );
        }
    };
    apply(&receiving, &input["receiving"]);
    apply(&source, &input["source"]);
    // Composition requires source intent, not only loaded rows.
    source.set(EntityKey::new("Item", 7_u64), "quantity", 0_i64);
    receiving.adopt_mutations_from(&source);
    let queries: Vec<_> = input["lookups"]
        .as_array()
        .unwrap()
        .iter()
        .map(|pair| EntityKey::new(pair[0].as_str().unwrap(), pair[1].as_u64().unwrap()))
        .collect();
    let comments: Vec<_> = queries
        .iter()
        .map(|key| receiving.get_entity_comment(key))
        .collect();
    let reason = receiving.get_comment();
    receiving.clear_committed();
    json!({"reason":reason,"comments":comments,"after_reason":receiving.get_comment(),"after_comments":queries.iter().map(|key| receiving.get_entity_comment(key)).collect::<Vec<_>>()})
}
