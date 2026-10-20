use cpp_inheritance_graph_core::Q;
use serde_json::{Value as Json,json};
use std::io::{self,BufRead};
fn observe(input:&Json)->Json {
 macro_rules! apply {($q:expr)=>{{let mut query=$q;
 if input["initial_slice"]==true {query=query.offset(3,7);}
 for value in input["filters"].as_array().unwrap(){query=query.filter_with_json(value.as_str().unwrap());}
 let query:teaql_core::SelectQuery=query.into();json!({"query":format!("{query:?}")})}};}
 let minimal=input["minimal"]==true;
 match input["entity"].as_str().unwrap(){
 "platform"=>apply!(if minimal{Q::platforms_minimal()}else{Q::platforms()}),
 "specialist"=>apply!(if minimal{Q::specialists_minimal()}else{Q::specialists()}),
 "school"=>apply!(if minimal{Q::schools_minimal()}else{Q::schools()}),
 "academy"=>apply!(if minimal{Q::academies_minimal()}else{Q::academies()}),
 "research_academy"=>apply!(if minimal{Q::research_academies_minimal()}else{Q::research_academies()}),
 _=>apply!(if minimal{Q::enrollments_minimal()}else{Q::enrollments()}),}
}
fn main(){std::panic::set_hook(Box::new(|_|{}));for line in io::stdin().lock().lines(){let input:Json=serde_json::from_str(&line.unwrap()).unwrap();let output=std::panic::catch_unwind(||observe(&input)).unwrap_or_else(|e|json!({"panic":e.downcast_ref::<String>().cloned().unwrap_or_else(||e.downcast_ref::<&str>().unwrap().to_string())}));println!("{output}");}}
