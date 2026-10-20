use cpp_dynamic_json_core::Q;
use serde_json::{Value as Json,json};
use std::io::{self,BufRead};
#[path="../../rust_values/src/fixture_value.rs"] mod fixture_value;
#[path="../../rust_field_layout/src/bin/original_like_privacy_5_0_6.rs"] mod driver;
fn observe(input:&Json)->Json {
 let operand=fixture_value::value(&input["value"]);
 let upper=fixture_value::value(&input["upper"]);
 macro_rules! emit {($request:expr)=>{{
  let request=$request;
  let query:teaql_core::SelectQuery=request.clone().into();
  let mut out=json!({"query":format!("{query:?}")});
  if input["execute"]==true {
   let pool=std::sync::Arc::new(std::sync::Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));
   pool.lock().unwrap().execute_batch(include_str!("../../../support/dynamic_json_schema.sql")).unwrap();
   pool.lock().unwrap().execute_batch(include_str!("../../../support/dynamic_json_dataset.sql")).unwrap();
   let user_context=driver::block_on(cpp_dynamic_json_core::service_runtime_from_pool(pool)).unwrap().with_active_root("Platform",1);
   match driver::block_on(request.comment("inspect scalar comparison").purpose("qualify scalar comparison").execute_for_rows(&user_context)) {
    Ok(rows)=>{out["rows"]=json!(rows.iter().map(|row|["id","version"].map(|k|row.get(k).map(|v|format!("{v:?}")))).collect::<Vec<_>>());out["error"]=Json::Null;},
    Err(error)=>{out["rows"]=json!([]);out["error"]=json!(error.to_string());},
   }
  } out
 }};}
 macro_rules! scalar {($request:expr,$eq:ident,$ne:ident,$gt:ident,$gte:ident,$lt:ident,$lte:ident,$between:ident)=>{{
  let mut request=$request;
  for _ in 0..if input["repeat"]==true {2} else {1} {
   request=match input["operation"].as_str().unwrap() {
    "eq"=>request.$eq(operand.clone()),"ne"=>request.$ne(operand.clone()),
    "gt"=>request.$gt(operand.clone()),"gte"=>request.$gte(operand.clone()),
    "lt"=>request.$lt(operand.clone()),"lte"=>request.$lte(operand.clone()),
    _=>request.$between(operand.clone(),upper.clone()),
   };
  } emit!(request)
 }};}
 macro_rules! identity {($request:expr,$eq:ident,$ne:ident)=>{{
  let mut request=$request;
  for _ in 0..if input["repeat"]==true {2} else {1} {request=if input["operation"]=="eq" {request.$eq(operand.clone())} else {request.$ne(operand.clone())};}
  emit!(request)
 }};}
 let minimal=input["minimal"]==true;
 match input["field"].as_str().unwrap() {
  "name"=>scalar!(if minimal {Q::people_minimal()} else {Q::people()},whose_name_is,whose_name_is_not,whose_name_greater_than,whose_name_greater_than_or_equal_to,whose_name_less_than,whose_name_less_than_or_equal_to,whose_name_between),
  "person_id"=>identity!(if minimal {Q::people_minimal()} else {Q::people()},whose_id_is,whose_id_is_not),
  "code"=>scalar!(if minimal {Q::orders_minimal()} else {Q::orders()},with_code_is,with_code_is_not,with_code_greater_than,with_code_greater_than_or_equal_to,with_code_less_than,with_code_less_than_or_equal_to,with_code_between),
  "order_id"=>identity!(if minimal {Q::orders_minimal()} else {Q::orders()},with_id_is,with_id_is_not),
  "priority"=>scalar!(if minimal {Q::orders_minimal()} else {Q::orders()},with_priority_is,with_priority_is_not,with_priority_greater_than,with_priority_greater_than_or_equal_to,with_priority_less_than,with_priority_less_than_or_equal_to,with_priority_between),
  _=>scalar!(if minimal {Q::orders_minimal()} else {Q::orders()},with_amount_is,with_amount_is_not,with_amount_greater_than,with_amount_greater_than_or_equal_to,with_amount_less_than,with_amount_less_than_or_equal_to,with_amount_between),
 }
}
fn main(){for line in io::stdin().lock().lines(){let input:Json=serde_json::from_str(&line.unwrap()).unwrap();println!("{}",observe(&input));}}
