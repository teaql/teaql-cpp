#![allow(dead_code)]
use std::{io::{self,BufRead},sync::Mutex,any::Any,time::{UNIX_EPOCH,Duration}};
use serde_json::{json,Value as J};
use teaql_core::{Value,TraceNode,TraceKind};
pub use teaql_runtime::{RuntimeError,MutationGovernanceSnapshot};
// The private logging impl is compiled unchanged against its field/resource
// shell. Resource registration itself has separate live-runtime parity tests.
pub struct UserContext{sql_log_options:context::SqlLogOptions,sql_log_entries:Mutex<Vec<SqlLogEntry>>,user_identifier:Option<String>,buffer:context::UnifiedLogBuffer}
impl UserContext{fn get_resource<T:Any>(&self)->Option<&T>{(&self.buffer as &dyn Any).downcast_ref::<T>()}}
mod context{pub use logging::*;mod logging{include!(concat!(env!("CARGO_MANIFEST_DIR"),"/../../../.reference/teaql-rs/teaql-runtime/src/context/logging.rs"));}pub use crate::UserContext;}
pub use context::{SqlLogEntry,SqlLogOperation};
#[path="../../../../../.reference/teaql-rs/teaql-runtime/src/event.rs"] mod event;
pub use event::*;
#[path="../../../../../.reference/teaql-rs/teaql-runtime/src/log_privacy.rs"] mod log_privacy;
#[path="../formatter_fixture.rs"] mod log_formatter;
fn n(j:&J)->u64{j.as_str().unwrap().parse().unwrap()}
fn text(j:&J)->Option<String>{j.as_str().map(str::to_owned)}
fn time(j:&J)->std::time::SystemTime{
    if j.is_object(){let s=j["seconds"].as_str().unwrap().parse::<i64>().unwrap();
        let base=if s<0{UNIX_EPOCH.checked_sub(Duration::from_secs(s.unsigned_abs())).unwrap()}
            else{UNIX_EPOCH.checked_add(Duration::from_secs(s as u64)).unwrap()};
        return base.checked_add(Duration::from_nanos(j["nanos"].as_u64().unwrap())).unwrap();}
    let n=j.as_str().unwrap().parse::<i64>().unwrap();if n>=0{UNIX_EPOCH+Duration::from_nanos(n as u64)}else{UNIX_EPOCH-Duration::from_nanos(n.unsigned_abs())}}
fn value(j:&J)->Value{match j["kind"].as_str().unwrap(){"null"=>Value::Null,"text"=>Value::Text(j["value"].as_str().unwrap().into()),"u64"=>Value::U64(n(&j["value"])),"i64"=>Value::I64(j["value"].as_str().unwrap().parse().unwrap()),_=>panic!("fixture value")}}
fn metadata(j:&J)->teaql_data_service::ExecutionMetadata{use teaql_data_service::{DataServiceOperation as O,SqlParameterLogPolicy as P};let kinds=[TraceKind::Operation,TraceKind::Request,TraceKind::Relation,TraceKind::Entity,TraceKind::Provider,TraceKind::Sql,TraceKind::Comment,TraceKind::Purpose,TraceKind::AuditReason];teaql_data_service::ExecutionMetadata{
 statements:j["children"].as_array().unwrap().iter().map(metadata).collect(),backend:j["backend"].as_str().unwrap().into(),operation:[O::Query,O::Insert,O::Update,O::Delete,O::Recover,O::Batch,O::Schema][n(&j["operation"]) as usize],started_at:time(&j["start"]),ended_at:time(&j["end"]),affected_rows:j["affected"].as_str().map(|s|s.parse().unwrap()),result_count:j["count"].as_str().map(|s|s.parse().unwrap()),trace_chain:j["trace"].as_array().unwrap().iter().map(|t|TraceNode::typed(kinds[n(&t["kind"]) as usize],t["entity"].as_str().unwrap(),Some(1001),t["comment"].as_str().unwrap())).collect(),comment:text(&j["comment"]),backend_request_id:None,parameterized_query:text(&j["sql"]),debug_query:text(&j["debug"]),params:j["params"].as_array().unwrap().iter().map(value).collect(),sql_log:teaql_data_service::SqlLogContext{database_kind:text(&j["database"]),generated_sql:j["generated"].as_bool().unwrap(),parameter_policies:j["policies"].as_array().unwrap().iter().map(|v|[P::Unknown,P::Plain,P::Masked,P::Credential][n(v) as usize]).collect(),..Default::default()}}}
fn timestamp(t:std::time::SystemTime)->String{match t.duration_since(UNIX_EPOCH){Ok(d)=>d.as_nanos().to_string(),Err(e)=>format!("-{}",e.duration().as_nanos())}}
fn snapshot(c:&UserContext)->J{let entries=c.sql_logs().iter().map(|e|format!("{e:?}")).collect::<Vec<_>>();let unified=c.buffer.entries.lock().unwrap().iter().map(|e|{let context::LogPayload::Sql(s)=&e.payload else{panic!("fixture payload")};json!({"entry":format!("{s:?}"),"user":e.user_identifier,"timestamp":timestamp(e.timestamp),"trace_matches":e.trace_chain==s.trace_path})}).collect::<Vec<_>>();json!({"entries":entries,"unified":unified,"select":c.sql_log_options().select,"mutation":c.sql_log_options().mutation})}
fn evaluate(j:J)->J{let mut c=UserContext{sql_log_options:Default::default(),sql_log_entries:Mutex::new(vec![]),user_identifier:None,buffer:Default::default()};let mut states=vec![];for s in j["steps"].as_array().unwrap(){match s["op"].as_str().unwrap(){"record"=>c.record_metadata_log(&metadata(&s["metadata"])),"clear"=>c.clear_sql_logs(),"disable"=>c.disable_sql_log(),"all"=>c.enable_all_sql_log(),"select_on"=>c.enable_select_sql_log(),"select_off"=>c.disable_select_sql_log(),"mutation_on"=>c.enable_mutation_sql_log(),"mutation_off"=>c.disable_mutation_sql_log(),"options"=>c.set_sql_log_options(context::SqlLogOptions{select:s["select"].as_bool().unwrap(),mutation:s["mutation"].as_bool().unwrap()}),"user"=>c.user_identifier=text(&s["value"]),_=>panic!("fixture op")};states.push(snapshot(&c));}json!(states)}
fn main(){unsafe{std::env::set_var("TEAQL_LOG_ENDPOINT","off");std::env::remove_var("TEAQL_ALLOW_SENSITIVE_PLAINTEXT_LOGS");}for line in io::stdin().lock().lines(){println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap()));}}
