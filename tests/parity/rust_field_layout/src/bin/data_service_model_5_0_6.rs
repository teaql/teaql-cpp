//! Original data-service models and inherited derived diagnostics.
use serde_json::{Value as Json, json};
use std::{collections::BTreeMap, io::{self, BufRead}, time::{Duration, SystemTime}};
use teaql_core::*;
use teaql_data_service::*;
#[path = "../../../rust_values/src/fixture_value.rs"]
mod fixture_value;

fn formatted<T: std::fmt::Debug>(value: &T) -> Json {
    json!({"compact":format!("{value:?}"),"pretty":format!("{value:#?}"),
        "lower_hex":format!("{value:x?}"),"upper_hex":format!("{value:X?}"),
        "pretty_lower_hex":format!("{value:#x?}"),"pretty_upper_hex":format!("{value:#X?}"),
        "width_precision":format!("{value:>40.2?}"),"fancy":format!("{value:🧊^+18.3?}"),
        "zero_pad":format!("{value:+012?}")})
}
fn time(input: &Json) -> SystemTime {
    let ns: i64 = input.as_str().unwrap().parse().unwrap();
    let duration = Duration::from_nanos(ns.unsigned_abs());
    if ns < 0 { SystemTime::UNIX_EPOCH - duration } else { SystemTime::UNIX_EPOCH + duration }
}
fn evaluate(input: Json) -> Json {
    let flags = input["flags"].as_u64().unwrap();
    let text = input["text"].as_str().unwrap();
    let payload = fixture_value::value(&input["value"]);
    let capabilities = DataServiceCapabilities {query:flags&1!=0,mutation:flags&2!=0,
        transaction:flags&4!=0,schema:flags&8!=0,id_generation:flags&16!=0,
        batch_mutation:flags&32!=0,returning:flags&64!=0,small_parent_relation_probes:flags&128!=0};
    let trace = TraceNode::typed(TraceKind::Comment,"Device",Some(u64::MAX),text);
    let traces = if flags&1!=0 {vec![trace.clone(),TraceNode::typed(TraceKind::AuditReason,"Device",None,"")]} else {vec![]};
    let mut query = SelectQuery::new("Device").comment(text);
    query.filter = Some(Expr::eq("payload", payload.clone()));
    let intent = match QueryIntent::new(input["comment"].as_str().unwrap_or(text),
                                      "inspect data-service model fixture") {
        Ok(intent) => intent,
        Err(error) => return json!({"intent_error":{"code":error.code(),"field":error.field,
                                                  "message":error.to_string()}}),
    };
    let request = QueryRequest {query,trace_chain:traces.clone(),intent,
        capture_debug_query:flags&2!=0,capture_execution_metadata:flags&4!=0};
    let mut metadata = ExecutionMetadata::unrecorded_query(input["count"].as_u64().unwrap() as usize);
    let unrecorded = formatted(&metadata);
    let operations = [DataServiceOperation::Query,DataServiceOperation::Insert,DataServiceOperation::Update,
        DataServiceOperation::Delete,DataServiceOperation::Recover,DataServiceOperation::Batch,DataServiceOperation::Schema];
    metadata.operation = operations[flags as usize%operations.len()];
    metadata.backend = text.into();
    metadata.started_at = time(&input["start"]); metadata.ended_at = time(&input["end"]);
    metadata.affected_rows = Some(u64::MAX); metadata.trace_chain = traces.clone();
    metadata.comment = Some(request.comment().to_owned()); metadata.backend_request_id = Some(text.into());
    metadata.parameterized_query = Some("SELECT ?".into()); metadata.params = vec![payload.clone()];
    metadata.debug_query = Some(text.into());
    metadata.sql_log.generated_sql = flags&8!=0;
    metadata.sql_log.parameter_policies = vec![SqlParameterLogPolicy::Credential];
    metadata.sql_log.masked_parameters = vec![true,false];
    if flags&16!=0 {
        let mut child=ExecutionMetadata::unrecorded_query(3); child.backend="child".into();
        child.params=vec![payload.clone()]; metadata.statements.push(child);
    }
    let row = CompactRow::new(vec!["payload".into(),"id".into()].into(),vec![payload.clone(),Value::U64(u64::MAX)]);
    let result = QueryResult {rows:vec![row.clone()],metadata:metadata.clone()};
    let mut insert=InsertCommand::new("Device"); insert.values.insert("payload".into(),payload.clone());insert.trace_chain=traces.clone();
    let mut update=UpdateCommand::new("Device",payload.clone());update.trace_chain=traces.clone();
    let mut delete=DeleteCommand::new("Device",payload.clone());delete.trace_chain=traces.clone();
    let mut recover=RecoverCommand::new("Device",payload.clone(),-1);recover.trace_chain=traces.clone();
    let mutation_request = |command| MutationRequest::new(command, "inspect data-service mutation fixture").unwrap();
    let requests=vec![mutation_request(MutationCommand::Insert(insert)),mutation_request(MutationCommand::Update(update)),
        mutation_request(MutationCommand::Delete(delete)),mutation_request(MutationCommand::Recover(recover)),
        mutation_request(MutationCommand::Batch(vec![])),
        mutation_request(MutationCommand::Batch(vec![mutation_request(MutationCommand::Batch(vec![]))]))];
    let guarded=GuardedMutationRequest::new(requests[0].clone(),Expr::eq("owner",7u64));
    let generated=GeneratedValues::from(BTreeMap::from([("payload".into(),payload.clone())]));
    let snapshot=if flags&32!=0 {Some(EntitySnapshot::from(BTreeMap::from([("payload".into(),payload.clone())])))} else {None};
    let mutation_result=MutationResult {affected_rows:u64::MAX,generated_values:generated,persisted_snapshot:snapshot,metadata:metadata.clone()};
    let chunk=StreamChunk {rows:vec![row],chunk_index:input["count"].as_u64().unwrap() as usize,is_last:flags&64!=0};
    let schema=SchemaRequest {entity_name:text.into()}; let schema_result=SchemaResult {changed:flags&128!=0};
    json!({"capabilities":formatted(&capabilities),"default_capabilities":formatted(&DataServiceCapabilities::default()),
        "request":formatted(&request),"request_clone":formatted(&request.clone()),"metadata":formatted(&metadata),
        "metadata_clone":formatted(&metadata.clone()),"unrecorded":unrecorded,"result":formatted(&result),
        "requests":requests.iter().map(formatted).collect::<Vec<_>>(),
        "comments":requests.iter().map(|r|r.comment()).collect::<Vec<_>>(),
        "traces":requests.iter().map(|r|formatted(&r.trace_chain())).collect::<Vec<_>>(),
        "guarded":formatted(&guarded),"mutation_result":formatted(&mutation_result),
        "chunk":formatted(&chunk),"schema":formatted(&schema),"schema_result":formatted(&schema_result),
        "operations":operations.iter().map(formatted).collect::<Vec<_>>()})
}
fn main() {
    for line in io::stdin().lock().lines() {
        println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
