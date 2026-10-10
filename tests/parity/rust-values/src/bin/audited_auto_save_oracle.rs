//! Original public ordinary and automatic-ledger audited calls; fixtures implement models and SPIs only.
use std::{collections::BTreeMap,io::{self,BufRead},sync::{Arc,Mutex,atomic::{AtomicU64,Ordering}},future::Future,task::{Context,Poll,Wake,Waker}};
use serde_json::{json,Value as J};
use teaql_core::*;use teaql_data_service::*;
use teaql_runtime::{UserContext,RuntimeModule,RuntimeError,RawAuditEventSink,RawAuditEvent,InternalIdGenerator,EntityKey,EntityRuntimeState,ObjectLocation,LedgerEntity,Checker,CheckResults,InMemoryCheckerRegistry,FixEvidence,FixEvidenceSource,save_audited_ledger_entity,AuditedSaveExt};
#[path="../fixture_value.rs"] mod fixture_value;
#[derive(Default)]struct State{notes:Mutex<Vec<String>>,requests:Mutex<Vec<J>>,events:Mutex<Vec<J>>,queries:Mutex<Vec<J>>,abandoned:AtomicU64}
impl State{fn note(&self,v:&str){self.notes.lock().unwrap().push(v.into())}}
fn record(j:&J)->BTreeMap<String,Value>{j.as_object().unwrap().iter().map(|(k,v)|(k.clone(),fixture_value::value(v))).collect()}
fn record_out(v:&BTreeMap<String,Value>)->J{json!(v.iter().map(|(k,v)|(k.clone(),if matches!(v,Value::Timestamp(t) if t.0>1_000_000) { "Timestamp(<now>)".into() } else { format!("{v:?}") })).collect::<BTreeMap<_,_>>())}
fn trace_out(v:&[TraceNode])->J{json!(v.iter().map(|n|json!({"kind":n.kind as u8,"entity":n.entity_type,"comment":n.comment})).collect::<Vec<_>>())}
fn request_out(r:&MutationRequest)->J{match r{
 MutationRequest::Insert(c)=>json!({"kind":"insert","entity":c.entity,"values":record_out(&c.values),"trace":trace_out(&c.trace_chain)}),
 MutationRequest::Update(c)=>json!({"kind":"update","entity":c.entity,"id":format!("{:?}",c.id),"version":c.expected_version,"values":record_out(&c.values),"old":c.old_values.as_ref().map(|s|record_out(s)),"trace":trace_out(&c.trace_chain)}),
 MutationRequest::Delete(c)=>json!({"kind":"delete","entity":c.entity,"id":format!("{:?}",c.id),"version":c.expected_version,"trace":trace_out(&c.trace_chain)}),
 MutationRequest::Recover(c)=>json!({"kind":"recover","entity":c.entity,"id":format!("{:?}",c.id),"version":c.expected_version,"trace":trace_out(&c.trace_chain)}),_=>panic!("unexpected batch request")}}
