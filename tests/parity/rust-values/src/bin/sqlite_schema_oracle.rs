//! Observe only the public, pinned production provider. No DDL algorithm replica.
use std::{io::{self,BufRead},future::Future,task::{Context,Poll,Waker},sync::{Arc,Mutex}};
use serde_json::{Value as J,json};
use teaql_core::{EntityDescriptor,RelationDescriptor};
use teaql_runtime::{RuntimeModule,UserContext,RawAuditEvent,RawAuditEventSink,RuntimeError,GraphNode};
use teaql_provider_sqlite::{SqliteMutationExecutor,SqliteProviderExt};
#[path="../fixture_value.rs"]mod fixture_value;
#[path="../fixture_sql.rs"]mod fixture_sql;
fn ready<F:Future>(future:F)->F::Output{let mut f=std::pin::pin!(future);match f.as_mut().poll(&mut Context::from_waker(Waker::noop())){Poll::Ready(v)=>v,_=>panic!("unexpected pending")}}
fn items<'a>(j:&'a J,key:&str)->&'a [J]{j[key].as_array().map(Vec::as_slice).unwrap_or(&[])}
fn entity(j:&J)->EntityDescriptor{let mut e=fixture_sql::entity(j);e.data_service=j["service"].as_str().map(str::to_owned);for v in items(j,"relations"){let mut r=RelationDescriptor::new(v["name"].as_str().unwrap(),v["target"].as_str().unwrap());if let Some(s)=v["local"].as_str(){r.local_key=s.into();}if let Some(s)=v["foreign"].as_str(){r.foreign_key=s.into();}r.many=v["many"].as_bool().unwrap_or(false);e.relations.push(r);}e}
struct Sink{db:usize,events:Arc<Mutex<Vec<J>>>,fail:bool}
impl RawAuditEventSink for Sink{fn on_event(&self,_:&UserContext,e:&RawAuditEvent)->Result<(),RuntimeError>{
 // Called synchronously by ensure_schema while its connection mutex is held.
 // The borrowed native handle stays alive for this whole fixture; do not relock.
 let autocommit=unsafe{libsqlite3_sys::sqlite3_get_autocommit(self.db as *mut _)!=0};
 let values=e.values.iter().map(|(k,v)|(k.clone(),v.to_json_value())).collect::<serde_json::Map<_,_>>();
 self.events.lock().unwrap().push(json!({"kind":format!("{:?}",e.kind),"entity":e.entity,"values":values,"autocommit":autocommit,"bootstrap":e.bootstrap_audit.is_some()}));
 if self.fail{Err(RuntimeError::Event("fixture sink error".into()))}else{Ok(())}
}}
fn sql_rows(db:&rusqlite::Connection,sql:&str)->J{let mut stmt=db.prepare(sql).unwrap();let count=stmt.column_count();let mut rows=stmt.query([]).unwrap();let mut all=vec![];while let Some(row)=rows.next().unwrap(){let mut values=vec![];for i in 0..count{use rusqlite::types::ValueRef::*;values.push(match row.get_ref(i).unwrap(){Null=>J::Null,Integer(v)=>json!(v),Real(v)=>json!(v),Text(v)=>json!(std::str::from_utf8(v).unwrap()),Blob(v)=>json!(v)});}all.push(values);}json!(all)}
fn evaluate(j:J)->J{
 let db=rusqlite::Connection::open_in_memory().unwrap();for sql in items(&j,"setup"){db.execute_batch(sql.as_str().unwrap()).unwrap();}let ptr=unsafe{db.handle()} as usize;
 let provider=SqliteMutationExecutor::from_connection(db);let mut rounds=vec![];
 for round in items(&j,"rounds"){
  for sql in items(round,"sql"){provider.connection().lock().unwrap().execute_batch(sql.as_str().unwrap()).unwrap();}
  let events=Arc::new(Mutex::new(vec![]));let mut module=RuntimeModule::new().event_sink(Sink{db:ptr,events:events.clone(),fail:round["sink_error"].as_bool().unwrap_or(false)});
  for v in items(round,"entities"){module=module.descriptor(entity(v));}
  if round["legacy"]=="initial"{module=module.initial_graph(GraphNode::new("Legacy"));}if round["legacy"]=="root"{module=module.root_graph(GraphNode::new("Legacy"));}
  let mut context=module.into_context();context.use_sqlite_provider(provider.clone());
  let result=match ready(context.ensure_schema()){Ok(())=>json!({"ok":true}),Err(e)=>{let RuntimeError::Schema(ref message)=e else{panic!("wrong error")};json!({"message":message,"display":e.to_string()})}};
  let db=provider.connection();let db=db.lock().unwrap();let queries=items(round,"queries").iter().map(|v|sql_rows(&db,v.as_str().unwrap())).collect::<Vec<_>>();
  rounds.push(json!({"result":result,"events":events.lock().unwrap().clone(),"autocommit":db.is_autocommit(),"master":sql_rows(&db,"SELECT type,name,tbl_name,sql FROM sqlite_master ORDER BY type,name"),"queries":queries}));
 }
 let mut tables=serde_json::Map::new();for name in items(&j,"inspect"){let name=name.as_str().unwrap();tables.insert(name.into(),json!({"exists":provider.table_exists(name).unwrap(),"columns":match provider.table_columns(name){Ok(v)=>json!(v),Err(e)=>json!({"error":e.to_string()})}}));}
 json!({"rounds":rounds,"tables":tables})
}
fn main(){unsafe{std::env::set_var("TEAQL_LOG_ENDPOINT","off");std::env::set_var("TEAQL_TRACE_MODE","off");std::env::set_var("TEAQL_TRACE_OFF_ACK","__i_agree_to_disable_runtime_trace_only_for_extreme_performance_testing");}for line in io::stdin().lock().lines(){println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap()));}}
