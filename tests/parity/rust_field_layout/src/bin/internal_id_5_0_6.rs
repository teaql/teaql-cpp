use std::{io::{self,BufRead},sync::Arc,time::{SystemTime,UNIX_EPOCH},panic::{catch_unwind,AssertUnwindSafe}};
use serde_json::{json,Value};
use teaql_core::{TeaqlEntity,Entity,EntityDescriptor,EntityError,CompactRow,MutationValues};
use teaql_runtime::{InternalIdGenerator,AtomicCounterIdGenerator,SnowflakeIdGenerator,RuntimeError,UserContext,canonical_id_space_entity};
struct Shared(Arc<dyn InternalIdGenerator>);
impl InternalIdGenerator for Shared{fn generate_id(&self,e:&str)->Result<u64,RuntimeError>{self.0.generate_id(e)}fn ensure_floor(&self,e:&str,n:u64)->Result<(),RuntimeError>{self.0.ensure_floor(e,n)}}
struct Fixed(bool);
impl InternalIdGenerator for Fixed{fn generate_id(&self,_:&str)->Result<u64,RuntimeError>{if self.0{Err(RuntimeError::IdGeneration("unavailable".into()))}else{Ok(0)}}}
struct Model{fresh:bool,id:u64}
impl TeaqlEntity for Model{const ENTITY_NAME:&'static str="Order";fn entity_descriptor()->EntityDescriptor{EntityDescriptor::new("Order")}}
impl Entity for Model{fn from_compact_row(_:CompactRow)->Result<Self,EntityError>{Ok(Self{fresh:false,id:17})}fn into_values(self)->MutationValues{MutationValues::default()}fn mark_as_new(&mut self){self.fresh=true;}fn is_new(&self)->bool{self.fresh}}
fn err(e:RuntimeError)->Value{let RuntimeError::IdGeneration(ref message)=e else{panic!("unexpected error")};json!({"message":message,"display":e.to_string()})}
fn num(v:&Value)->u64{v.as_str().unwrap().parse().unwrap()}
fn panic_text(e:Box<dyn std::any::Any+Send>)->String{if let Some(s)=e.downcast_ref::<String>(){s.clone()}else if let Some(s)=e.downcast_ref::<&str>(){s.to_string()}else{"panic".into()}}
fn evaluate(j:&Value)->Value{
 if j["kind"]=="snowflake"{
  return match catch_unwind(AssertUnwindSafe(||{let worker=num(&j["worker"]);let dc=num(&j["datacenter"]);let generator=SnowflakeIdGenerator::new(worker,dc);generator.ensure_floor("ignored",u64::MAX).unwrap();let before=SystemTime::now().duration_since(UNIX_EPOCH).unwrap().as_millis() as u64;let mut previous=None;let mut valid=true;for _ in 0..j["count"].as_u64().unwrap(){let id=generator.generate_id("Order").unwrap();valid &= ((id>>12)&31)==worker && ((id>>17)&31)==dc && previous.is_none_or(|n|id>n) && (id>>22)+1288834974657>=before;previous=Some(id);}let after=SystemTime::now().duration_since(UNIX_EPOCH).unwrap().as_millis() as u64;valid &= previous.is_none_or(|n|(n>>22)+1288834974657<=after);json!({"valid":valid})})){Ok(v)=>v,Err(e)=>json!({"panic":panic_text(e)})};
 }
 let generator:Option<Arc<dyn InternalIdGenerator>>=match j["kind"].as_str().unwrap(){"atomic"=>Some(Arc::new(AtomicCounterIdGenerator::new(num(&j["start"])))),"fixed"=>Some(Arc::new(Fixed(false))),"error"=>Some(Arc::new(Fixed(true))),_=>None};
 let mut context=UserContext::new();if let Some(g)=&generator{context.set_internal_id_generator(Shared(g.clone()));}let mut model=Model{fresh:false,id:17};
 Value::Array(j["steps"].as_array().unwrap().iter().map(|s|{let entity=s["entity"].as_str().unwrap();match s["op"].as_str().unwrap(){
 "canonical"=>json!(canonical_id_space_entity(entity)),
 "generate"=>match context.generate_id(entity){Ok(n)=>json!({"ok":n.map(|v|v.to_string())}),Err(e)=>err(e)},
 "next"=>match context.next_id(entity){Ok(n)=>json!({"ok":n.to_string()}),Err(e)=>err(e)},
 "floor"=>match generator.as_ref().unwrap().ensure_floor(entity,num(&s["floor"])){Ok(())=>json!({"ok":null}),Err(e)=>err(e)},
 "bootstrap"=>{model.fresh=s["fresh"].as_bool().unwrap();let result=match context.initialize_generated_bootstrap_entity(&mut model,entity,num(&s["floor"])){Ok(())=>json!({"ok":null}),Err(e)=>err(e)};json!({"result":result,"fresh":model.fresh,"id":model.id.to_string()})},
 _=>panic!("unknown step")}}).collect())
}
fn main(){std::panic::set_hook(Box::new(|_|{}));for line in io::stdin().lock().lines(){let j:Value=serde_json::from_str(&line.unwrap()).unwrap();println!("{}",evaluate(&j));}}
