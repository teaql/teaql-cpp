#![allow(dead_code)]
use std::{io::{self,BufRead},collections::BTreeMap,panic::{catch_unwind,AssertUnwindSafe}};
use serde_json::{json,Value as J};
use teaql_core::{Value,Record,Decimal,DataType,TraceNode,TraceKind};
pub use teaql_runtime::{UserContext,RuntimeError,SqlLogEntry,MutationGovernanceSnapshot};
use teaql_runtime::{EntityKey,MutationPolicyIdentity,MutationOperationSummary,MutationOperationKind,MutationPolicySource,MutationPolicyApprovalStatus};
#[path="../../../../../.reference/teaql-rs/teaql-runtime/src/event.rs"] mod event;
#[path="../../../../../.reference/teaql-rs/teaql-runtime/src/log_privacy.rs"] mod log_privacy;
pub use event::*;
mod context {pub use teaql_runtime::{SqlLogEntry,SqlLogOperation};}
#[path="../formatter_fixture.rs"] mod log_formatter;
use log_formatter::{LogFormatter,HumanReaderFormatter,DebugReaderFormatter};
fn without_time(text:String)->String{let end=text.find("]-").unwrap();text[end+1..].to_owned()}

fn value(j:&J)->Value{match j["kind"].as_str().unwrap(){
 "null"=>Value::Null,"typed_null"=>Value::TypedNull(DataType::Text),"bool"=>Value::Bool(j["value"].as_bool().unwrap()),"text"=>Value::Text(j["value"].as_str().unwrap().into()),"i64"=>Value::I64(j["value"].as_str().unwrap().parse().unwrap()),"u64"=>Value::U64(j["value"].as_str().unwrap().parse().unwrap()),"f64"=>Value::F64(j["value"].as_str().unwrap().parse().unwrap()),"f64_bits"=>Value::F64(f64::from_bits(u64::from_str_radix(j["value"].as_str().unwrap(),16).unwrap())),"decimal"=>Value::Decimal(j["value"].as_str().unwrap().parse::<Decimal>().unwrap()),"date"=>Value::Date(j["value"].as_str().unwrap().parse().unwrap()),"timestamp"=>Value::Timestamp(teaql_core::time::Timestamp(j["value"].as_str().unwrap().parse().unwrap())),"json"=>Value::Json(j["value"].clone()),"object"=>Value::Object(record(&j["value"])),"list"=>Value::List(j["value"].as_array().unwrap().iter().map(value).collect()),_=>panic!("unknown value")}}
