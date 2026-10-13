//! Original graph execution/save/read bodies; fixtures implement provider and sink SPIs only.
use std::{collections::BTreeMap,io::{self,BufRead},sync::{Arc,Mutex,atomic::{AtomicU64,Ordering}},future::Future,task::{Context,Poll,Wake,Waker}};
use serde_json::{json,Value as J};
use teaql_core::*;use teaql_data_service::*;
use teaql_runtime::{UserContext,RuntimeModule,RuntimeError,RawAuditEventSink,RawAuditEvent,InternalIdGenerator,EntityKey,EntityRuntimeState,ObjectLocation,__cpp_execute_ledger};
#[path = "../../../rust_values/src/fixture_value.rs"]
mod fixture_value;
#[derive(Default)]struct State{notes:Mutex<Vec<String>>,requests:Mutex<Vec<J>>,events:Mutex<Vec<J>>,queries:Mutex<Vec<J>>}
impl State{fn note(&self,v:&str){self.notes.lock().unwrap().push(v.into())}}
fn record(j:&J)->BTreeMap<String,Value>{j.as_object().unwrap().iter().map(|(k,v)|(k.clone(),fixture_value::value(v))).collect()}
fn record_out(v:&BTreeMap<String,Value>)->J{json!(v.iter().map(|(k,v)|(k.clone(),if matches!(v,Value::Timestamp(t) if t.0>1_000_000) { "Timestamp(<now>)".into() } else { format!("{v:?}") })).collect::<BTreeMap<_,_>>())}
fn trace_out(v:&[TraceNode])->J{json!(v.iter().map(|n|json!({"kind":n.kind as u8,"entity":n.entity_type,"comment":n.comment})).collect::<Vec<_>>())}
fn request_out(r:&MutationRequest)->J{match &r.command{
 MutationCommand::Insert(c)=>json!({"kind":"insert","entity":c.entity,"values":record_out(&c.values),"trace":trace_out(&c.trace_chain)}),
 MutationCommand::Update(c)=>json!({"kind":"update","entity":c.entity,"id":format!("{:?}",c.id),"version":c.expected_version,"values":record_out(&c.values),"old":c.old_values.as_ref().map(|s|record_out(s)),"trace":trace_out(&c.trace_chain)}),
 MutationCommand::Delete(c)=>json!({"kind":"delete","entity":c.entity,"id":format!("{:?}",c.id),"version":c.expected_version,"trace":trace_out(&c.trace_chain)}),
 MutationCommand::Recover(c)=>json!({"kind":"recover","entity":c.entity,"id":format!("{:?}",c.id),"version":c.expected_version,"trace":trace_out(&c.trace_chain)}),_=>panic!("unexpected batch request")}}
struct Mock{state:Arc<State>,input:J}
impl DataServiceExecutor for Mock{type Error=io::Error;fn capabilities(&self)->DataServiceCapabilities{DataServiceCapabilities::default()}}
impl QueryExecutor for Mock{async fn query(&self,q:QueryRequest)->Result<QueryResult,Self::Error>{let index=self.state.queries.lock().unwrap().len();self.state.note("query");self.state.queries.lock().unwrap().push(json!({"entity":q.query.entity,"filter":q.query.filter.as_ref().map(|e|format!("{e:?}")),"trace":trace_out(&q.query.trace_chain),"comment":q.query.comment}));if self.input["read_fail_at"].as_i64().unwrap()==index as i64{return Err(io::Error::other("read failed"))}let rows=self.input["read_batches"].as_array().unwrap().get(index).unwrap_or(&self.input["rows"]);Ok(QueryResult{rows:rows.as_array().unwrap().iter().map(|j|CompactRow::from_map(record(j))).collect(),metadata:ExecutionMetadata::unrecorded_query(rows.as_array().unwrap().len())})}}
impl MutationExecutor for Mock{async fn mutate(&self,q:MutationRequest)->Result<MutationResult,Self::Error>{let index=self.state.requests.lock().unwrap().len();self.state.note("provider");self.state.requests.lock().unwrap().push(request_out(&q));if self.input["fail"].as_i64().unwrap()==index as i64{return Err(io::Error::other("provider failed"))}let count=self.input["counts"].as_array().unwrap().get(index).and_then(J::as_u64).unwrap_or(1);let mut metadata=ExecutionMetadata::unrecorded_query(0);metadata.operation=DataServiceOperation::Update;metadata.affected_rows=Some(count);Ok(MutationResult{affected_rows:count,generated_values:GeneratedValues::new(),persisted_snapshot:None,metadata})}}
struct Sink{state:Arc<State>,fail:i64}
impl RawAuditEventSink for Sink{fn on_event(&self,_:&UserContext,e:&RawAuditEvent)->Result<(),RuntimeError>{let index=self.state.events.lock().unwrap().len();self.state.note("event");self.state.events.lock().unwrap().push(json!({"kind":format!("{:?}",e.kind),"entity":e.entity,"values":record_out(&e.values),"old":e.old_values.as_ref().map(record_out),"new":e.new_values.as_ref().map(record_out),"fields":e.updated_fields,"trace":trace_out(&e.trace_chain),"governance":e.mutation_governance.as_ref().map(|s|json!({"source":format!("{:?}",s.source),"request":s.request_key,"warnings":s.warning_codes}))}));if index as i64==self.fail{Err(RuntimeError::Event("event failed".into()))}else{Ok(())}}}
struct Id{next:AtomicU64,state:Arc<State>,fail:i64}
impl InternalIdGenerator for Id{fn generate_id(&self,entity:&str)->Result<u64,RuntimeError>{
 self.state.note(&format!("id:{entity}"));let next=self.next.load(Ordering::Relaxed);
 if self.fail>=0 && next==100+self.fail as u64{return Err(RuntimeError::IdGeneration("id failed".into()))}
 Ok(self.next.fetch_add(1,Ordering::Relaxed))}}
