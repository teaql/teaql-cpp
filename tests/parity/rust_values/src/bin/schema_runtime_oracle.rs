use std::{future::Future,pin::Pin,sync::{Arc,Mutex},task::{Context,Poll,Wake,Waker},io::{self,BufRead}};
use serde_json::{json,Value as J};
use teaql_core::{Record,business_id::*};
use teaql_runtime::{UserContext,RuntimeModule,RuntimeError,SchemaProvider,SchemaInvocation,GeneratedSchemaBootstrapFuture,BusinessIdSchemaContributor,RawAuditEvent,RawAuditEventSink};
struct Shared{config:J,log:Mutex<Vec<String>>}
struct Sink(Arc<Shared>);
impl RawAuditEventSink for Sink{fn on_event(&self,_:&UserContext,e:&RawAuditEvent)->Result<(),RuntimeError>{self.0.log.lock().unwrap().push(format!("{}:{}",e.entity,if e.bootstrap_audit.is_some(){"bootstrap"}else{"normal"}));Ok(())}}
fn note(context:&UserContext,name:&str){context.send_event(RawAuditEvent::created(name,Record::new())).unwrap();}
struct Step<'a>{context:&'a UserContext,name:String,remaining:u64,fail:bool,done:bool}
impl Future for Step<'_>{type Output=Result<(),RuntimeError>;fn poll(mut self:Pin<&mut Self>,cx:&mut Context<'_>)->Poll<Self::Output>{note(self.context,&format!("{}.poll",self.name));if self.remaining>0{self.remaining-=1;cx.waker().wake_by_ref();return Poll::Pending;}self.done=true;Poll::Ready(if self.fail{Err(RuntimeError::Schema(format!("{} failed",self.name)))}else{Ok(())})}}
impl Drop for Step<'_>{fn drop(&mut self){note(self.context,&format!("{}.{}",self.name,if self.done{"finish"}else{"cancel"}));}}
struct Provider(Arc<Shared>);
impl SchemaProvider for Provider{fn ensure_schema<'a>(&'a self,context:&'a UserContext,_:&'a SchemaInvocation)->Pin<Box<dyn Future<Output=Result<(),RuntimeError>>+Send+'a>>{note(context,"provider.create");Box::pin(Step{context,name:"provider".into(),remaining:self.0.config["provider_delay"].as_u64().unwrap(),fail:self.0.config["fail"]=="provider",done:false})}}
struct Business(Arc<Shared>);
impl BusinessIdAllocator for Business{fn allocate(&self,_:&BusinessIdPlan)->Result<BusinessIdAllocation,BusinessIdError>{Err(BusinessIdError::new(BusinessIdErrorCode::Allocation,"unused"))}}
impl BusinessIdSchemaContributor for Business{fn ensure_schema(&self,context:&UserContext)->Result<(),BusinessIdError>{note(context,"business");if self.0.config["fail"]=="business"{Err(BusinessIdError::new(BusinessIdErrorCode::Allocation,"business failed"))}else{Ok(())}}}
fn bootstrap(context:&UserContext,index:usize)->GeneratedSchemaBootstrapFuture<'_>{let state=context.get_resource::<Arc<Shared>>().unwrap();let name=format!("callback{index}");note(context,&format!("{name}.create"));Box::pin(Step{context,name,remaining:state.config["delays"][index].as_u64().unwrap(),fail:state.config["fail"]==index.to_string(),done:false})}
fn first(c:&UserContext)->GeneratedSchemaBootstrapFuture<'_>{bootstrap(c,0)}fn second(c:&UserContext)->GeneratedSchemaBootstrapFuture<'_>{bootstrap(c,1)}fn third(c:&UserContext)->GeneratedSchemaBootstrapFuture<'_>{bootstrap(c,2)}
struct Noop;impl Wake for Noop{fn wake(self:Arc<Self>){}}
fn evaluate(j:J)->J{
 let state=Arc::new(Shared{config:j.clone(),log:Mutex::new(vec![])});let mut module=RuntimeModule::new().event_sink(Sink(state.clone()));for i in 0..j["delays"].as_array().unwrap().len(){module=module.generated_schema_bootstrap([first,second,third][i]);}let mut context=module.into_context();context.insert_resource(state.clone());if j["provider"].as_bool().unwrap(){context.set_schema_provider(Provider(state.clone()));}if j["business"].as_bool().unwrap(){context.set_business_id_infrastructure(Business(state.clone()));}
 note(&context,"before");
 let mut future=Box::pin(context.ensure_schema());
 let waker=Waker::from(Arc::new(Noop));
 let mut cx=Context::from_waker(&waker);
 let mut outcome=json!({"cancelled":true});
 for _ in 0..j["polls"].as_u64().unwrap(){
  match future.as_mut().poll(&mut cx){
   Poll::Pending=>note(&context,"outside"),
   Poll::Ready(result)=>{
    outcome=match result{
     Ok(())=>json!({"ok":true}),
     Err(e)=>{
      let RuntimeError::Schema(ref message)=e else{panic!("wrong error")};
      json!({"message":message,"display":e.to_string()})
     }
    };
    break;
   }
  }
 }
 drop(future);note(&context,"after");
 let calls=state.log.lock().unwrap().clone();
 json!({"result":outcome,"calls":calls})
}

fn main(){unsafe{std::env::set_var("TEAQL_TRACE_MODE","off");std::env::set_var("TEAQL_TRACE_OFF_ACK","__i_agree_to_disable_runtime_trace_only_for_extreme_performance_testing");}for line in io::stdin().lock().lines(){println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap()));}}