fn record(j:&J)->Record{j.as_object().unwrap().iter().map(|(k,v)|(k.clone(),value(v))).collect()}
fn strings(j:&J)->Vec<String>{j.as_array().unwrap().iter().map(|s|s.as_str().unwrap().into()).collect()}
fn optional_record(j:&J)->Option<Record>{if j.is_null(){None}else{Some(record(j))}}
fn count(j:&J)->usize{j.as_str().unwrap().parse().unwrap()}
fn raw(j:&J)->RawAuditEvent{
 let entity=j["entity"].as_str().unwrap();let values=record(&j["values"]);let id=value(&j["id"]);let old=optional_record(&j["old"]);let version=j["version"].as_str().map(|s|s.parse::<i64>().unwrap());
 let mut e=match j["kind"].as_str().unwrap(){"created"=>RawAuditEvent::created(entity,values),"updated"=>RawAuditEvent::updated(entity,values),"updated_old"=>RawAuditEvent::updated_with_old_values(entity,values,old,record(&j["new"]),strings(&j["fields"])),"deleted"=>RawAuditEvent::deleted(entity,id,version),"deleted_old"=>RawAuditEvent::deleted_with_old_values(entity,id,version,old),"recovered"=>RawAuditEvent::recovered(entity,id,version.unwrap()),"recovered_old"=>RawAuditEvent::recovered_with_old_values(entity,id,version.unwrap(),old),"schema_created"=>RawAuditEvent::schema_created(entity,j["table"].as_str().unwrap(),count(&j["count"])),"schema_verified"=>RawAuditEvent::schema_verified(entity,j["table"].as_str().unwrap(),count(&j["count"])),"field_added"=>RawAuditEvent::field_added(entity,j["table"].as_str().unwrap(),j["field"].as_str().unwrap()),"data_seeded"=>RawAuditEvent::data_seeded(entity,j["table"].as_str().unwrap(),count(&j["count"]),count(&j["updated"])),_=>panic!("unknown event")};
 for t in j["trace"].as_array().unwrap(){e.trace_chain.push(TraceNode::typed(TraceKind::AuditReason,t["entity"].as_str().unwrap(),Some(1001),t["comment"].as_str().unwrap()));}
 if j["bootstrap"]==true{e.bootstrap_audit=Some(BootstrapAuditIdentity{actor:"actor secret 1001".into(),category:"category secret".into(),reason:"reason secret 1001".into(),resulting_version:Some(-3),occurred_at_millis:1234});}
 if j["governance"]==true{e.mutation_governance=Some(MutationGovernanceSnapshot{execution_id:"execution secret".into(),request_key:"request secret".into(),source:MutationPolicySource::Customer,policy:Some(MutationPolicyIdentity::new("policy secret","v1","fingerprint")),approval_status:MutationPolicyApprovalStatus::Approved,warning_codes:vec!["warning".into()],operations:vec![MutationOperationSummary{kind:MutationOperationKind::Update,entity:EntityKey::new("Order",Value::U64(1001)),changed_fields:vec!["secret".into(),"field".into()]}]});}e
}
fn record_out(r:&Record)->J{json!(r.iter().map(|(k,v)|(k.clone(),format!("{v:?}"))).collect::<BTreeMap<_,_>>())}
fn trace_out(t:&[TraceNode])->J{json!(t.iter().map(|n|json!({"entity":n.entity_type,"comment":n.comment,"id":n.entity_id.map(|v|v.to_string())})).collect::<Vec<_>>())}
fn governance(g:&Option<MutationGovernanceSnapshot>)->J{g.as_ref().map(|g|json!({"execution":g.execution_id,"request":g.request_key,"source":format!("{:?}",g.source),"approval":format!("{:?}",g.approval_status),"policy":g.policy.as_ref().map(|p|vec![p.id.clone(),p.version.clone(),p.fingerprint.clone()]),"warnings":g.warning_codes,"operations":g.operations.iter().map(|o|json!({"kind":format!("{:?}",o.kind),"entity":o.entity.entity,"id":format!("{:?}",o.entity.id),"fields":o.changed_fields})).collect::<Vec<_>>()})).unwrap_or(J::Null)}
fn raw_out(e:&RawAuditEvent)->J{json!({"kind":format!("{:?}",e.kind),"entity":e.entity,"values":record_out(&e.values),"fields":e.updated_fields,"old":e.old_values.as_ref().map(record_out),"new":e.new_values.as_ref().map(record_out),"changes":e.changes.iter().map(|c|json!({"field":c.field,"old":c.old_value.as_ref().map(|v|format!("{v:?}")),"new":c.new_value.as_ref().map(|v|format!("{v:?}"))})).collect::<Vec<_>>(),"trace":trace_out(&e.trace_chain),"bootstrap":e.bootstrap_audit.as_ref().map(|b|json!({"actor":b.actor,"category":b.category,"reason":b.reason,"version":b.resulting_version,"time":b.occurred_at_millis})),"governance":governance(&e.mutation_governance)})}
fn field_out(f:&SafeAuditField)->J{json!({"name":f.name,"value":f.value,"masked":f.masked,"truncated":f.truncated,"raw_length":f.raw_length,"output_length":f.output_length,"mask_reason":f.mask_reason,"truncate_reason":f.truncate_reason})}
fn evaluate(j:J)->J{
 unsafe{if let Some(s)=j["ack"].as_str(){std::env::set_var("TEAQL_ALLOW_SENSITIVE_PLAINTEXT_LOGS",s);}else{std::env::remove_var("TEAQL_ALLOW_SENSITIVE_PLAINTEXT_LOGS");}}
 match j["op"].as_str().unwrap(){
 "debug"=>json!(format!("{:?}",value(&j["value"]))),
 "key"=>{let mut a=EntityKey::new(j["a_entity"].as_str().unwrap(),value(&j["a"]));let b=EntityKey::new(j["b_entity"].as_str().unwrap(),value(&j["b"]));if let Some(v)=j.get("mutated"){a.id=value(v);}let copied=a.clone();json!({"eq":copied==b,"cmp":match copied.cmp(&b){std::cmp::Ordering::Less=>-1,std::cmp::Ordering::Equal=>0,std::cmp::Ordering::Greater=>1}})},
 "identity"=>{match catch_unwind(AssertUnwindSafe(||MutationPolicyIdentity::new(j["id"].as_str().unwrap(),j["version"].as_str().unwrap(),j["fingerprint"].as_str().unwrap()))){Ok(p)=>json!({"id":p.id,"version":p.version,"fingerprint":p.fingerprint}),Err(e)=>json!({"panic":e.downcast_ref::<&str>().map(|s|s.to_string()).or_else(||e.downcast_ref::<String>().cloned()).unwrap()})}},
 "field"=>field_out(&build_safe_audit_field(j["name"].as_str().unwrap(),j["raw"].as_str(),&strings(&j["masks"]),j["max"].as_u64().map(|n|n as usize))),
 "limit"=>{let (text,truncated)=limit_audit_value(j["raw"].as_str().unwrap(),count(&j["max"]));json!({"text":text,"truncated":truncated})},
 _=>{let e=raw(&j);if j["formatter"]==true{return json!({"human":without_time(HumanReaderFormatter.format_audit_log(&e)),"human_sensitive":without_time(HumanReaderFormatter.format_sensitive_audit_log(&e)),"debug":DebugReaderFormatter.format_audit_log(&e),"debug_sensitive":DebugReaderFormatter.format_sensitive_audit_log(&e)});}let before=raw_out(&e);let safe=e.build_safe_event(&strings(&j["masks"]),j["max"].as_u64().map(|n|n as usize));json!({"raw":before,"unchanged":raw_out(&e)==before,"safe":{"kind":format!("{:?}",safe.kind),"entity":safe.entity,"fields":safe.fields.iter().map(field_out).collect::<Vec<_>>(),"trace":trace_out(&safe.trace_chain),"governance":governance(&safe.mutation_governance)},"projection":raw_out(&log_privacy::audit_event(&e,j["allow"].as_bool().unwrap()))})}
 }
}
fn validate_counts_32(j: &J) -> J {
    // The native oracle remains 64-bit. Parse the same fixture decimal strings
    // into u32 to observe the exact 32-bit usize input range before source calls.
    let fits = |key: &str| j[key].as_str().unwrap().parse::<u32>().is_ok();
    let accepted = match j["kind"].as_str().unwrap() {
        "schema_created" | "schema_verified" => fits("count"),
        "data_seeded" => fits("count") && fits("updated"),
        _ => true,
    };
    json!({"accepted": accepted})
}
fn main() {
    std::panic::set_hook(Box::new(|_| {}));
    let validate = std::env::args().nth(1).as_deref() == Some("--validate-counts-32");
    for line in io::stdin().lock().lines() {
        let input = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", if validate { validate_counts_32(&input) } else { evaluate(input) });
    }
}
