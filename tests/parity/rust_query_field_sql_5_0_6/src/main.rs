use cpp_query_branches_core::Q;
use serde_json::{Value as Json,json};use std::io::{self,BufRead};
#[path="../../rust_field_layout/src/bin/original_like_privacy_5_0_6.rs"] mod driver;
fn observe(input:&Json)->Json {
 let mut query=if input["minimal"]==true{Q::entries_minimal()}else{Q::entries()};let alias=input["alias"].as_str().unwrap();
 for _ in 0..if input["repeat"]==true{2}else{1}{query=match input["operation"].as_str().unwrap(){
 "select"=>query.select_ratio_with_function(teaql_core::AggregateFunction::Sum),
 "select_as"=>query.select_ratio_as_with_function(alias,teaql_core::AggregateFunction::Sum),
 "std"=>query.standard_deviation_ratio(),"std_as"=>query.standard_deviation_ratio_as(alias),
 "pop"=>query.square_root_of_population_standard_deviation_ratio(),"pop_as"=>query.square_root_of_population_standard_deviation_ratio_as(alias),
 "var"=>query.sample_variance_ratio(),"var_as"=>query.sample_variance_ratio_as(alias),
 "var_pop"=>query.sample_population_variance_ratio(),"var_pop_as"=>query.sample_population_variance_ratio_as(alias),
 "reference_value"=>query.filter_by_tenant(teaql_core::Value::U64(input["id"].as_u64().unwrap())),
 _=>query.filter_by_tenant(input["id"].as_u64().unwrap()),};}
 let core:teaql_core::SelectQuery=query.clone().into();let mut out=json!({"query":format!("{core:?}")});
 let pool=std::sync::Arc::new(std::sync::Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));
 pool.lock().unwrap().execute_batch(include_str!("../../../support/query_field_branches_sql.sql")).unwrap();
 let user_context=driver::block_on(cpp_query_branches_core::service_runtime_from_pool(pool)).unwrap().with_active_root("Tenant",1);
 match driver::block_on(query.comment("inspect numeric and reference fields").purpose("qualify numeric and reference SQL").execute_for_rows(&user_context)) {
  Ok(rows)=>{out["rows"]=json!(rows.iter().map(|row|["id","version","ratio","tenant",alias].map(|k|row.get(k).map(|v|format!("{v:?}")))).collect::<Vec<_>>());out["error"]=Json::Null;},
  Err(error)=>{out["rows"]=json!([]);out["error"]=json!(error.to_string());}
 }out
}
fn main(){for line in io::stdin().lock().lines(){let input:Json=serde_json::from_str(&line.unwrap()).unwrap();println!("{}",observe(&input));}}