struct Sink{state:Arc<State>,fail:i64}
impl RawAuditEventSink for Sink{fn on_event(&self,_:&UserContext,e:&RawAuditEvent)->Result<(),RuntimeError>{let index=self.state.events.lock().unwrap().len();self.state.note("event");self.state.events.lock().unwrap().push(json!({"kind":format!("{:?}",e.kind),"entity":e.entity,"values":record_out(&e.values),"old":e.old_values.as_ref().map(record_out),"new":e.new_values.as_ref().map(record_out),"fields":e.updated_fields,"trace":trace_out(&e.trace_chain),"governance":e.mutation_governance.as_ref().map(|s|json!({"source":format!("{:?}",s.source),"request":s.request_key,"warnings":s.warning_codes}))}));if index as i64==self.fail{Err(RuntimeError::Event("event failed".into()))}else{Ok(())}}}
struct Id{next:AtomicU64,state:Arc<State>,fail:i64}
impl InternalIdGenerator for Id{fn generate_id(&self,entity:&str)->Result<u64,RuntimeError>{
 self.state.note(&format!("id:{entity}"));let next=self.next.load(Ordering::Relaxed);
 if self.fail>=0 && next==100+self.fail as u64{return Err(RuntimeError::IdGeneration("id failed".into()))}
 Ok(self.next.fetch_add(1,Ordering::Relaxed))}}
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
#[derive(Clone)]
struct Model{
 values:MutationValues,runtime:Option<EntityRuntimeState>,comment:Option<String>,
 loaded:Option<std::collections::BTreeSet<String>>,dirty:Option<std::collections::BTreeSet<String>>,
 old:Option<EntitySnapshot>,fresh:bool,removed:bool,restored:bool,
}
impl TeaqlEntity for Model{const ENTITY_NAME:&'static str="Order";fn entity_descriptor()->EntityDescriptor{EntityDescriptor::new("Order").property(PropertyDescriptor::new("id",DataType::U64).id()).property(PropertyDescriptor::new("version",DataType::I64).version()).property(PropertyDescriptor::new("name",DataType::Text))}}
impl Entity for Model{
 fn from_compact_row(row:CompactRow)->Result<Self,EntityError>{
  if row.get("name").and_then(Value::try_text).as_deref()==Some("decode error"){return Err(EntityError::new("Order","decode failed"))}
  Ok(Self{values:row.into_map().into(),runtime:None,comment:None,loaded:None,dirty:None,old:None,fresh:false,removed:false,restored:false})
 }
 fn into_values(self)->MutationValues{self.values}
 fn set_comment(&mut self,comment:String){if let Some(root)=&self.runtime{root.set_comment(comment.clone())}self.comment=Some(comment)}
 fn get_comment(&self)->Option<String>{self.comment.clone()}
 fn is_field_loaded(&self,name:&str)->bool{self.loaded.as_ref().is_none_or(|fields|fields.contains(name))}
 fn dirty_fields(&self)->Option<std::collections::BTreeSet<String>>{self.dirty.clone()}
 fn original_values(&self)->Option<EntitySnapshot>{self.old.clone()}
 fn is_new(&self)->bool{self.fresh}
 fn is_marked_as_delete(&self)->bool{self.removed}
 fn on_loaded(&mut self,context:&dyn std::any::Any){if let Some(root)=context.downcast_ref::<EntityRuntimeState>(){self.runtime=Some(root.clone());self.restored=true}}
}
impl LedgerEntity for Model{fn entity_runtime_state(&self)->Option<EntityRuntimeState>{self.runtime.clone()}}
struct ProbeChecker{state:Arc<State>,action:String}
impl Checker for ProbeChecker{
 fn entity(&self)->&str{"Order"}
 fn check_and_fix(&self,context:&UserContext,values:&mut teaql_runtime::EntityValues,location:&ObjectLocation,results:&mut CheckResults){
  self.state.note("checker");
  if self.action=="fill"||self.action=="error"{context.record_fix_evidence(FixEvidence::new("Order",&location.clone().member("name").model_path(),FixEvidenceSource::Context,"fixture"))}
  if self.action=="fill"{values.insert("name".into(),Value::Text("fixed".into()));}
  if self.action=="error"{results.push(teaql_runtime::CheckResult::required(location.clone().member("name")));}
 }
}
fn key(j:&J)->EntityKey{EntityKey::new(j["entity"].as_str().unwrap(),fixture_value::value(&j["id"]))}
fn fields(j:&J)->Option<std::collections::BTreeSet<String>>{j.as_array().map(|v|v.iter().map(|v|v.as_str().unwrap().into()).collect())}
fn evaluate(j:J)->J{
 let state=Arc::new(State::default());let mut module=RuntimeModule::new().event_sink(Sink{state:state.clone(),fail:j["event_fail"].as_i64().unwrap()});
 for info in j["descriptors"].as_array().unwrap(){let mut d=EntityDescriptor::new(info["entity"].as_str().unwrap());let id=info["id"].as_str().unwrap();if !id.is_empty(){d=d.property(PropertyDescriptor::new(id,DataType::U64).id())}
  let version=info["version"].as_str().unwrap();if !version.is_empty(){d=d.property(PropertyDescriptor::new(version,DataType::I64).version())}
  let mut name=PropertyDescriptor::new("name",DataType::Text);if info["required"]==true{name=name.not_null()}d=d.property(name);
  for input in info["relations"].as_array().unwrap(){let mut rel=RelationDescriptor::new(input["name"].as_str().unwrap(),input["target"].as_str().unwrap());rel.local_key=input["local"].as_str().unwrap().into();rel.foreign_key=input["foreign"].as_str().unwrap().into();rel.many=input["many"].as_bool().unwrap();d=d.relation(rel)}module=module.descriptor(d);
 }
 let mut c=module.into_context();c.set_user_identifier("fixture");c.set_trace_id("fixture");c.disable_sql_log();c.set_internal_id_generator(Id{next:AtomicU64::new(100),state:state.clone(),fail:-1});
 if j["checker"]!="none"{c.set_checker_registry(InMemoryCheckerRegistry::new().with_checker(ProbeChecker{state:state.clone(),action:j["checker"].as_str().unwrap().into()}));}
 let root=EntityRuntimeState::default();
 for seed in j["seeds"].as_array().unwrap(){let k=key(seed);if seed["new"]==true{root.mark_as_new(k.clone())}if let Some(version)=seed["version"].as_i64(){root.set_original_version(k.clone(),version)}for(field,v)in record(&seed["values"]){root.set(k.clone(),field,v)}if seed["delete"]==true{root.mark_as_delete(k)}}
 let model=Model{values:record(&j["values"]).into(),runtime:if j["runtime"]==true{Some(root.clone())}else{None},comment:None,loaded:fields(&j["loaded"]),dirty:fields(&j["dirty"]),old:j["old"].as_object().map(|_|record(&j["old"]).into()),fresh:j["fresh"].as_bool().unwrap(),removed:j["removed"].as_bool().unwrap(),restored:false};
 if j["registered"]==true{c.register_executor(Ambient{state:state.clone(),input:j.clone()});}
 let audited=model.audit_as(j["reason"].as_str().unwrap());
 let result=if j["mode"]=="plain"{wait(audited.save(&c))}else{wait(save_audited_ledger_entity(audited,&c))};
 let outcome=match result{Ok(entity)=>json!({"values":record_out(&entity.values),"restored":entity.restored}),Err(error)=>json!({"error":if !j["registered"].as_bool().unwrap() && error.to_string().starts_with("graph write error: no DynGraphSaver registered"){"graph write error: no DynGraphSaver registered (<type>)".into()}else{error.to_string()}})};
 json!({"outcome":outcome,"abandoned":state.abandoned.load(Ordering::Relaxed),"notes":*state.notes.lock().unwrap(),"requests":*state.requests.lock().unwrap(),"queries":*state.queries.lock().unwrap(),"events":*state.events.lock().unwrap(),"changes":root.current_change_set().changes().iter().map(|(k,v)|json!({"key":format!("{k:?}"),"values":record_out(v)})).collect::<Vec<_>>(),"new":root.new_keys().iter().map(|k|format!("{k:?}")).collect::<Vec<_>>(),"deleted":root.deleted_keys().iter().map(|k|format!("{k:?}")).collect::<Vec<_>>(),"evidence":c.last_fix_evidence().iter().map(|e|json!({"entity":e.entity_type,"path":e.model_path,"source":format!("{:?}",e.source),"label":e.source_label})).collect::<Vec<_>>()})
}
fn main(){unsafe{std::env::set_var("TEAQL_LOG_ENDPOINT","off");}for line in io::stdin().lock().lines(){println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap()));}}
