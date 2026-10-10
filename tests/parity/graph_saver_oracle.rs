//! Original graph execution/save/read bodies; fixtures implement provider and sink SPIs only.
use std::{collections::BTreeMap,io::{self,BufRead},sync::{Arc,Mutex,atomic::{AtomicU64,Ordering}},future::Future,task::{Context,Poll,Wake,Waker}};
use serde_json::{json,Value as J};
use teaql_core::*;use teaql_data_service::*;
use teaql_runtime::{UserContext,RuntimeModule,RuntimeError,RawAuditEventSink,RawAuditEvent,InternalIdGenerator,GraphNode,GraphOperation,EntityRuntimeState,EntityKey,MutationPolicy,MutationPolicyIdentity,MutationDecision,MutationPlan,__cpp_registered_save};
mod fixture_value;
#[derive(Default)]struct State{notes:Mutex<Vec<String>>,requests:Mutex<Vec<J>>,events:Mutex<Vec<J>>,queries:Mutex<Vec<J>>,abandoned:AtomicU64}
impl State{fn note(&self,v:&str){self.notes.lock().unwrap().push(v.into())}}
fn record(j:&J)->BTreeMap<String,Value>{j.as_object().unwrap().iter().map(|(k,v)|(k.clone(),fixture_value::value(v))).collect()}
fn record_out(v:&BTreeMap<String,Value>)->J{json!(v.iter().map(|(k,v)|(k.clone(),format!("{v:?}"))).collect::<BTreeMap<_,_>>())}
fn trace_out(v:&[TraceNode])->J{json!(v.iter().map(|n|json!({"kind":n.kind as u8,"entity":n.entity_type,"comment":n.comment})).collect::<Vec<_>>())}
fn request_out(r:&MutationRequest)->J{match r{
 MutationRequest::Insert(c)=>json!({"kind":"insert","entity":c.entity,"values":record_out(&c.values),"trace":trace_out(&c.trace_chain)}),
 MutationRequest::Update(c)=>json!({"kind":"update","entity":c.entity,"id":format!("{:?}",c.id),"version":c.expected_version,"values":record_out(&c.values),"old":c.old_values.as_ref().map(|s|record_out(s)),"trace":trace_out(&c.trace_chain)}),
 MutationRequest::Delete(c)=>json!({"kind":"delete","entity":c.entity,"id":format!("{:?}",c.id),"version":c.expected_version,"trace":trace_out(&c.trace_chain)}),
 MutationRequest::Recover(c)=>json!({"kind":"recover","entity":c.entity,"id":format!("{:?}",c.id),"version":c.expected_version,"trace":trace_out(&c.trace_chain)}),_=>panic!("unexpected batch request")}}
fn node(j:&J)->GraphNode{
 let mut out=GraphNode::new(j["entity"].as_str().unwrap()).operation([GraphOperation::Upsert,GraphOperation::Create,GraphOperation::Reference,GraphOperation::Remove][j["operation"].as_u64().unwrap() as usize]);out.values=record(&j["values"]).into();if let Some(s)=j["comment"].as_str(){out.set_comment(s)}for(k,v)in j["relations"].as_object().unwrap(){out.relations.insert(k.clone(),v.as_array().unwrap().iter().map(node).collect());}out
}
fn node_out(n:&GraphNode)->J{json!({"entity":n.entity,"values":record_out(&n.values),"operation":n.operation as u8,"comment":n.comment,"relations":n.relations.iter().map(|(k,v)|(k.clone(),v.iter().map(node_out).collect::<Vec<_>>())).collect::<BTreeMap<_,_>>()})}
struct Sink{state:Arc<State>,fail:i64}
impl RawAuditEventSink for Sink{fn on_event(&self,_:&UserContext,e:&RawAuditEvent)->Result<(),RuntimeError>{let index=self.state.events.lock().unwrap().len();self.state.note("event");self.state.events.lock().unwrap().push(json!({"kind":format!("{:?}",e.kind),"entity":e.entity,"values":record_out(&e.values),"old":e.old_values.as_ref().map(record_out),"new":e.new_values.as_ref().map(record_out),"fields":e.updated_fields,"trace":trace_out(&e.trace_chain),"governance":e.mutation_governance.as_ref().map(|s|json!({"source":format!("{:?}",s.source),"request":s.request_key,"warnings":s.warning_codes}))}));if index as i64==self.fail{Err(RuntimeError::Event("event failed".into()))}else{Ok(())}}}
struct Id{next:AtomicU64}impl InternalIdGenerator for Id{fn generate_id(&self,_:&str)->Result<u64,RuntimeError>{Ok(self.next.fetch_add(1,Ordering::Relaxed))}}
struct NoWake;impl Wake for NoWake{fn wake(self:Arc<Self>){}}fn wait<F:Future>(f:F)->F::Output{let mut f=std::pin::pin!(f);let w=Waker::from(Arc::new(NoWake));let mut cx=Context::from_waker(&w);loop{if let Poll::Ready(v)=f.as_mut().poll(&mut cx){return v}}}

