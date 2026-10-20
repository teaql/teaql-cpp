use serde_json::{Value as Json,json};use std::io::{self,BufRead};
#[path="../../rust_field_layout/src/bin/original_like_privacy_5_0_6.rs"] mod driver;
fn observe(input:&Json)->Json {
 let pool=std::sync::Arc::new(std::sync::Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));
 pool.lock().unwrap().execute_batch(include_str!("../../../support/dynamic_json_domain_models.sql")).unwrap();
 macro_rules! emit {($q:expr,$context:expr)=>{{let mut query=$q;for filter in input["filters"].as_array().unwrap(){query=query.filter_with_json(filter.as_str().unwrap());}
 let core:teaql_core::SelectQuery=query.clone().into();let mut out=json!({"query":format!("{core:?}"),"rows":[],"scalar":null,"error":null});
 let user_context=$context;let query=query.comment("inspect domain JSON criteria").purpose("qualify domain JSON execution");
 match input["mode"].as_str().unwrap(){
 "count"=>match driver::block_on(query.execute_for_count(&user_context)){Ok(v)=>out["scalar"]=json!(v),Err(e)=>out["error"]=json!(e.to_string())},
 "exists"=>match driver::block_on(query.execute_for_exists(&user_context)){Ok(v)=>out["scalar"]=json!(v),Err(e)=>out["error"]=json!(e.to_string())},
 _=>match driver::block_on(query.execute_for_rows(&user_context)){Ok(rows)=>out["rows"]=json!(rows.iter().map(|row|["id","version","name","code","phase","tag","status_id","platform_id","tenant_id","receipt_number","ticket_number"].map(|k|row.get(k).map(|v|format!("{v:?}")))).collect::<Vec<_>>()),Err(e)=>out["error"]=json!(e.to_string())}}
 out["sql"]=json!(user_context.sql_logs().iter().map(|entry|json!({"sql":entry.sql,"params":entry.params.iter().map(|v|format!("{v:?}")).collect::<Vec<_>>(),"comment":entry.comment,"purpose":entry.purpose})).collect::<Vec<_>>());out}};}
 let minimal=input["minimal"]==true;match input["entity"].as_str().unwrap(){
 "order_status"=>emit!(if minimal{cpp_candidates_core::Q::order_statuses_minimal()}else{cpp_candidates_core::Q::order_statuses()},driver::block_on(cpp_candidates_core::service_runtime_from_pool(pool)).unwrap().with_active_root("Platform",1)),
 "record"=>emit!(if minimal{cpp_candidates_core::Q::records_minimal()}else{cpp_candidates_core::Q::records()},driver::block_on(cpp_candidates_core::service_runtime_from_pool(pool)).unwrap().with_active_root("Platform",1)),
 "person"=>emit!(if minimal{cpp_candidates_core::Q::people_minimal()}else{cpp_candidates_core::Q::people()},driver::block_on(cpp_candidates_core::service_runtime_from_pool(pool)).unwrap().with_active_root("Platform",1)),
 "receipt"=>emit!(if minimal{cpp_business_ids_core::Q::receipts_minimal()}else{cpp_business_ids_core::Q::receipts()},driver::block_on(cpp_business_ids_core::service_runtime_from_pool(pool)).unwrap().with_active_root("Tenant",1)),
 _=>emit!(if minimal{cpp_business_ids_core::Q::tickets_minimal()}else{cpp_business_ids_core::Q::tickets()},driver::block_on(cpp_business_ids_core::service_runtime_from_pool(pool)).unwrap().with_active_root("Tenant",1)),}
}
fn main(){std::panic::set_hook(Box::new(|_|{}));for line in io::stdin().lock().lines(){let input:Json=serde_json::from_str(&line.unwrap()).unwrap();let out=std::panic::catch_unwind(||observe(&input)).unwrap_or_else(|e|json!({"panic":e.downcast_ref::<String>().cloned().unwrap_or_else(||e.downcast_ref::<&str>().unwrap().to_string())}));println!("{out}");}}
