use cpp_dynamic_json_core::Q;
use serde_json::{Value as Json,json};
use teaql_core::Value;
use std::io::{self,BufRead};
#[path="../../rust_values/src/fixture_value.rs"] mod fixture_value;
#[path="../../rust_field_layout/src/bin/original_like_privacy_5_0_6.rs"] mod driver;
fn observe(input:&Json)->Json {
 let values:Vec<Value>=input["values"].as_array().unwrap().iter().map(fixture_value::value).collect();
 macro_rules! apply {
  ($request:expr,$positive:ident,$negative:ident) => {{
   let mut request=$request;
   for _ in 0..if input["repeat"]==true {2} else {1} {
    request=match input["carrier"].as_str().unwrap() {
     "i64"=>{let items:Vec<i64>=values.iter().map(|v|v.try_i64().unwrap()).collect();if input["negative"]==true {request.match_types(items)} else {request.match_types(items)}},
     "text"=>{let items:Vec<String>=values.iter().map(|v|v.try_text().unwrap().to_owned()).collect();if input["negative"]==true {request.match_types(items)} else {request.match_types(items)}},
     _=>if input["negative"]==true {request.match_types(values.clone())} else {request.match_types(values.clone())},
    };
   }
   let query:teaql_core::SelectQuery=request.clone().into();
   let mut output=json!({"query":format!("{query:?}")});
   if input["execute"]==true {
    let pool=std::sync::Arc::new(std::sync::Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));
    pool.lock().unwrap().execute_batch(include_str!("../../../support/dynamic_json_schema.sql")).unwrap();
    pool.lock().unwrap().execute_batch(include_str!("../../../support/dynamic_json_dataset.sql")).unwrap();
    let user_context=driver::block_on(cpp_dynamic_json_core::service_runtime_from_pool(pool)).unwrap().with_active_root("Platform",1);
    match driver::block_on(request.comment("inspect type matching inputs").purpose("qualify type matching inputs").execute_for_rows(&user_context)) {
     Ok(rows)=>{output["rows"]=json!(rows.iter().map(|row|["id","version"].map(|k|row.get(k).map(|v|format!("{v:?}")))).collect::<Vec<_>>());output["error"]=Json::Null;},
     Err(error)=>{output["rows"]=json!([]);output["error"]=json!(error.to_string());},
    }
   }
   output
  }};
 }
 let minimal=input["minimal"]==true;
 match input["field"].as_str().unwrap() {
  "name"=>apply!(if minimal {Q::people_minimal()} else {Q::people()},whose_name_in,whose_name_not_in),
  "person_id"=>apply!(if minimal {Q::people_minimal()} else {Q::people()},whose_id_in,whose_id_not_in),
  "code"=>apply!(if minimal {Q::orders_minimal()} else {Q::orders()},with_code_in,with_code_not_in),
  "order_id"=>apply!(if minimal {Q::orders_minimal()} else {Q::orders()},with_id_in,with_id_not_in),
  "priority"=>apply!(if minimal {Q::orders_minimal()} else {Q::orders()},with_priority_in,with_priority_not_in),
  _=>apply!(if minimal {Q::orders_minimal()} else {Q::orders()},with_amount_in,with_amount_not_in),
 }
}
fn main(){for line in io::stdin().lock().lines(){let input:Json=serde_json::from_str(&line.unwrap()).unwrap();println!("{}",observe(&input));}}
