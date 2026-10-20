use cpp_dynamic_json_core::Q;
use serde_json::{Value as Json,json};
use teaql_core::AggregateFunction;
use std::io::{self,BufRead};
#[path="../../rust_values/src/fixture_value.rs"] mod fixture_value;
#[path="../../rust_field_layout/src/bin/original_like_privacy_5_0_6.rs"] mod driver;
fn function(name:&str)->AggregateFunction {match name {
 "Count"=>AggregateFunction::Count,"Sum"=>AggregateFunction::Sum,"Avg"=>AggregateFunction::Avg,
 "Min"=>AggregateFunction::Min,"Max"=>AggregateFunction::Max,"Stddev"=>AggregateFunction::Stddev,
 "StddevPop"=>AggregateFunction::StddevPop,"VarSamp"=>AggregateFunction::VarSamp,"VarPop"=>AggregateFunction::VarPop,
 "BitAnd"=>AggregateFunction::BitAnd,"BitOr"=>AggregateFunction::BitOr,_=>AggregateFunction::BitXor}}
fn observe(input:&Json)->Json {
 let minimal=input["minimal"]==true;
 let alias=input["alias"].as_str().unwrap();
 macro_rules! emit {($request:expr)=>{{
  let request=$request;let query:teaql_core::SelectQuery=request.clone().into();
  let mut out=json!({"query":format!("{query:?}")});
  if input["execute"]==true {
   let pool=std::sync::Arc::new(std::sync::Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));
   pool.lock().unwrap().execute_batch(include_str!("../../../support/dynamic_json_schema.sql")).unwrap();
   pool.lock().unwrap().execute_batch(include_str!("../../../support/dynamic_json_dataset.sql")).unwrap();
   let user_context=driver::block_on(cpp_dynamic_json_core::service_runtime_from_pool(pool)).unwrap().with_active_root("Platform",1);
   match driver::block_on(request.comment("inspect projection and text").purpose("qualify projection and text").execute_for_rows(&user_context)) {
    Ok(rows)=>{out["rows"]=json!(rows.iter().map(|row|["id","version",input["field"].as_str().unwrap(),alias,"_group"].map(|k|row.get(k).map(|v|format!("{v:?}")))).collect::<Vec<_>>());out["error"]=Json::Null;},
    Err(error)=>{out["rows"]=json!([]);out["error"]=json!(error.to_string());},
   }
  }out
 }};}
 macro_rules! text {($request:expr,$contain:ident,$not_contain:ident,$start:ident,$not_start:ident,$end:ident,$not_end:ident,$sound:ident,$asc:ident,$desc:ident,$gbk:ident)=>{{
  let mut request=$request;let operand=fixture_value::value(&input["value"]);
  for _ in 0..if input["repeat"]==true {2} else {1} {
   request=match input["operation"].as_str().unwrap() {
    "contains"=>request.$contain(operand.try_text().unwrap()),"not_contains"=>request.$not_contain(operand.try_text().unwrap()),
    "starts"=>request.$start(operand.try_text().unwrap()),"not_starts"=>request.$not_start(operand.try_text().unwrap()),
    "ends"=>request.$end(operand.try_text().unwrap()),"not_ends"=>request.$not_end(operand.try_text().unwrap()),
    _=>request.$sound(operand.clone()),
   };
  }
  request=match input["order"].as_str().unwrap(){"asc"=>request.$asc(),"desc"=>request.$desc(),"gbk"=>request.$gbk(),_=>request};
  emit!(request)
 }};}
 macro_rules! numeric {($request:expr,$select:ident,$raw:ident,$remove:ident,$default:ident,$named:ident,$group:ident,$asc:ident,$desc:ident,$gbk:ident)=>{{
  let mut request=$request;let function=function(input["function"].as_str().unwrap());
  for op in input["operations"].as_array().unwrap() {
   request=match op.as_str().unwrap(){"select"=>request.$select(),"raw"=>request.$raw("1"),"remove"=>request.$remove(),
    "default"=>request.$default(function),"named"=>request.$named(alias,function),"group"=>request.$group("_group"),_=>request};
  }
  request=match input["order"].as_str().unwrap(){"asc"=>request.$asc(),"desc"=>request.$desc(),"gbk"=>request.$gbk(),_=>request};
  emit!(request)
 }};}
 if input["route"]=="text" {
  if input["field"]=="name" {text!(if minimal{Q::people_minimal()}else{Q::people()},whose_name_contains,whose_name_not_contains,whose_name_starts_with,whose_name_not_starts_with,whose_name_ends_with,whose_name_not_ends_with,whose_name_sounds_like,order_by_name_asc,order_by_name_desc,order_by_name_asc_using_gbk)}
  else{text!(if minimal{Q::orders_minimal()}else{Q::orders()},with_code_containing,with_code_not_containing,with_code_starting_with,with_code_not_starting_with,with_code_ending_with,with_code_not_ending_with,with_code_sounding_like,order_by_code_asc,order_by_code_desc,order_by_code_asc_using_gbk)}
 } else if input["field"]=="priority" {numeric!(if minimal{Q::orders_minimal()}else{Q::orders()},select_priority,select_priority_raw,unselect_priority,select_priority_with_function,select_priority_as_with_function,group_by_priority_as,order_by_priority_asc,order_by_priority_desc,order_by_priority_asc_using_gbk)}
 else{numeric!(if minimal{Q::orders_minimal()}else{Q::orders()},select_amount,select_amount_raw,unselect_amount,select_amount_with_function,select_amount_as_with_function,group_by_amount_as,order_by_amount_asc,order_by_amount_desc,order_by_amount_asc_using_gbk)}
}
fn main(){for line in io::stdin().lock().lines(){let input:Json=serde_json::from_str(&line.unwrap()).unwrap();println!("{}",observe(&input));}}
