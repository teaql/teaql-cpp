#![allow(dead_code)]
use std::{io::{self,BufRead},time::{UNIX_EPOCH,Duration}};
use serde_json::{json,Value as J};
use teaql_core::{Value,Record,Decimal,DataType,TraceNode,TraceKind};
pub use teaql_runtime::{UserContext,RuntimeError,SqlLogEntry,MutationGovernanceSnapshot};
use teaql_runtime::SqlLogOperation;
use teaql_data_service::{SqlLogContext,SqlParameterLogPolicy,SqlExecutionOutcome,SqlProjectionState,SqlIntentRedactions};
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

fn txt(j:&J)->Option<String>{j.as_str().map(str::to_owned)}
fn num(j:&J)->u64{j.as_str().unwrap().parse().unwrap()}
fn time(j:&J)->std::time::SystemTime{let n=j.as_str().unwrap().parse::<i64>().unwrap();if n>=0{UNIX_EPOCH+Duration::from_nanos(n as u64)}else{UNIX_EPOCH-Duration::from_nanos(n.unsigned_abs())}}
fn policy(j:&J)->SqlParameterLogPolicy{[SqlParameterLogPolicy::Unknown,SqlParameterLogPolicy::Plain,SqlParameterLogPolicy::Masked,SqlParameterLogPolicy::Credential][num(j) as usize]}
fn evaluate(j:J)->J{
 let kinds=[TraceKind::Operation,TraceKind::Request,TraceKind::Relation,TraceKind::Entity,TraceKind::Provider,TraceKind::Sql,TraceKind::Comment,TraceKind::Purpose,TraceKind::AuditReason];
 let mut e=SqlLogEntry{
 log_context:SqlLogContext{database_kind:txt(&j["database"]),mode:txt(&j["mode"]),omission_reason:txt(&j["omission"]),generated_sql:j["generated"].as_bool().unwrap(),parameter_policies:j["policies"].as_array().unwrap().iter().map(policy).collect(),masked_parameters:j["masked"].as_array().unwrap().iter().map(|v|v.as_bool().unwrap()).collect(),execution_outcome:j["outcome"].as_str().map(|s|[SqlExecutionOutcome::Success,SqlExecutionOutcome::Failure,SqlExecutionOutcome::Cancelled][s.parse::<usize>().unwrap()]),..Default::default()},
 operation:[SqlLogOperation::Select,SqlLogOperation::Insert,SqlLogOperation::Update,SqlLogOperation::Delete,SqlLogOperation::Recover][num(&j["operation"]) as usize],
 comment:txt(&j["comment"]),purpose:txt(&j["purpose"]),audit_reason:txt(&j["reason"]),result_type:txt(&j["type"]),
 sql:j["sql"].as_str().unwrap().into(),debug_sql:j["debug"].as_str().unwrap().into(),pretty_sql:j["pretty"].as_str().unwrap().into(),params:j["params"].as_array().unwrap().iter().map(value).collect(),
 trace_path:j["trace"].as_array().unwrap().iter().map(|n|TraceNode::typed(kinds[num(&n["kind"]) as usize],n["entity"].as_str().unwrap(),Some(1001),n["comment"].as_str().unwrap())).collect(),
 started_at:time(&j["start"]),ended_at:time(&j["end"]),elapsed:Duration::new(num(&j["seconds"]),num(&j["nanos"]) as u32),result_count:j["count"].as_str().map(|s|s.parse().unwrap()),affected_rows:j["affected"].as_str().map(|s|s.parse().unwrap()),result_summary:j["summary"].as_str().unwrap().into()};
 for v in j["intent_ids"].as_array().unwrap(){e.log_context.intent_redactions.capture_target_id(&value(v));}
 if j["intent_bindings"]==true{e.log_context.intent_redactions.extend(&SqlIntentRedactions::from_bindings(&e.log_context,&e.params,&e.sql));}
 if j["formatter"]==true{let trace=if j["empty_trace"]==true{&[][..]}else{&e.trace_path};return json!({"human":without_time(HumanReaderFormatter.format_sql_log(trace,&e)),"debug":DebugReaderFormatter.format_sql_log(trace,&e)});}
 let original=format!("{e:?}");let mut projections=vec![];
 for step in j["steps"].as_array().unwrap(){if let Some(p)=step.get("patch"){let v=&p["value"];match p["name"].as_str().unwrap(){"sql"=>e.sql=v.as_str().unwrap().into(),"debug"=>e.debug_sql=v.as_str().unwrap().into(),"pretty"=>e.pretty_sql=v.as_str().unwrap().into(),"comment"=>e.comment=txt(v),"summary"=>e.result_summary=v.as_str().unwrap().into(),"mode"=>e.log_context.mode=txt(v),"param"=>e.params[0]=value(v),"start"=>e.started_at+=Duration::from_nanos(1),"count"=>e.result_count=Some(9),"intent"=>e.log_context.intent_redactions.capture_target_id(&value(v)),"policy"=>e.log_context.parameter_policies[0]=policy(v),"masked"=>e.log_context.masked_parameters[0]=v.as_bool().unwrap(),"trace"=>e.trace_path[0].comment=v.as_str().unwrap().into(),"clear_state"=>e.log_context.projection_state=SqlProjectionState::default(),_=>panic!("unknown patch")}}
 let before=format!("{e:?}");let projected=log_privacy::sql_entry(&e,step["allow"].as_bool().unwrap());assert_eq!(before,format!("{e:?}"));e=projected;projections.push(format!("{e:?}"));}
 json!({"original":original,"projections":projections})
}
fn main(){for line in io::stdin().lock().lines(){let j=serde_json::from_str(&line.unwrap()).unwrap();println!("{}",evaluate(j));}}