#[derive(Clone)]
struct Ambient { state: Arc<State>, input: J }
struct Tx { owner: Ambient, active: bool }
impl Drop for Tx {
    fn drop(&mut self) {
        if self.active { self.owner.state.abandoned.fetch_add(1,Ordering::Relaxed); }
    }
}
impl DataServiceExecutor for Ambient {type Error=io::Error;fn capabilities(&self)->DataServiceCapabilities{DataServiceCapabilities::default()}}
impl DataServiceExecutor for Tx {type Error=io::Error;fn capabilities(&self)->DataServiceCapabilities{DataServiceCapabilities::default()}}
async fn query(owner:&Ambient,q:QueryRequest,ambient:bool)->Result<QueryResult,io::Error>{
 let index=owner.state.queries.lock().unwrap().len();owner.state.note(if ambient{"ambient"}else{"tx"});owner.state.note("query");
 owner.state.queries.lock().unwrap().push(json!({"entity":q.query.entity,"filter":q.query.filter.as_ref().map(|e|format!("{e:?}")),"trace":trace_out(&q.query.trace_chain),"comment":q.query.comment}));
 if owner.input["read_fail_at"].as_i64().unwrap()==index as i64{return Err(io::Error::other("read failed"))}
 let rows=owner.input["read_batches"].as_array().unwrap().get(index).unwrap_or(if ambient{&owner.input["ambient_rows"]}else{&owner.input["rows"]});
 Ok(QueryResult{rows:rows.as_array().unwrap().iter().map(|j|CompactRow::from_map(record(j))).collect(),metadata:ExecutionMetadata::unrecorded_query(rows.as_array().unwrap().len())})
}
impl QueryExecutor for Ambient{async fn query(&self,q:QueryRequest)->Result<QueryResult,io::Error>{query(self,q,true).await}}
impl QueryExecutor for Tx{async fn query(&self,q:QueryRequest)->Result<QueryResult,io::Error>{query(&self.owner,q,false).await}}
impl MutationExecutor for Ambient{async fn mutate(&self,_:MutationRequest)->Result<MutationResult,io::Error>{panic!("ambient mutation is forbidden")}}
impl MutationExecutor for Tx{async fn mutate(&self,q:MutationRequest)->Result<MutationResult,io::Error>{
 let owner=&self.owner;let index=owner.state.requests.lock().unwrap().len();owner.state.note("provider");owner.state.requests.lock().unwrap().push(request_out(&q));
 if owner.input["fail"].as_i64().unwrap()==index as i64{return Err(io::Error::other("provider failed"))}
 let count=owner.input["counts"].as_array().unwrap().get(index).and_then(J::as_u64).unwrap_or(1);let mut metadata=ExecutionMetadata::unrecorded_query(0);metadata.operation=DataServiceOperation::Update;metadata.affected_rows=Some(count);
 Ok(MutationResult{affected_rows:count,generated_values:GeneratedValues::new(),persisted_snapshot:None,metadata})
}}
impl Transaction for Tx {
 type Error=io::Error;
 async fn commit(mut self)->Result<(),io::Error>{self.owner.state.note("commit");if self.owner.input["commit_fail"]==true{return Err(io::Error::other("commit failed"))}self.active=false;Ok(())}
 async fn rollback(mut self)->Result<(),io::Error>{self.owner.state.note("rollback");if self.owner.input["rollback_fail"]==true{return Err(io::Error::other("rollback failed"))}self.active=false;Ok(())}
}
impl TransactionExecutor for Ambient{
 type Tx<'tx>=Tx where Self:'tx;
 async fn begin(&self)->Result<Tx,io::Error>{self.state.note("begin");if self.input["begin_fail"]==true{return Err(io::Error::other("begin failed"))}Ok(Tx{owner:self.clone(),active:true})}
}
struct Policy{state:Arc<State>,deny:bool}
impl MutationPolicy for Policy{
 fn identity(&self)->MutationPolicyIdentity{MutationPolicyIdentity::new("fixture","1","fingerprint")}
 fn review(&self,_:&UserContext,_:&MutationPlan)->MutationDecision{self.state.note("review");if self.deny{MutationDecision::deny("DENIED","denied",Vec::<String>::new())}else{MutationDecision::allow()}}
}
fn key(j:&J)->EntityKey{EntityKey::new(j["entity"].as_str().unwrap(),fixture_value::value(&j["id"]))}
fn evaluate(j:J)->J{
 let state=Arc::new(State::default());let mut module=RuntimeModule::new().event_sink(Sink{state:state.clone(),fail:j["event_fail"].as_i64().unwrap()});
 for info in j["descriptors"].as_array().unwrap(){let mut d=EntityDescriptor::new(info["entity"].as_str().unwrap());let id=info["id"].as_str().unwrap();if !id.is_empty(){d=d.property(PropertyDescriptor::new(id,DataType::U64).id())}
  let version=info["version"].as_str().unwrap();if !version.is_empty(){d=d.property(PropertyDescriptor::new(version,DataType::I64).version())}let mut name=PropertyDescriptor::new("name",DataType::Text);if info["required"]==true{name=name.not_null()}d=d.property(name);
  for item in info["relations"].as_array().unwrap(){let mut rel=RelationDescriptor::new(item["name"].as_str().unwrap(),item["target"].as_str().unwrap());rel.local_key=item["local"].as_str().unwrap().into();rel.foreign_key=item["foreign"].as_str().unwrap().into();rel.many=item["many"].as_bool().unwrap();d=d.relation(rel)}module=module.descriptor(d);
 }
 let mut c=module.into_context();c.set_user_identifier("fixture");c.set_trace_id("fixture");c.disable_sql_log();c.set_internal_id_generator(Id{next:AtomicU64::new(100)});
 if j["policy"]!="none"{let p:Arc<dyn MutationPolicy>=Arc::new(Policy{state:state.clone(),deny:j["policy"]=="deny"});c.set_mutation_policy_registry(move|_:&str|Some(p.clone()));}
 c.register_executor(Ambient{state:state.clone(),input:j.clone()});
 let root=EntityRuntimeState::default();if let Some(comment)=j["ledger_comment"].as_str(){root.set_comment(comment)}
 for seed in j["seeds"].as_array().unwrap(){let k=key(seed);if seed["new"]==true{root.mark_as_new(k.clone())}if let Some(version)=seed["version"].as_i64(){root.set_original_version(k.clone(),version)}for(field,v)in record(&seed["values"]){root.set(k.clone(),field,v)}if seed["delete"]==true{root.mark_as_delete(k)}}
 let ledger=j["mode"]=="ledger";
 let result=wait(__cpp_registered_save::<Ambient>(&c,node(&j["root"]),if ledger{Some(root.clone())}else{None}));
 let outcome=match result{Ok(n)=>json!({"root":node_out(&n)}),Err(e)=>json!({"error":e.to_string()})};
 json!({"outcome":outcome,"notes":*state.notes.lock().unwrap(),"requests":*state.requests.lock().unwrap(),"queries":*state.queries.lock().unwrap(),"events":*state.events.lock().unwrap(),"abandoned":state.abandoned.load(Ordering::Relaxed),"pending_keys":root.current_change_set().changes().keys().map(|k|format!("{k:?}")).collect::<Vec<_>>(),"new_keys":root.new_keys().iter().map(|k|format!("{k:?}")).collect::<Vec<_>>(),"deleted_keys":root.deleted_keys().iter().map(|k|format!("{k:?}")).collect::<Vec<_>>()})
}
fn main(){unsafe{std::env::set_var("TEAQL_LOG_ENDPOINT","off");}for line in io::stdin().lock().lines(){println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap()));}}
