#![allow(dead_code)]
use std::{io::{self,BufRead},time::{UNIX_EPOCH,Duration}};
use serde_json::Value as J;
use teaql_core::{Value,Record,TraceNode,TraceKind};
pub use teaql_runtime::{UserContext,RuntimeError,SqlLogEntry,MutationGovernanceSnapshot};
use teaql_runtime::SqlLogOperation;
use teaql_data_service::{SqlLogContext,SqlParameterLogPolicy as P,SqlExecutionOutcome};
#[path="../../../../../.reference/teaql-rs/teaql-runtime/src/event.rs"] mod event;
#[path="../../../../../.reference/teaql-rs/teaql-runtime/src/log_privacy.rs"] mod log_privacy;
pub use event::*;
mod context {pub use teaql_runtime::{SqlLogEntry,SqlLogOperation};}
#[path="../formatter_fixture.rs"] mod log_formatter;
fn environment(j:&J){for(k,v) in j.as_object().unwrap(){unsafe{if let Some(s)=v.as_str(){std::env::set_var(k,s);}else{std::env::remove_var(k);}}}}
fn business_value(j:&J)->String{j["large"].as_str().map(|n|"密".repeat(n.parse::<usize>().unwrap())).unwrap_or("private-school-name".into())}
fn sql_entry(j:&J)->SqlLogEntry{let mut e=SqlLogEntry{log_context:SqlLogContext{database_kind:Some("sqlite".into()),generated_sql:true,parameter_policies:vec![P::Masked,P::Credential,P::Plain],execution_outcome:Some(SqlExecutionOutcome::Success),..Default::default()},operation:SqlLogOperation::Update,comment:Some("change private-school-name secret-token 7".into()),purpose:None,audit_reason:None,trace_path:vec![TraceNode::typed(TraceKind::Comment,"Order",Some(7),"change private-school-name secret-token 7")],sql:"UPDATE orders SET name = ?, password = ? WHERE id = ?".into(),params:vec![Value::Text(business_value(j)),Value::Text("secret-token".into()),Value::U64(7)],debug_sql:"untrusted private-school-name secret-token".into(),pretty_sql:"untrusted private-school-name secret-token".into(),started_at:UNIX_EPOCH,ended_at:UNIX_EPOCH,elapsed:Duration::from_nanos(1500),result_count:None,result_type:None,affected_rows:Some(1),result_summary:"1 rows affected".into()};e.log_context.intent_redactions.capture_target_id(&e.params[2]);if j["invalid"]==true{e.sql="SELECT ?4".into();}if j["safe"]==true{return log_privacy::sql_entry(&e,false);}e}
fn main(){let line=io::stdin().lock().lines().next().unwrap().unwrap();let input:J=serde_json::from_str(&line).unwrap();environment(&input["env"]);for step in input["steps"].as_array().unwrap(){if let Some(delta)=step.get("env"){environment(delta);}match step["op"].as_str().unwrap(){
"config"=>{let _=log_formatter::LogManager::config();},"warning"=>{let _=log_privacy::plaintext_enabled();},"mkdir"=>{std::fs::create_dir(step["path"].as_str().unwrap()).unwrap();},"field"|"credential"|"empty_field"=>{let op=step["op"].as_str().unwrap();let _=build_safe_audit_field(if op=="credential"{"password"}else{"name"},if op=="empty_field"{None}else{Some("private-school-name")},&["name".into()],None);},
"sql"|"sensitive_sql"=>{let e=sql_entry(step);let before=format!("{e:?}");if step["op"]=="sql"{log_formatter::LogManager::write_sql_log(&e.trace_path,&e);}else{log_formatter::sensitive_sql(&e.trace_path,&e);}assert_eq!(before,format!("{e:?}"));},
_=>{let mut e=RawAuditEvent::created("Order",Record::from([("id".into(),Value::U64(7)),("name".into(),Value::Text(business_value(step))),("password".into(),Value::Text("secret-token".into()))]));e.trace_chain.push(TraceNode::typed(TraceKind::AuditReason,"Order",Some(7),"change private-school-name secret-token 7"));let before=format!("{e:?}");if step["op"]=="event_projection"{let _=e.build_safe_event(&[],None);}else{log_formatter::LogManager::write_audit_log(&e);}assert_eq!(before,format!("{e:?}"));}
}}}
