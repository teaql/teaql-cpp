//! Original public raw/safe audit event model constructors, derives and projection.
use serde_json::{json,Value as J};
use std::{collections::BTreeMap,io::{self,BufRead}};
use teaql_core::{Value,Record,TraceNode,TraceKind};
use teaql_runtime::*;
#[path="../../../rust_values/src/fixture_value.rs"] mod fixture_value;
fn formatted<T:std::fmt::Debug>(v:&T)->J {json!({"compact":format!("{v:?}"),"pretty":format!("{v:#?}"),"lower_hex":format!("{v:x?}"),"upper_hex":format!("{v:X?}"),"pretty_lower_hex":format!("{v:#x?}"),"pretty_upper_hex":format!("{v:#X?}"),"width_precision":format!("{v:>40.2?}"),"fancy":format!("{v:🧊^+18.3?}"),"zero_pad":format!("{v:+012?}")})}
fn record(j:&J)->Record {j.as_object().unwrap().iter().map(|(k,v)|(k.clone(),fixture_value::value(v))).collect::<BTreeMap<_,_>>()}
fn strings(j:&J)->Vec<String> {j.as_array().unwrap().iter().map(|v|v.as_str().unwrap().to_owned()).collect()}
fn raw(j:&J)->RawAuditEvent {
 let values=record(&j["values"]);let old=j["old"].as_object().map(|_|record(&j["old"]));let entity=j["entity"].as_str().unwrap();let id=fixture_value::value(&j["id"]);let version=j["version"].as_str().map(|v|v.parse::<i64>().unwrap());let count=j["count"].as_u64().unwrap() as usize;
 let mut event=match j["kind"].as_str().unwrap(){
 "created"=>RawAuditEvent::created(entity,values),"updated"=>RawAuditEvent::updated(entity,values),
 "updated_old"=>RawAuditEvent::updated_with_old_values(entity,values,old,record(&j["new"]),strings(&j["fields"])),
 "deleted"=>RawAuditEvent::deleted(entity,id,version),"deleted_old"=>RawAuditEvent::deleted_with_old_values(entity,id,version,old),
 "recovered"=>RawAuditEvent::recovered(entity,id,version.unwrap()),"recovered_old"=>RawAuditEvent::recovered_with_old_values(entity,id,version.unwrap(),old),
 "schema_created"=>RawAuditEvent::schema_created(entity,"orders",count),"schema_verified"=>RawAuditEvent::schema_verified(entity,"orders",count),
 "field_added"=>RawAuditEvent::field_added(entity,"orders","name"),"data_seeded"=>RawAuditEvent::data_seeded(entity,"orders",count,2),_=>panic!("unknown event constructor")};
 if j["trace"]==true {event.trace_chain=vec![TraceNode::typed(TraceKind::AuditReason,"Order",Some(1001),"reason secret 1001"),TraceNode::typed(TraceKind::Entity,"Order",None,"trace🧊")];}
 if j["bootstrap"]==true {event.bootstrap_audit=Some(BootstrapAuditIdentity{actor:"actor🧊".into(),category:"category".into(),reason:"bootstrap reason".into(),resulting_version:Some(-3),occurred_at_millis:u64::MAX});}
 if j["governance"]==true {event.mutation_governance=Some(MutationGovernanceSnapshot{execution_id:"execution".into(),request_key:"Order.saveGraph".into(),source:MutationPolicySource::Customer,policy:Some(MutationPolicyIdentity::new("policy","v1","fingerprint")),approval_status:MutationPolicyApprovalStatus::Approved,warning_codes:vec!["warning".into()],operations:vec![MutationOperationSummary{kind:MutationOperationKind::Update,entity:EntityKey::new("Order",Value::U64(1001)),changed_fields:vec!["secret".into(),"name".into()]}]});}
 event
}
fn evaluate(j:J)->J {
 unsafe {if j["plaintext"]==true {std::env::set_var("TEAQL_ALLOW_SENSITIVE_PLAINTEXT_LOGS","I_UNDERSTAND_SENSITIVE_DATA_MAY_BE_WRITTEN_TO_DISK");} else {std::env::remove_var("TEAQL_ALLOW_SENSITIVE_PLAINTEXT_LOGS");}}
 let value=raw(&j);let clone=value.clone();let mut other=value.clone();other.entity.push('!');let safe=value.build_safe_event(&strings(&j["mask"]),j["maximum"].as_u64().map(|v|v as usize));let safe_clone=safe.clone();let mut safe_other=safe.clone();safe_other.entity.push('!');
 json!({"raw":formatted(&value),"raw_clone":formatted(&clone),"raw_clone_equal":value==clone,"raw_changed_equal":value==other,"safe":formatted(&safe),"safe_clone":formatted(&safe_clone),"safe_clone_equal":safe==safe_clone,"safe_changed_equal":safe==safe_other})
}
fn main(){for line in io::stdin().lock().lines(){println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap()));}}
