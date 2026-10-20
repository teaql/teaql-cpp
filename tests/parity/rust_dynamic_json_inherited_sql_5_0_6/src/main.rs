use cpp_inheritance_graph_core::Q;
use serde_json::{Value as Json,json};use std::io::{self,BufRead};
#[path="../../rust_field_layout/src/bin/original_like_privacy_5_0_6.rs"] mod driver;
fn observe(input:&Json)->Json {
 let minimal=input["minimal"]==true;
 macro_rules! emit {($q:expr)=>{{let mut query=$q;
 for filter in input["filters"].as_array().unwrap(){query=query.filter_with_json(filter.as_str().unwrap());}
 let core:teaql_core::SelectQuery=query.clone().into();let mut out=json!({"query":format!("{core:?}"),"rows":[],"error":null,"scalar":null});
 let pool=std::sync::Arc::new(std::sync::Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));
 pool.lock().unwrap().execute_batch(include_str!("../../../support/dynamic_json_inherited_rows.sql")).unwrap();
 let user_context=driver::block_on(cpp_inheritance_graph_core::service_runtime_from_pool(pool)).unwrap().with_active_root("Platform",1);
 let query=query.comment("inspect inherited JSON criteria").purpose("qualify inherited JSON execution");
 match input["mode"].as_str().unwrap(){
 "count"=>match driver::block_on(query.execute_for_count(&user_context)){Ok(v)=>out["scalar"]=json!(v),Err(e)=>out["error"]=json!(e.to_string())},
 "exists"=>match driver::block_on(query.execute_for_exists(&user_context)){Ok(v)=>out["scalar"]=json!(v),Err(e)=>out["error"]=json!(e.to_string())},
 _=>match driver::block_on(query.execute_for_rows(&user_context)){
 Ok(rows)=>out["rows"]=json!(rows.iter().map(|row|["id","version","name","rank","platform_id","advisor_id","campus_code","budget","school_id","note"].map(|k|row.get(k).map(|v|format!("{v:?}")))).collect::<Vec<_>>()),
 Err(error)=>out["error"]=json!(error.to_string()),}}
 out["sql"]=json!(user_context.sql_logs().iter().map(|entry|json!({"sql":entry.sql,"params":entry.params.iter().map(|v|format!("{v:?}")).collect::<Vec<_>>(),"comment":entry.comment,"purpose":entry.purpose})).collect::<Vec<_>>());out}};}
 match input["entity"].as_str().unwrap(){
 "school"=>emit!(if minimal{Q::schools_minimal()}else{Q::schools()}),
 "academy"=>emit!(if minimal{Q::academies_minimal()}else{Q::academies()}),
 "research_academy"=>emit!(if minimal{Q::research_academies_minimal()}else{Q::research_academies()}),
 _=>emit!(if minimal{Q::enrollments_minimal()}else{Q::enrollments()}),}
}
fn main(){std::panic::set_hook(Box::new(|_|{}));for line in io::stdin().lock().lines(){let input:Json=serde_json::from_str(&line.unwrap()).unwrap();let out=std::panic::catch_unwind(||observe(&input)).unwrap_or_else(|e|json!({"panic":e.downcast_ref::<String>().cloned().unwrap_or_else(||e.downcast_ref::<&str>().unwrap().to_string())}));println!("{out}");}}
