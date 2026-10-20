//! Original public Q lookup functions; query and execution implementations are untouched.
use cpp_business_ids_core as domain;
use serde_json::{Value as Json, json};
use std::{io::{self, BufRead}, sync::{Arc, Mutex}};
use teaql_core::{Entity, TeaqlEntity, SelectQuery};
#[path="../../../rust_field_layout/src/bin/original_like_privacy_5_0_6.rs"] mod driver;

fn snapshot<T: Entity + TeaqlEntity>(entity: T) -> Json {
    let mut loaded=serde_json::Map::new();
    let descriptor=T::entity_descriptor();
    for field in &descriptor.properties {loaded.insert(field.name.clone(),json!(entity.is_field_loaded(&field.name)));}
    for relation in &descriptor.relations {loaded.insert(relation.name.clone(),json!(entity.is_field_loaded(&relation.name)));}
    json!({"loaded":loaded,"json":entity.into_json()})
}

fn observe(input:&Json)->Json {
    let pool=Arc::new(Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));
    pool.lock().unwrap().execute_batch(include_str!("../../../../support/business_id_lookup_rows.sql")).unwrap();
    let user_context=driver::block_on(domain::service_runtime_from_pool(pool)).unwrap().with_active_root("Tenant",1);
    macro_rules! run {($number:ty,$lookup:ident,$ordinary:ident,$predicate:ident)=>{{
        let number=match <$number>::new(input["text"].as_str().unwrap()) {
            Ok(value)=>value,Err(error)=>return json!({"error":error}),
        };
        let value=number.as_str().to_owned();
        let manual=domain::Q::$ordinary().$predicate(number.clone()).limit(2);
        let before:SelectQuery=manual.clone().into();
        let mut query=domain::Q::$lookup(number);
        let lookup:SelectQuery=query.clone().into();
        query=match input["operation"].as_str().unwrap() {
            "larger"=>query.limit(10),"offset"=>query.skip(2).limit(10),
            "deleted"=>query.with_deleted_rows().limit(10),_=>query,
        };
        let after:SelectQuery=query.clone().into();
        let mut out=json!({"value":value,"manual":format!("{before:?}"),"lookup":format!("{lookup:?}"),
            "after":format!("{after:?}"),"rows":[],"scalar":null,"error":null});
        if input["mode"]!="construct" {
            let request=query.comment("Lookup a business identifier").purpose("Qualify typed lookup scope");
            match input["mode"].as_str().unwrap() {
                "count"=>match driver::block_on(request.execute_for_count(&user_context)) {
                    Ok(value)=>out["scalar"]=json!(value),Err(error)=>out["error"]=json!(error.to_string()),
                },
                "one"=>match driver::block_on(request.execute_for_one(&user_context)) {
                    Ok(value)=>out["rows"]=json!(value.map(snapshot).into_iter().collect::<Vec<_>>()),Err(error)=>out["error"]=json!(error.to_string()),
                },
                "list"=>match driver::block_on(request.execute_for_list(&user_context)) {
                    Ok(rows)=>out["rows"]=json!(rows.into_iter().map(snapshot).collect::<Vec<_>>()),Err(error)=>out["error"]=json!(error.to_string()),
                },
                _=>match driver::block_on(request.execute_for_rows(&user_context)) {
                    Ok(rows)=>out["rows"]=json!(rows.iter().map(|row|row.iter().map(|(key,value)|(key.clone(),format!("{value:?}")))
                        .collect::<std::collections::BTreeMap<_,_>>()).collect::<Vec<_>>()),Err(error)=>out["error"]=json!(error.to_string()),
                },
            }
        }
        out["logs"]=json!(user_context.sql_logs().iter().map(|entry|json!({"sql":entry.sql,
            "params":entry.params.iter().map(|v|format!("{v:?}")).collect::<Vec<_>>(),
            "comment":entry.comment,"purpose":entry.purpose,"debug_sql":entry.debug_sql,
            "pretty_sql":entry.pretty_sql,"context":format!("{:?}",entry.log_context),
            "trace":entry.trace_path.iter().map(|v|format!("{v:?}")).collect::<Vec<_>>(),
            "result_count":entry.result_count,"result_type":entry.result_type,
            "affected_rows":entry.affected_rows,"summary":entry.result_summary})).collect::<Vec<_>>());
        out
    }};}
    match input["entity"].as_str().unwrap() {
        "receipt"=>run!(domain::ReceiptNumber,receipt_by_receipt_number,receipts,with_receipt_number_is),
        "ticket"=>run!(domain::TicketNumber,ticket_by_ticket_number,tickets,with_ticket_number_is),
        _=>unreachable!(),
    }
}
fn main(){for line in io::stdin().lock().lines(){let input:Json=serde_json::from_str(&line.unwrap()).unwrap();println!("{}",observe(&input));}}
