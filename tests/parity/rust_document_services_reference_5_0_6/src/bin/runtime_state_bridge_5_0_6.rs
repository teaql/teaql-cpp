//! Actual original automatic save: borrowed Any root, absent/wrong core hook and typed independence.
use std::{any::Any,collections::BTreeMap,future::Future,io::{self,BufRead},sync::{Arc,atomic::{AtomicU64,Ordering}},task::{Context,Poll,Wake,Waker}};
use serde_json::{Value as Json,json};
use teaql_core::{CompactRow,DataType,Entity,EntityDescriptor,EntityError,MutationValues,PropertyDescriptor,TeaqlEntity,Value};
use teaql_runtime::{AuditedSaveExt,EntityRuntimeState,LedgerEntity,RuntimeError,RuntimeModule};
#[derive(Default)]struct Counts{core:AtomicU64,typed:AtomicU64}
struct Model<const MODE:u8,const TYPED:bool>{values:MutationValues,root:Option<EntityRuntimeState>,comment:Option<String>,pending:bool,counts:Arc<Counts>,wrong:i32}
impl<const M:u8,const L:bool> TeaqlEntity for Model<M,L>{
    const ENTITY_NAME:&'static str="BridgeEntity";
    fn entity_descriptor()->EntityDescriptor{EntityDescriptor::new(Self::ENTITY_NAME)
        .property(PropertyDescriptor::new("id",DataType::U64).id())
        .property(PropertyDescriptor::new("version",DataType::I64).version())
        .property(PropertyDescriptor::new("name",DataType::Text))}
}
impl<const M:u8,const L:bool> Entity for Model<M,L>{
    fn from_compact_row(row:CompactRow)->Result<Self,EntityError>{Ok(Self{values:row.into_map().into(),root:None,comment:None,pending:false,counts:Arc::new(Counts::default()),wrong:42})}
    fn into_values(self)->MutationValues{self.values}
    fn has_pending_dynamic_mutations(&self)->bool{self.pending}
    fn __teaql_runtime_state_any(&self)->Option<&dyn Any>{
        self.counts.core.fetch_add(1,Ordering::Relaxed);
        match M{0=>self.root.as_ref().map(|root|root as &dyn Any),2=>Some(&self.wrong),_=>None}
    }
    fn set_comment(&mut self,reason:String){if let Some(root)=&self.root{root.set_comment(reason.clone())}self.comment=Some(reason)}
    fn get_comment(&self)->Option<String>{self.comment.clone()}
}
impl<const M:u8> LedgerEntity for Model<M,true>{fn entity_runtime_state(&self)->Option<EntityRuntimeState>{self.counts.typed.fetch_add(1,Ordering::Relaxed);self.root.clone()}}
struct Noop;impl Wake for Noop{fn wake(self:Arc<Self>){}}
fn wait<T>(future:impl Future<Output=T>)->T{let waker=Waker::from(Arc::new(Noop));let mut cx=Context::from_waker(&waker);let mut future=Box::pin(future);loop{if let Poll::Ready(result)=future.as_mut().poll(&mut cx){return result}}}
fn observe<const M:u8,const L:bool>(input:&Json)->Json{
    let mut context=RuntimeModule::new().descriptor(Model::<M,L>::entity_descriptor()).into_context();context.disable_sql_log();
    let counts=Arc::new(Counts::default());
    let model=Model::<M,L>{values:BTreeMap::from([("id".into(),Value::U64(7)),("version".into(),Value::I64(1)),("name".into(),Value::Text("bridge".into()))]).into(),root:input["root"].as_bool().unwrap().then(EntityRuntimeState::default),comment:None,pending:input["pending"].as_bool().unwrap(),counts:counts.clone(),wrong:42};
    let error=match wait(model.audit_as("bridge reason").save(&context)){Ok(_)=>panic!("expected missing saver"),Err(error)=>error};
    let raw=error.to_string();let route=match &error{
        RuntimeError::Graph(message) if message=="dynamic save requires the runtime mutation ledger carrier"=>"carrier_error",
        RuntimeError::Graph(message) if message.starts_with("no DynGraphSaver registered")=>"missing_saver",_=>"other_error"};
    json!({"common":{"route":route,"core_calls":counts.core.load(Ordering::Relaxed),"typed_calls":counts.typed.load(Ordering::Relaxed)},"raw_error":raw})
}
fn mode<const M:u8>(input:&Json)->Json{if input["typed"].as_bool().unwrap(){observe::<M,true>(input)}else{observe::<M,false>(input)}}
fn main(){for line in io::stdin().lock().lines(){let input:Json=serde_json::from_str(&line.unwrap()).unwrap();println!("{}",match input["mode"].as_str().unwrap(){"borrowed"=>mode::<0>(&input),"empty"=>mode::<1>(&input),_=>mode::<2>(&input)});}}
