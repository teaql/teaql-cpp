use std::{io::{self,BufRead},sync::{Arc,Mutex},future::Future,task::{Context,Poll,Wake,Waker}};
use serde_json::{Value as J,json};
use teaql_core::*;
use teaql_runtime::{MetadataStore,DataServiceError};
use teaql_data_service::*;
struct RuntimeDataService<'a,M,E>{metadata:&'a M,executor:&'a E}
include!(concat!(env!("OUT_DIR"),"/runtime_base_query.rs"));
struct Spy{capture:bool,debug:bool,logs:Mutex<Vec<usize>>}
impl MetadataStore for Spy{fn entity(&self,_:&str)->Option<&EntityDescriptor>{None}fn all_entities(&self)->Vec<&EntityDescriptor>{vec![]}fn capture_execution_metadata(&self)->bool{self.capture}fn capture_query_debug(&self)->bool{self.debug}fn record_metadata_log(&self,m:&ExecutionMetadata){self.logs.lock().unwrap().push(m.result_count.unwrap_or(0));}}
struct Mock{failure:bool,bad_model:bool,event:bool,seen:Mutex<J>}
impl DataServiceExecutor for Mock{type Error=io::Error;fn capabilities(&self)->DataServiceCapabilities{DataServiceCapabilities::default()}}
impl QueryExecutor for Mock{
 async fn query(&self,q:QueryRequest)->Result<QueryResult,Self::Error>{self.query_observed(q,None).await}
 async fn query_observed<'a>(&'a self,q:QueryRequest,observer:Option<ExecutionObserver<'a>>)->Result<QueryResult,Self::Error>{
  *self.seen.lock().unwrap()=json!({"entity":q.query.entity,"trace":q.trace_chain.len(),"comment":q.comment,"capture":q.capture_execution_metadata,"debug":q.capture_debug_query,"observer":observer.is_some()});
  if self.event{if let Some(observer)=observer{observer(ExecutionMetadata::unrecorded_query(99));}}
  if self.failure{return Err(io::Error::other("fetch failed"))}Ok(QueryResult{rows:vec![CompactRow::from_map([("id".into(),Value::U64(1)),("fail".into(),Value::Bool(self.bad_model))].into())],metadata:ExecutionMetadata::unrecorded_query(1)})
 }
}
impl MutationExecutor for Mock{async fn mutate(&self,_:MutationRequest)->Result<MutationResult,Self::Error>{panic!("unused")}}
struct Item(CompactRow);
impl TeaqlEntity for Item{const ENTITY_NAME:&'static str="Item";fn entity_descriptor()->EntityDescriptor{EntityDescriptor::new("Item")}}
impl Entity for Item{fn from_compact_row(row:CompactRow)->Result<Self,EntityError>{if row.get("fail").and_then(Value::try_bool).unwrap_or(false){Err(EntityError::new("Item","handwritten model decode failed"))}else{Ok(Self(row))}}fn into_values(self)->MutationValues{self.0.into_map().into()}}
struct Noop;impl Wake for Noop{fn wake(self:Arc<Self>){}}
fn run<T>(future:impl Future<Output=T>)->T{let mut future=Box::pin(future);let waker=Waker::from(Arc::new(Noop));let mut context=Context::from_waker(&waker);match future.as_mut().poll(&mut context){Poll::Ready(v)=>v,Poll::Pending=>panic!("unexpected pending")}}
fn evaluate(j:&J)->J{
 let spy=Spy{capture:j["capture"].as_bool().unwrap(),debug:j["debug"].as_bool().unwrap(),logs:Mutex::new(vec![])};let mock=Mock{failure:j["failure"].as_bool().unwrap(),bad_model:j["bad_model"].as_bool().unwrap(),event:j["event"].as_bool().unwrap(),seen:Mutex::new(J::Null)};let base=RuntimeDataService{metadata:&spy,executor:&mock};let mut query=SelectQuery::new("Item");query.comment=Some("comment".into());if j["trace"]==true{query.trace_chain.push(TraceNode::typed(TraceKind::Request,"Item",None,"trace"));}
 let result:Result<(Vec<String>,bool),DataServiceError<io::Error>>=match j["mode"].as_str().unwrap(){
  "all"=>run(base.fetch_all(&query)).map(|rows|(rows.iter().map(|r|format!("{:?}",r.get("id").unwrap())).collect(),false)),
  "list"=>run(base.fetch_smart_list(&query)).map(|rows|(rows.as_slice().iter().map(|r|format!("{:?}",r.get("id").unwrap())).collect(),rows.is_loaded)),
  mode=>{let result=if mode=="entities"{run(base.fetch_entities::<Item>(&query))}else{run(base.fetch_enhanced_entities::<Item>(&query))};result.map(|rows|(rows.as_slice().iter().map(|r|format!("{:?}",r.0.get("id").unwrap())).collect(),rows.is_loaded))}
 };
 let mut out=match result{Ok((rows,loaded))=>json!({"rows":rows,"loaded":loaded}),Err(e)=>json!({"error":e.to_string()})};out["logs"]=json!(*spy.logs.lock().unwrap());out["request"]=mock.seen.lock().unwrap().clone();out
}
fn main(){for line in io::stdin().lock().lines(){println!("{}",evaluate(&serde_json::from_str::<J>(&line.unwrap()).unwrap()));}}
