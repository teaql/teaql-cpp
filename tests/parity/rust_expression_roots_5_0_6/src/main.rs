use cpp_expression_roots_core::{Q,E};
use serde_json::{json,Value as Json};
use std::{io::{self,BufRead},sync::{Arc,Mutex}};
#[path="../../rust_field_layout/src/bin/original_like_privacy_5_0_6.rs"] mod driver;
fn panic_fields(panic: Box<dyn std::any::Any + Send>) -> Json {
    let text = panic.downcast_ref::<String>().unwrap();
    let mut fields=serde_json::Map::new();
    for line in text.lines() {
        if let Some((name,value))=line.split_once(": ") {
            fields.insert(name.into(),if name=="root" {json!(value)}
                else if name=="human_message" {json!(&value[1..value.len()-1])}
                else {serde_json::from_str(value).unwrap()});
        }
    }
    Json::Object(fields)
}
fn observe(input:&Json)->Json {
    let pool=Arc::new(Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));
    pool.lock().unwrap().execute_batch(include_str!("../../../support/expression_root_sql.sql")).unwrap();
    let user_context=driver::block_on(cpp_expression_roots_core::service_runtime_from_pool(pool)).unwrap().with_active_root("Platform",1);
    macro_rules! run {($query:expr,$entry:ident)=>{{
        let mut query=$query;
        if input["deleted"]==true {query=query.with_deleted_rows();}
        match input["selection"].as_str().unwrap() {
            "name"=>query=query.select_name(),
            "removed"=>query=query.select_name().unselect_name(),
            "replaced"=>query=query.select_name().unselect_name().select_name(),
            "platform_minimal"=>query=query.select_platform_with(Q::platforms_minimal()),
            "platform_named"=>query=query.select_platform_with(Q::platforms_minimal().select_name()),
            "self"=>query=query.select_self(),
            _=>{},
        }
        let core:teaql_core::SelectQuery=query.clone().into();
        let mut out=json!({"query":format!("{core:?}"),"present":false,"value":null,"diagnostic":null,"error":null});
        let result=driver::block_on(query.with_id_is(input["id"].as_u64().unwrap())
            .comment("Inspect expression root identity").purpose("Use the actual generated E entry")
            .execute_for_one(&user_context));
        match result {
            Err(error)=>out["error"]=json!(error.to_string()),
            Ok(None)=>{},
            Ok(Some(entity))=>{
                out["present"]=json!(true);
                let value=std::panic::catch_unwind(std::panic::AssertUnwindSafe(||{
                    let expression=E::$entry(&entity);
                    let field=if input["path"]=="platform" {expression.get_platform().get_name()} else {expression.get_name()};
                    if input["action"]=="eval" {format!("{:?}",field.eval())} else {format!("{:?}",field.unwrap())}
                }));
                match value {Ok(value)=>out["value"]=json!(value),Err(error)=>out["diagnostic"]=panic_fields(error)}
            },
        }
        out["logs"]=json!(user_context.sql_logs().iter().map(|entry|json!({"sql":entry.sql,"params":entry.params.iter().map(|value|format!("{value:?}")).collect::<Vec<_>>(),"comment":entry.comment,"purpose":entry.purpose,"trace":entry.trace_path.iter().map(|node|format!("{node:?}")).collect::<Vec<_>>()})).collect::<Vec<_>>());
        out
    }};}
    match input["entity"].as_str().unwrap() {
        "class"=>run!(Q::classes_minimal(),class),
        "operator"=>run!(Q::operators_minimal(),operator),
        "template"=>run!(Q::templates_minimal(),template),
        "namespace"=>run!(Q::namespaces_minimal(),namespace),
        "plain_record"=>run!(Q::plain_records_minimal(),plain_record),
        _=>panic!("unknown fixture entity"),
    }
}
fn main() {
    std::panic::set_hook(Box::new(|_|{}));
    for line in io::stdin().lock().lines() {
        println!("{}",observe(&serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
