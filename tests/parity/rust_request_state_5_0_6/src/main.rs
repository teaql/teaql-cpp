use cpp_dynamic_json_core::Q;
use teaql_core::{AggregateFunction, SelectQuery, request::{QuerySelection, UnsafeRawSqlSegment}};
use serde_json::{Value as Json,json};
use std::io::{self,BufRead};
fn observe(input:&Json)->Json {
 let text=input["text"].as_str().unwrap();
 macro_rules! inspect {
  ($request:expr,$raw:ident,$relation:ident,$child:expr,$filter:ident) => {{
   let mut request=$request;
   for operation in input["operations"].as_array().unwrap() {
    request=match operation.as_str().unwrap() {
     "options"=>request.comment(text).raw_sql(text).raw_sql_filter(text)
       .create_property_as("_computed",text).$raw(UnsafeRawSqlSegment::trusted(text)),
     "child"=>request.enhance_child($child),
     "relation"=>request.$relation($child),
     "filter"=>request.$filter($child),
     "shape"=>request.group_by("id").aggregate_with_function("id","_count",AggregateFunction::Count).page_offset(3,7),
     _=>request,
    };
   }
   let options=format!("{:?}",request.query_options());
   let relations=format!("{:?}",request.relation_selections());
   let filters=format!("{:?}",request.relation_filters());
   let children=format!("{:?}",request.child_enhancements());
   let raw=request.clone().into_query();
   let complete:SelectQuery=request.clone().into();
   let selection:QuerySelection=request.into();
   json!({"options":options,"relations":relations,"filters":filters,"children":children,
    "raw":format!("{raw:?}"),"complete":format!("{complete:?}"),"selection":format!("{selection:?}")})
  }};
 }
 if input["entity"]=="person" {
  inspect!(if input["minimal"]==true {Q::people_minimal()} else {Q::people()},
   select_name_unsafe_raw,select_orders_with,
   Q::orders().comment(text).select_code().with_code_is(text),with_orders_matching)
 } else {
  inspect!(if input["minimal"]==true {Q::orders_minimal()} else {Q::orders()},
   select_code_unsafe_raw,select_customer_with,
   Q::people().comment(text).select_name().whose_name_is(text),with_customer_matching)
 }
}
fn main() {
 for line in io::stdin().lock().lines() {
  let input:Json=serde_json::from_str(&line.unwrap()).unwrap();
  println!("{}",observe(&input));
 }
}
