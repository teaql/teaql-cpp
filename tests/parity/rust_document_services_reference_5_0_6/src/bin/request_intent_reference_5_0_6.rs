//! Byte-preserved historical public observers, linked with exact reference dependencies.
use serde_json::{Value, json};
use std::io::{self, BufRead};

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

fn main() {
    for line in io::stdin().lock().lines() {
        let input: Value = serde_json::from_str(&line.unwrap()).unwrap();
        let output = match input["kind"].as_str().unwrap() {
            "request_intent" => request_intent_probe(&input),
            "trace_tokens" => trace_tokens_probe(&input),
            "request_envelope" => request_envelope_probe(&input),
            "mutation_envelope" => mutation_envelope_probe(&input),
            _ => panic!("Unrecognized intent observation"),
        };
        println!("{}", output);
    }
}
