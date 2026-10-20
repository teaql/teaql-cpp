use cpp_query_branches_core::Q;
use serde_json::{Value as Json,json};use std::io::{self,BufRead};
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
 let core:teaql_core::SelectQuery=query.into();json!({"query":format!("{core:?}")})
}
fn main(){for line in io::stdin().lock().lines(){let input:Json=serde_json::from_str(&line.unwrap()).unwrap();println!("{}",observe(&input));}}
