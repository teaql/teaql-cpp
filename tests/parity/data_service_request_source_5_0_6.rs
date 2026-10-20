//! Observe request-owned intent and statement-local trace through the real source APIs.
use serde_json::{Value as Json, json};
use std::io::{self, BufRead};
use teaql_core::*;
use teaql_data_service::*;
#[path = "rust_values/src/fixture_value.rs"]
mod fixture_value;

fn trace(input: &Json) -> Vec<TraceNode> {
    let kinds = [TraceKind::Operation, TraceKind::Request, TraceKind::Relation,
        TraceKind::Entity, TraceKind::Provider, TraceKind::Sql,
        TraceKind::Comment, TraceKind::Purpose, TraceKind::AuditReason];
    input["trace"].as_array().unwrap().iter().map(|node| TraceNode::typed(
        kinds[node["kind"].as_u64().unwrap() as usize], node["name"].as_str().unwrap(),
        node["id"].as_str().map(|value|value.parse().unwrap()),
        node["comment"].as_str().unwrap())).collect()
}

fn observe(input: Json) -> Json {
    let entity = input["entity"].as_str().unwrap();
    let comment = input["comment"].as_str().unwrap();
    let frames = trace(&input);
    if input["kind"] == "query" {
        let mut query = SelectQuery::new(entity).comment("builder comment").purpose("builder purpose");
        query.trace_chain = frames;
        let mut request = QueryRequest::new(query,
            QueryIntent::new(comment,input["purpose"].as_str().unwrap()).unwrap());
        request.capture_debug_query = input["debug"].as_bool().unwrap();
        request.capture_execution_metadata = input["metadata"].as_bool().unwrap();
        let before = format!("{:?}",request.trace_chain);
        let execution = format!("{:?}",request.execution_trace_chain());
        request.query.entity = "ChangedBuilder".into();
        request.query.comment = Some("changed builder comment".into());
        request.query.purpose = Some("changed builder purpose".into());
        request.query.trace_chain.clear();
        return json!({"stored_before":before,"stored_after":format!("{:?}",request.trace_chain),
            "execution":execution,"after_builder_change":format!("{:?}",request.execution_trace_chain()),
            "comment":request.comment(),"purpose":request.purpose(),
            "debug":request.capture_debug_query,"metadata":request.capture_execution_metadata});
    }
    let id = fixture_value::value(&input["id"]);
    let mut command = match input["kind"].as_str().unwrap() {
        "insert" => {
            let mut value = InsertCommand::new(entity);
            if input["include_id"] == true { value.values.insert("id".into(),id); }
            MutationCommand::Insert(value)
        },
        "update" => MutationCommand::Update(UpdateCommand::new(entity,id)),
        "delete" => MutationCommand::Delete(DeleteCommand::new(entity,id)),
        "recover" => MutationCommand::Recover(RecoverCommand::new(entity,id,-3)),
        "batch" => {
            let child = MutationCommand::Insert(InsertCommand::new("Child").value("id",7_u64))
                .request("child owned reason").unwrap();
            let nested = MutationCommand::Batch(vec![child.clone()]).request("nested owned reason").unwrap();
            MutationCommand::Batch(vec![child,nested])
        },
        _ => panic!("Unknown request observation"),
    };
    match &mut command {
        MutationCommand::Insert(value) => value.trace_chain = frames,
        MutationCommand::Update(value) => value.trace_chain = frames,
        MutationCommand::Delete(value) => value.trace_chain = frames,
        MutationCommand::Recover(value) => value.trace_chain = frames,
        MutationCommand::Batch(_) => {},
    }
    let request = command.request(comment).unwrap();
    let before = format!("{:?}",request.trace_chain());
    let execution = format!("{:?}",request.execution_trace_chain());
    let children = match &request.command {
        MutationCommand::Batch(children) => children.iter()
            .map(|child|format!("{:?}",child.execution_trace_chain())).collect::<Vec<_>>(),
        _ => Vec::new(),
    };
    json!({"stored_before":before,"stored_after":format!("{:?}",request.trace_chain()),
        "execution":execution,"comment":request.comment(),"children":children})
}

fn main() {
    for line in io::stdin().lock().lines() {
        println!("{}",observe(serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
