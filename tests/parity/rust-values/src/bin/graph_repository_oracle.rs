//! Observe the original public repository extraction, without access bridges.
use std::{collections::{BTreeMap,BTreeSet},io::{self,BufRead},cell::RefCell,rc::Rc};
use serde_json::{json,Value as J};
use teaql_core::*;
use teaql_runtime::{UserContext,RuntimeModule,EntityDataService};
use teaql_data_service::{DataServiceExecutor,DataServiceCapabilities,QueryExecutor,QueryRequest,QueryResult,MutationExecutor,MutationRequest,MutationResult};
#[path="../fixture_value.rs"] mod fixture_value;
struct UnusedExecutor;
impl DataServiceExecutor for UnusedExecutor {type Error=std::io::Error;fn capabilities(&self)->DataServiceCapabilities{DataServiceCapabilities::default()}}
impl QueryExecutor for UnusedExecutor {async fn query(&self,_:QueryRequest)->Result<QueryResult,Self::Error>{panic!("extraction performed I/O")}}
impl MutationExecutor for UnusedExecutor {async fn mutate(&self,_:MutationRequest)->Result<MutationResult,Self::Error>{panic!("extraction performed I/O")}}
struct Model { values:MutationValues,dirty:Option<BTreeSet<String>>,old:Option<EntitySnapshot>,comment:Option<String>,deleted:bool,calls:Rc<RefCell<Vec<String>>> }
impl TeaqlEntity for Model {const ENTITY_NAME:&'static str="Parent";fn entity_descriptor()->EntityDescriptor{EntityDescriptor::new("Parent")}}
impl Entity for Model {
 fn from_compact_row(_:CompactRow)->Result<Self,EntityError>{panic!("not used")}
 fn dirty_fields(&self)->Option<BTreeSet<String>>{self.calls.borrow_mut().push("dirty".into());self.dirty.clone()}
 fn original_values(&self)->Option<EntitySnapshot>{self.calls.borrow_mut().push("original".into());self.old.clone()}
 fn is_marked_as_delete(&self)->bool{self.calls.borrow_mut().push("deleted".into());self.deleted}
 fn get_comment(&self)->Option<String>{self.calls.borrow_mut().push("comment".into());self.comment.clone()}
 fn is_new(&self)->bool{panic!("repository extraction called is_new")}
 fn is_field_loaded(&self,_:&str)->bool{panic!("repository extraction called is_field_loaded")}
 fn into_values(self)->MutationValues{self.calls.borrow_mut().push("values".into());self.values}
}
fn record(j:&J)->BTreeMap<String,Value>{j.as_object().unwrap().iter().map(|(k,v)|(k.clone(),fixture_value::value(v))).collect()}
fn formatted<T: std::fmt::Debug>(value: &T) -> J {
    json!({"compact":format!("{value:?}"),"pretty":format!("{value:#?}"),"lower_hex":format!("{value:x?}"),"upper_hex":format!("{value:X?}"),"pretty_lower_hex":format!("{value:#x?}"),"pretty_upper_hex":format!("{value:#X?}"),"width_precision":format!("{value:>40.2?}"),"fancy":format!("{value:🧊^+18.3?}"),"zero_pad":format!("{value:+012?}")})
}
fn evaluate(j:&J)->J {
 let mut module=RuntimeModule::new();
 for name in ["Parent","Child"] {let mut d=EntityDescriptor::new(name).property(PropertyDescriptor::new("id",DataType::U64).id());d.relations.push(RelationDescriptor::new("kids","Child"));module=module.descriptor(d);}
 let context=if j["missing"].as_bool().unwrap(){UserContext::new()}else{module.into_context()};
 let executor=UnusedExecutor;let service=EntityDataService::for_executor(&context,j["repository"].as_str().unwrap(),&executor);
 let calls=Rc::new(RefCell::new(vec![]));
 let model=Model{values:record(&j["values"]).into(),dirty:j["dirty"].as_array().map(|v|v.iter().map(|s|s.as_str().unwrap().to_owned()).collect()),old:if j["old"].is_null(){None}else{Some(record(&j["old"]).into())},comment:j["comment"].as_str().map(str::to_owned),deleted:j["deleted"].as_bool().unwrap(),calls:calls.clone()};
 let result=service.graph_node_from_entity(model);
 match result {Ok(graph)=>json!({"calls":*calls.borrow(),"graph":formatted(&graph),"retained":formatted(&graph.clone())}),Err(e)=>json!({"calls":*calls.borrow(),"error":e.to_string()})}
}
fn main(){for line in io::stdin().lock().lines(){let j:J=serde_json::from_str(&line.unwrap()).unwrap();println!("{}",evaluate(&j));}}
