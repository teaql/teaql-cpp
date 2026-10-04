#![allow(dead_code)]
use std::io::{self,BufRead};
use serde_json::{json,Value as J};
use teaql_core::{Value,Record,Decimal,DataType};
pub use teaql_runtime::{UserContext,RuntimeError,SqlLogEntry,MutationGovernanceSnapshot};
use teaql_runtime::EntityKey;
#[path="../../../../../.reference/teaql-rs/teaql-runtime/src/event.rs"] mod event;
#[path="../../../../../.reference/teaql-rs/teaql-runtime/src/log_privacy.rs"] mod log_privacy;
pub use event::*;
mod context {pub use teaql_runtime::{SqlLogEntry,SqlLogOperation};}
#[path="../formatter_fixture.rs"] mod log_formatter;
use log_formatter::LogConfig;
fn without_time(text:String)->String{let end=text.find("]-").unwrap();text[end+1..].to_owned()}

fn value(j:&J)->Value{match j["kind"].as_str().unwrap(){
 "null"=>Value::Null,"typed_null"=>Value::TypedNull(DataType::Text),"bool"=>Value::Bool(j["value"].as_bool().unwrap()),"text"=>Value::Text(j["value"].as_str().unwrap().into()),"i64"=>Value::I64(j["value"].as_str().unwrap().parse().unwrap()),"u64"=>Value::U64(j["value"].as_str().unwrap().parse().unwrap()),"f64"=>Value::F64(j["value"].as_str().unwrap().parse().unwrap()),"f64_bits"=>Value::F64(f64::from_bits(u64::from_str_radix(j["value"].as_str().unwrap(),16).unwrap())),"decimal"=>Value::Decimal(j["value"].as_str().unwrap().parse::<Decimal>().unwrap()),"date"=>Value::Date(j["value"].as_str().unwrap().parse().unwrap()),"timestamp"=>Value::Timestamp(teaql_core::time::Timestamp(j["value"].as_str().unwrap().parse().unwrap())),"json"=>Value::Json(j["value"].clone()),"object"=>Value::Object(record(&j["value"])),"list"=>Value::List(j["value"].as_array().unwrap().iter().map(value).collect()),_=>panic!("unknown value")}}
fn record(j:&J)->Record{j.as_object().unwrap().iter().map(|(k,v)|(k.clone(),value(v))).collect()}

fn evaluate(j:J)->J{
 if j["op"]=="key"{let mut k=EntityKey::new(j["entity"].as_str().unwrap(),value(&j["id"]));if let Some(v)=j.get("mutated"){k.id=value(v);}return json!(format!("{k:?}"));}
 if j["op"]=="bound"{let (sql,audit)=log_formatter::bounds(j["text"].as_str().unwrap(),j["max"].as_str().unwrap().parse().unwrap());return json!({"sql":sql,"audit":audit});}
 for name in ["TEAQL_AUDIT_LOG","TEAQL_SQL_LOG","TEAQL_TOOL_LOG","TEAQL_AUDIT_LOG_ENTITIES","TEAQL_SQL_LOG_TABLES","TEAQL_TOOL_LOG_FOCUS"]{unsafe{if let Some(v)=j["env"][name].as_str(){std::env::set_var(name,v);}else{std::env::remove_var(name);}}}
 let c=LogConfig::load();json!({"levels":[c.audit_level as usize,c.sql_level as usize,c.tool_level as usize],"entities":c.audit_entities,"tables":c.sql_tables,"focus":c.tool_focus,"audit":c.should_log_audit(j["entity"].as_str().unwrap()),"sensitive":c.should_log_sensitive_audit(j["entity"].as_str().unwrap()),"sql":c.should_log_sql(j["sql"].as_str().unwrap()),"tool":c.should_log_tool(j["module"].as_str().unwrap())})
}
fn main(){for line in io::stdin().lock().lines(){let j=serde_json::from_str(&line.unwrap()).unwrap();println!("{}",evaluate(j));}}
