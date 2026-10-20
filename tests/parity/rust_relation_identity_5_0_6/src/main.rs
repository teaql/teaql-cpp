use cpp_relation_identities_core::{Q,E};
use serde_json::{json,Value as Json};
use std::{io::{self,BufRead},sync::{Arc,Mutex}};
use teaql_core::{Entity,TeaqlEntity};
#[path="../../rust_field_layout/src/bin/original_like_privacy_5_0_6.rs"] mod driver;
fn snapshot<T:Entity+TeaqlEntity>(entity:T)->Json {
 let descriptor=T::entity_descriptor();let mut loaded=serde_json::Map::new();
 for field in descriptor.properties {loaded.insert(field.name.clone(),json!(entity.is_field_loaded(&field.name)));}
 for relation in descriptor.relations {loaded.insert(relation.name.clone(),json!(entity.is_field_loaded(&relation.name)));}
 json!({"loaded":loaded,"json":entity.into_json()})
}
fn observe(input:&Json)->Json {
 let pool=Arc::new(Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));
 pool.lock().unwrap().execute_batch(include_str!("../../../support/relation_identity_sql.sql")).unwrap();
 let user_context=driver::block_on(cpp_relation_identities_core::service_runtime_from_pool(pool)).unwrap().with_active_root("Platform",1);
 macro_rules! run {($model:ty,$request:expr,$first:ident,$first_with:ident,$second:ident,$second_with:ident,$child:expr,$remove:ident)=>{{
 let mut request=$request;
 for operation in input["ops"].as_array().unwrap() {request=match operation.as_str().unwrap(){
 "self"=>request.select_self(),"all"=>request.select_all(),"children"=>request.select_children(),
 "first"=>request.$first(),"second"=>request.$second(),
 "first_with"=>request.$first_with($child.select_self().order_by_id_asc()),
 "second_with"=>request.$second_with($child.select_self().order_by_id_desc()),
 "remove_first"=>request.$remove(),
 "deleted"=>request.with_deleted_rows(),
 "json"=>request.filter_with_json(input["json"].as_str().unwrap()),
 _=>panic!("unknown relation fixture operation"),};}
 let descriptor=<$model>::entity_descriptor();
 let relations=descriptor.relations.iter().map(|r|json!({"name":r.name,"target":r.target_entity,"local":r.local_key,"foreign":r.foreign_key,"many":r.many,"attach":r.attach,"delete_missing":r.delete_missing})).collect::<Vec<_>>();
 let core:teaql_core::SelectQuery=request.clone().into();let mut output=json!({"query":format!("{core:?}"),"relations":relations,"rows":[],"error":null});
 let executable=request.comment("inspect canonical relationship identities").purpose("preserve query paths and graph reads");
 if input["mode"]=="rows" {match driver::block_on(executable.execute_for_rows(&user_context)){
 Ok(rows)=>output["rows"]=json!(rows.iter().map(|row|row.iter().map(|(k,v)|(k.clone(),format!("{v:?}"))).collect::<std::collections::BTreeMap<_,_>>()).collect::<Vec<_>>()),
 Err(error)=>output["error"]=json!(error.to_string()),}}
 else {match driver::block_on(executable.execute_for_list(&user_context)){
 Ok(rows)=>output["rows"]=json!(rows.into_iter().map(snapshot).collect::<Vec<_>>()),Err(error)=>output["error"]=json!(error.to_string()),}}
 output["logs"]=json!(user_context.sql_logs().iter().map(|entry|json!({"sql":entry.sql,"params":entry.params.iter().map(|v|format!("{v:?}")).collect::<Vec<_>>(),"comment":entry.comment,"purpose":entry.purpose,"trace":entry.trace_path.iter().map(|v|format!("{v:?}")).collect::<Vec<_>>()})).collect::<Vec<_>>());output
 }};}
 let minimal=input["minimal"]==true;
 match input["entity"].as_str().unwrap(){
 "entry"=>run!(cpp_relation_identities_core::Entry,if minimal{Q::entries_minimal()}else{Q::entries()},select_namespace,select_namespace_with,select_operator,select_operator_with,Q::owners(),unselect_namespace),
 "owner"=>run!(cpp_relation_identities_core::Owner,if minimal{Q::owners_minimal()}else{Q::owners()},select_class,select_class_with,select_template,select_template_with,Q::entries(),select_self),
 _=>run!(cpp_relation_identities_core::Profile,if minimal{Q::profiles_minimal()}else{Q::profiles()},select_owner,select_owner_with,select_owner,select_owner_with,Q::owners(),unselect_owner),
 }
}
fn main(){for line in io::stdin().lock().lines(){let input:Json=serde_json::from_str(&line.unwrap()).unwrap();println!("{}",observe(&input));}}
