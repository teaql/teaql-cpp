use std::{io::{self,BufRead},sync::Mutex,future::Future,task::{Context,Poll,Waker}};
use teaql_sql::*;
use teaql_core::{CompactRow,Value};
use serde_json::{Value as J,json};
#[path="../fixture_value.rs"] mod fixture_value;
use fixture_value::value;
#[derive(Debug)] struct Error(String);
impl std::fmt::Display for Error{fn fmt(&self,f:&mut std::fmt::Formatter<'_>)->std::fmt::Result{write!(f,"{}",self.0)}}
impl std::error::Error for Error{}
fn snapshot(q:&CompiledQuery)->J{json!({"sql":q.sql,"comment":q.comment,"params":q.params.iter().map(Value::to_json_value).collect::<Vec<_>>(),"generated":q.log_context.generated_sql,"policies":q.log_context.parameter_policies.iter().map(|p|match p{teaql_data_service::SqlParameterLogPolicy::Unknown=>0,teaql_data_service::SqlParameterLogPolicy::Plain=>1,teaql_data_service::SqlParameterLogPolicy::Masked=>2,teaql_data_service::SqlParameterLogPolicy::Credential=>3}).collect::<Vec<_>>()})}
struct Transport{index:usize,fail:usize,counts:Vec<usize>,calls:Mutex<Vec<J>>}
impl SqlTransport for Transport{
    type Error=Error;
    async fn fetch_all_compact_sql(&self,q:&CompiledQuery)->Result<Vec<CompactRow>,Error>{
        let call={let mut calls=self.calls.lock().unwrap();calls.push(snapshot(q));calls.len()};
        if call==self.fail{return Err(Error("fetch failed".into()));}
        Ok((0..self.counts.get(call-1).copied().unwrap_or(1)).map(|row|CompactRow::new(vec!["call".into(),"row".into(),"value".into()].into(),vec![Value::U64(call as u64),Value::U64(row as u64),q.params[self.index].clone()])).collect())
    }
    async fn execute_sql(&self,_:&CompiledQuery)->Result<u64,Error>{panic!("unused fixture path")}
}
fn ready<F:Future>(future:F)->F::Output{let mut future=std::pin::pin!(future);match future.as_mut().poll(&mut Context::from_waker(Waker::noop())){Poll::Ready(v)=>v,Poll::Pending=>panic!("fixture future must be ready")}}
fn evaluate(j:J)->J{
    if let Some(kind)=j["error_kind"].as_str(){let message=j["message"].as_str().unwrap().to_owned();let error:SqlExecutorError<Error>=match kind{"compile"=>SqlExecutorError::Compile(SqlCompileError::UnknownField(message)),"transport"=>SqlExecutorError::Transport(Error(message)),_=>SqlExecutorError::PersistedRecord(message)};return json!({"text":error.to_string()});}
    let mut query=CompiledQuery{sql:j["sql"].as_str().unwrap().into(),params:j["params"].as_array().unwrap().iter().map(value).collect(),comment:j["comment"].as_str().map(str::to_owned),log_context:Default::default()};
    query.log_context.generated_sql=j["generated"].as_bool().unwrap();query.log_context.parameter_policies=j["policies"].as_array().unwrap().iter().map(|p|[teaql_data_service::SqlParameterLogPolicy::Unknown,teaql_data_service::SqlParameterLogPolicy::Plain,teaql_data_service::SqlParameterLogPolicy::Masked,teaql_data_service::SqlParameterLogPolicy::Credential][p.as_u64().unwrap() as usize]).collect();
    let transport=Transport{index:j["index"].as_u64().unwrap() as usize,fail:j["fail"].as_u64().unwrap() as usize,counts:j["counts"].as_array().unwrap().iter().map(|c|c.as_u64().unwrap() as usize).collect(),calls:Mutex::new(Vec::new())};
    let values:Vec<_>=j["values"].as_array().unwrap().iter().map(value).collect();let future=transport.fetch_repeated_compact_sql(&query,transport.index,&values);let mut out=json!({"calls_before_poll":transport.calls.lock().unwrap().len()});
    match std::panic::catch_unwind(std::panic::AssertUnwindSafe(||ready(future))){
        Ok(Ok(rows))=>out["rows"]=json!(rows.iter().map(|r|r.iter().map(|(k,v)|(k.clone(),v.to_json_value())).collect::<serde_json::Map<_,_>>()).collect::<Vec<_>>()),Ok(Err(error))=>out["error"]=json!(error.to_string()),
        Err(p)=>{let text=p.downcast_ref::<String>().map(String::as_str).or_else(||p.downcast_ref::<&str>().copied()).unwrap_or("");assert!(text.contains("index out of bounds"));out["panic"]=json!("index");}
    }
    out["calls"]=json!(*transport.calls.lock().unwrap());out["original"]=snapshot(&query);out
}
fn main(){std::panic::set_hook(Box::new(|_|{}));for line in io::stdin().lock().lines(){println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap()));}}
