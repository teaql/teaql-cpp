#![allow(dead_code)]
use std::{io::{self,BufRead},sync::{Arc,Mutex},future::Future,collections::BTreeMap,task::{Context,Poll,Wake,Waker},panic::{catch_unwind,AssertUnwindSafe}};
use serde_json::{json,Value as J};
use teaql_core::{Value,TraceNode,TraceKind,EntityDescriptor};
pub use teaql_runtime::{RuntimeError,MutationGovernanceSnapshot,MutationPolicySource,MutationPolicyApprovalStatus,RuntimeOperation,RuntimeTelemetry,RuntimeTelemetryScope,RuntimeAttributeValue,SqlLogEntry};
#[path="../../../../../.reference/teaql-rs/teaql-runtime/src/event.rs"] mod event;
pub use event::*;
#[path="../../../../../.reference/teaql-rs/teaql-runtime/src/log_privacy.rs"] mod log_privacy;
mod context{pub use teaql_runtime::{SqlLogEntry,SqlLogOperation};}
#[path="../formatter_fixture.rs"] mod log_formatter;
// Compile the locked private methods unchanged. This shell supplies only their
// fields/services; it does not reimplement event dispatch or task-local logic.
pub struct UserContext{event_sink:Option<Box<dyn RawAuditEventSink>>,custom_event_sink:Option<Box<dyn SafeAuditEventSink>>,metadata:Option<Box<dyn teaql_runtime::MetadataStore>>,bootstrap:bool,telemetry:Arc<dyn RuntimeTelemetry>}
impl UserContext{fn is_generated_schema_bootstrap(&self)->bool{self.bootstrap}fn start_runtime_operation(&self,op:RuntimeOperation)->teaql_runtime::FailOpenRuntimeTelemetryScope{teaql_runtime::start_runtime_operation(&self.telemetry,op)}}
include!(concat!(env!("OUT_DIR"),"/context_event.rs"));
include!(concat!(env!("OUT_DIR"),"/governance_scope.rs"));
#[derive(Default)]struct State{notes:Mutex<Vec<String>>,raw:Mutex<Option<RawAuditEvent>>,safe:Mutex<Option<SafeAuditEvent>>}
impl State{fn note(&self,s:&str){self.notes.lock().unwrap().push(s.into());}}
struct Raw{state:Arc<State>,behavior:String}
impl RawAuditEventSink for Raw{fn on_event(&self,_:&UserContext,e:&RawAuditEvent)->Result<(),RuntimeError>{self.state.note("raw");*self.state.raw.lock().unwrap()=Some(e.clone());if self.behavior=="throw"{panic!("sink panic")}if self.behavior=="error"{return Err(RuntimeError::Event("raw failed".into()))}Ok(())}}
struct Safe{state:Arc<State>,behavior:String}
impl SafeAuditEventSink for Safe{fn on_safe_event(&self,_:&UserContext,e:&SafeAuditEvent)->Result<(),RuntimeError>{self.state.note("safe");*self.state.safe.lock().unwrap()=Some(e.clone());if self.behavior=="throw"{panic!("sink panic")}if self.behavior=="error"{return Err(RuntimeError::Event("safe failed".into()))}Ok(())}}
struct Scope{state:Arc<State>}
impl Drop for Scope{fn drop(&mut self){self.state.note("drop");}}
impl RuntimeTelemetryScope for Scope{fn success(&mut self,_:BTreeMap<String,RuntimeAttributeValue>){self.state.note("success")}fn failure(&mut self,t:&str){self.state.note(t)}}
struct Adapter{state:Arc<State>}
impl RuntimeTelemetry for Adapter{fn start(&self,op:RuntimeOperation)->Box<dyn RuntimeTelemetryScope>{assert_eq!(op.family,"audit");assert_eq!(op.name,"Order.event");self.state.note("start");Box::new(Scope{state:self.state.clone()})}}
fn snapshot(id:&str)->MutationGovernanceSnapshot{MutationGovernanceSnapshot{execution_id:id.into(),request_key:"request".into(),source:MutationPolicySource::Customer,policy:None,approval_status:MutationPolicyApprovalStatus::Approved,warning_codes:vec![],operations:vec![]}}
struct NoWake;impl Wake for NoWake{fn wake(self:Arc<Self>){}}
fn run<F:Future>(future:F)->F::Output{let mut f=Box::pin(future);let w=Waker::from(Arc::new(NoWake));let mut cx=Context::from_waker(&w);match f.as_mut().poll(&mut cx){Poll::Ready(v)=>v,Poll::Pending=>panic!("unexpected suspension")}}
fn evaluate(j:J)->J{
 let state=Arc::new(State::default());let mut c=UserContext{event_sink:None,custom_event_sink:None,metadata:None,bootstrap:j["bootstrap"].as_bool().unwrap(),telemetry:Arc::new(Adapter{state:state.clone()})};
 if j["raw"]!="none"{c.event_sink=Some(Box::new(Raw{state:state.clone(),behavior:j["raw"].as_str().unwrap().into()}));}if j["safe"]!="none"{c.custom_event_sink=Some(Box::new(Safe{state:state.clone(),behavior:j["safe"].as_str().unwrap().into()}));}
 if j["metadata"]==true{let mut desc=EntityDescriptor::new("Order");desc.audit_mask_fields=vec!["name".into()];desc.audit_value_max_len=Some(3);c.metadata=Some(Box::new(teaql_runtime::InMemoryMetadataStore::new().with_entity(desc)));}
 let mut e=RawAuditEvent::created("Order",BTreeMap::from([("name".into(),Value::Text("private name".into())),("version".into(),Value::I64(7))]));e.kind=[RawAuditEventKind::Created,RawAuditEventKind::Updated,RawAuditEventKind::Deleted,RawAuditEventKind::Recovered,RawAuditEventKind::SchemaCreated,RawAuditEventKind::SchemaVerified,RawAuditEventKind::FieldAdded,RawAuditEventKind::DataSeeded][j["kind"].as_u64().unwrap() as usize];
 match j["version"].as_str().unwrap(){"missing"=>{e.new_values=None;},"empty"=>{e.new_values=Some(BTreeMap::new());},"null"=>{e.new_values.as_mut().unwrap().insert("version".into(),Value::Null);},"text"=>{e.new_values.as_mut().unwrap().insert("version".into(),Value::Text("bad".into()));},_=>()}
 if let Some(comment)=j["comment"].as_str(){e.trace_chain.push(TraceNode::typed(TraceKind::Comment,"Order",None,comment));}
 e.mutation_governance=Some(snapshot("forged"));if j["identity"]==true{e.bootstrap_audit=Some(BootstrapAuditIdentity{actor:"caller".into(),category:"caller".into(),reason:"caller".into(),resulting_version:Some(99),occurred_at_millis:1});}
 let before=std::time::SystemTime::now();let send=||c.send_event(e.clone());let result=catch_unwind(AssertUnwindSafe(||if j["scope"]==true{run(with_mutation_governance(snapshot("accepted"),async{send()}))}else{send()}));let after=std::time::SystemTime::now();
 let outcome=match result{Ok(Ok(()))=>"ok".to_owned(),Ok(Err(RuntimeError::Event(s)))=>s,Ok(Err(_))=>panic!("error kind"),Err(_)=>"panic".into()};
 let raw=state.raw.lock().unwrap();let safe=state.safe.lock().unwrap();let normalized=raw.as_ref().map(|e|json!({"governance":e.mutation_governance.as_ref().map(|s|s.execution_id.clone()),"bootstrap":e.bootstrap_audit.as_ref().map(|b|{let time=if b.actor=="caller"{b.occurred_at_millis==1}else{b.occurred_at_millis>=before.duration_since(std::time::UNIX_EPOCH).unwrap().as_millis() as u64 && b.occurred_at_millis<=after.duration_since(std::time::UNIX_EPOCH).unwrap().as_millis() as u64};json!({"actor":b.actor,"category":b.category,"reason":b.reason,"version":b.resulting_version,"time_valid":time})})}));
 json!({"notes":state.notes.lock().unwrap().clone(),"outcome":outcome,"raw":normalized,"safe":safe.as_ref().map(|e|json!({"governance":e.mutation_governance.as_ref().map(|s|s.execution_id.clone()),"fields":e.fields.iter().map(|f|json!({"name":f.name,"value":f.value,"masked":f.masked,"truncated":f.truncated})).collect::<Vec<_>>()})),"outside":current_mutation_governance().is_none(),"unchanged":e.mutation_governance.unwrap().execution_id=="forged"})
}
fn main(){std::panic::set_hook(Box::new(|_|{}));unsafe{std::env::set_var("TEAQL_LOG_ENDPOINT","off");std::env::remove_var("TEAQL_ALLOW_SENSITIVE_PLAINTEXT_LOGS");}for line in io::stdin().lock().lines(){println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap()));}}
