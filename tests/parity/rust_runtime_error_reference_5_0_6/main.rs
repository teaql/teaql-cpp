use serde_json::{json, Value};
use std::{cell::Cell,error::Error,fmt::{self,Debug},io::{self,BufRead},rc::Rc};
use teaql_runtime::{RuntimeError,DataServiceError};
use teaql_core::{RequestIntentError,RequestKind,dynamic_fields::DynamicFieldError};
fn formatted<T:Debug>(value:&T)->Value {
 json!({"compact":format!("{value:?}"),"pretty":format!("{value:#?}"),
 "lower_hex":format!("{value:x?}"),"upper_hex":format!("{value:X?}"),
 "pretty_lower_hex":format!("{value:#x?}"),"pretty_upper_hex":format!("{value:#X?}"),
 "width_precision":format!("{value:>40.2?}"),"fancy":format!("{value:🧊^+18.3?}"),"zero_pad":format!("{value:+012?}")})
}
struct OwnedError(String,Rc<Cell<usize>>);
impl Debug for OwnedError{fn fmt(&self,f:&mut fmt::Formatter<'_>)->fmt::Result{f.debug_tuple("OwnedError").field(&self.0).finish()}}
impl fmt::Display for OwnedError{fn fmt(&self,f:&mut fmt::Formatter<'_>)->fmt::Result{fmt::Display::fmt(&self.0,f)}}
impl Error for OwnedError{}
impl Drop for OwnedError{fn drop(&mut self){self.1.set(self.1.get()+1)}}
fn main(){for line in io::stdin().lock().lines(){
 let input:Value=serde_json::from_str(&line.unwrap()).unwrap();let text=input["text"].as_str().unwrap();
 let output=if input["kind"]=="executor"{
  let count=Rc::new(Cell::new(0));
  let error=DataServiceError::Executor(OwnedError(text.into(),count.clone()));
  let mut output=json!({"display":error.to_string(),"debug":formatted(&error),"source_none":error.source().is_none()});
  drop(error);output["destroyed_once"]=json!(count.get()==1);output
 }else{
  let mut error=match input["kind"].as_str().unwrap(){
   "request"=>RuntimeError::from(RequestIntentError{request_kind:if input["mutation"]==true{RequestKind::Mutation}else{RequestKind::Query},field:match input["field"].as_str().unwrap(){"purpose"=>"purpose","comment"=>"comment","other"=>"other",_=>""}}),
   "dynamic"=>RuntimeError::DynamicField(DynamicFieldError{code:if input["alternative"]==true{"CUSTOM_DYNAMIC_CODE"}else{"DYNAMIC_FIELD_TYPE_MISMATCH"},field:text.into()}),
   _=>RuntimeError::Event(text.into()),
  };
  for _ in 0..input["depth"].as_u64().unwrap(){error=RuntimeError::AuditAfterCommit{source:Box::new(error)}}
  let mut output=json!({"display":error.to_string(),"debug":formatted(&error),"source_none":error.source().is_none()});
  let wrapped=DataServiceError::<OwnedError>::Runtime(error);
  output["wrapped_display"]=json!(wrapped.to_string());output["wrapped_debug"]=formatted(&wrapped);output["wrapped_source_none"]=json!(wrapped.source().is_none());output
 };
 println!("{output}");
}}
