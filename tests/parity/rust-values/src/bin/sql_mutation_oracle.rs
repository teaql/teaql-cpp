use std::io::{self,BufRead};
use teaql_core::*;
use teaql_sql::*;
use serde_json::{Value as J,json};
#[path="../fixture_value.rs"] mod fixture_value;
#[path="../fixture_sql.rs"] mod fixture_sql;
use fixture_value::value;
use fixture_sql::{FixtureDialect,entity,expression};
fn items<'a>(j:&'a J,key:&str)->&'a [J]{j[key].as_array().map(Vec::as_slice).unwrap_or(&[])}
fn values(j:&J)->MutationValues{j.as_object().unwrap().iter().map(|(k,v)|(k.clone(),value(v))).collect::<std::collections::BTreeMap<_,_>>().into()}
fn version(j:&J)->Option<i64>{j.as_str().map(|s|s.parse().unwrap())}
fn compile(d:&FixtureDialect,e:&EntityDescriptor,j:&J)->Result<CompiledQuery,SqlCompileError>{
    let guard=j.get("guard").map(expression);
    match j["mode"].as_str().unwrap(){
        "insert"=>{let mut c=InsertCommand::new("IgnoredCommandEntity");c.values=values(&j["values"]);d.compile_insert(e,&c)},
        "batch_insert"=>{let mut c=BatchInsertCommand::new("IgnoredCommandEntity");c.batch_values=items(j,"rows").iter().map(values).collect();d.compile_batch_insert(e,&c)},
        "update"=>{let mut c=UpdateCommand::new("IgnoredCommandEntity",value(&j["id"]));c.values=values(&j["values"]);c.expected_version=version(&j["version"]);if let Some(g)=guard{d.compile_guarded_update(e,&c,&g)}else{d.compile_update(e,&c)}},
        "delete"=>{let mut c=DeleteCommand::new("IgnoredCommandEntity",value(&j["id"]));c.soft_delete=j["soft"].as_bool().unwrap();c.expected_version=version(&j["version"]);if let Some(g)=guard{d.compile_guarded_delete(e,&c,&g)}else{d.compile_delete(e,&c)}},
        "recover"=>{let c=RecoverCommand::new("IgnoredCommandEntity",value(&j["id"]),version(&j["version"]).unwrap());if let Some(g)=guard{d.compile_guarded_recover(e,&c,&g)}else{d.compile_recover(e,&c)}},
        "batch_update"=>{let mut c=BatchUpdateCommand::new("IgnoredCommandEntity",items(j,"fields").iter().map(|s|s.as_str().unwrap().into()).collect());c.batch_ids=items(j,"ids").iter().map(value).collect();c.batch_values=items(j,"rows").iter().map(values).collect();c.batch_expected_versions=items(j,"versions").iter().map(version).collect();d.compile_batch_update(e,&c)},
        _=>panic!("unknown mutation")
    }
}
fn kind(v:&Value)->u64{match v{Value::Null=>0,Value::Bool(_)=>1,Value::I64(_)=>2,Value::U64(_)=>3,Value::F64(_)=>4,Value::Text(_)=>5,Value::TypedNull(_)=>6,Value::List(_)=>7,Value::Object(_)=>8,Value::Decimal(_)=>9,Value::Json(_)=>10,Value::Date(_)=>11,Value::Timestamp(_)=>12}}
fn type_id(t:DataType)->u64{match t{DataType::Bool=>0,DataType::I64=>1,DataType::U64=>2,DataType::F64=>3,DataType::Decimal=>4,DataType::Text=>5,DataType::LargeText=>6,DataType::Json=>7,DataType::Date=>8,DataType::Timestamp=>9}}
fn evaluate(j:J)->J{
    let d=FixtureDialect{numbered:j["numbered"].as_bool().unwrap_or(false)};let e=entity(&j["entity"]);
    match std::panic::catch_unwind(||compile(&d,&e,&j)){
        Ok(Ok(q))=>json!({"sql":q.sql,"comment":q.comment,"generated":q.log_context.generated_sql,"values":q.params.iter().map(Value::to_json_value).collect::<Vec<_>>(),"kinds":q.params.iter().map(kind).collect::<Vec<_>>(),"null_types":q.params.iter().map(|v|if let Value::TypedNull(t)=v{Some(type_id(*t))}else{None}).collect::<Vec<_>>(),"policies":q.log_context.parameter_policies.iter().map(|p|match p{teaql_data_service::SqlParameterLogPolicy::Unknown=>0,teaql_data_service::SqlParameterLogPolicy::Plain=>1,teaql_data_service::SqlParameterLogPolicy::Masked=>2,teaql_data_service::SqlParameterLogPolicy::Credential=>3}).collect::<Vec<_>>()}),
        Ok(Err(e))=>json!({"error":e.to_string()}),
        Err(p)=>{let message=p.downcast_ref::<String>().map(String::as_str).or_else(||p.downcast_ref::<&str>().copied()).unwrap_or("");if message.contains("overflow"){json!({"panic":"overflow"})}else if message.contains("index out of bounds"){json!({"panic":"index"})}else{panic!("unexpected panic: {message}")}}
    }
}
fn main(){std::panic::set_hook(Box::new(|_|{}));for line in io::stdin().lock().lines(){println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap()));}}