struct NoWake;impl Wake for NoWake{fn wake(self:Arc<Self>){}}fn wait<F:Future>(f:F)->F::Output{let mut f=std::pin::pin!(f);let w=Waker::from(Arc::new(NoWake));let mut cx=Context::from_waker(&w);loop{if let Poll::Ready(v)=f.as_mut().poll(&mut cx){return v}}}
fn key(j:&J)->EntityKey{EntityKey::new(j["entity"].as_str().unwrap(),fixture_value::value(&j["id"]))}
fn evaluate(j:J)->J{
 let state=Arc::new(State::default());
 let mut module=RuntimeModule::new().event_sink(Sink{state:state.clone(),fail:j["event_fail"].as_i64().unwrap()});
 for info in j["descriptors"].as_array().unwrap(){
  let mut d=EntityDescriptor::new(info["entity"].as_str().unwrap());
  let id=info["id"].as_str().unwrap();if !id.is_empty(){d=d.property(PropertyDescriptor::new(id,DataType::U64).id())}
  let version=info["version"].as_str().unwrap();if !version.is_empty(){d=d.property(PropertyDescriptor::new(version,DataType::I64).version())}
  let mut name=PropertyDescriptor::new("name",DataType::Text);if info["required"]==true{name=name.not_null()}d=d.property(name);
  if info["timestamps"]==true{d=d.property(PropertyDescriptor::new("create_time",DataType::Timestamp)).property(PropertyDescriptor::new("update_time",DataType::Timestamp))}
  for item in info["relations"].as_array().unwrap(){let mut rel=RelationDescriptor::new(item["name"].as_str().unwrap(),item["target"].as_str().unwrap());rel.local_key=item["local"].as_str().unwrap().into();rel.foreign_key=item["foreign"].as_str().unwrap().into();rel.many=item["many"].as_bool().unwrap();d=d.relation(rel)}
  module=module.descriptor(d);
 }
 let mut c=module.into_context();c.set_user_identifier("fixture");c.set_trace_id("fixture");c.disable_sql_log();
 c.set_internal_id_generator(Id{next:AtomicU64::new(100),state:state.clone(),fail:j["id_fail"].as_i64().unwrap()});
 let mut root=EntityRuntimeState::default();
 if j["loaded"].is_object(){root.set_original_compact_row(j["service"].as_str().unwrap(),CompactRow::from_map(record(&j["loaded"])))}
 if let Some(s)=j["comment"].as_str(){root.set_comment(s)}
 let mut locations=BTreeMap::new();
 for seed in j["seeds"].as_array().unwrap(){let k=key(seed);if seed["new"]==true{root.mark_as_new(k.clone())}if let Some(v)=seed["version"].as_i64(){root.set_original_version(k.clone(),v)}for(field,v)in record(&seed["values"]){root.set(k.clone(),field,v)}if seed["delete"]==true{root.mark_as_delete(k.clone())}
  let mut location=ObjectLocation::root();for part in seed["path"].as_array().unwrap(){location=if let Some(name)=part.as_str(){location.member(name)}else{location.element(part.as_u64().unwrap()as usize)}}locations.insert(k,location);
 }
 if j["composition_conflict"]==true{let k=EntityKey::new("Order",Value::U64(7));root.set_original_version(k.clone(),1);let other=EntityRuntimeState::default();other.set_original_version(k,2);root.adopt_mutations_from(&other);}
 let provider=Mock{state:state.clone(),input:j.clone()};
 let outcome=match wait(__cpp_execute_ledger(&c,&provider,j["service"].as_str().unwrap(),root,&locations)){
  Ok(ids)=>json!({"generated":ids.iter().map(|(k,v)|json!({"key":format!("{k:?}"),"value":format!("{v:?}")})).collect::<Vec<_>>()}),
  Err(e)=>json!({"error":e.to_string()})};
 json!({"outcome":outcome,"notes":*state.notes.lock().unwrap(),"requests":*state.requests.lock().unwrap(),"queries":*state.queries.lock().unwrap(),"events":*state.events.lock().unwrap()})
}
fn main(){unsafe{std::env::set_var("TEAQL_LOG_ENDPOINT","off");}for line in io::stdin().lock().lines(){println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap()));}}
