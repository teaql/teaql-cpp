use cpp_dynamic_json_core::Q;
use serde_json::{json, Value as Json};
use std::{io::{self,BufRead},sync::{Arc,Mutex}};
use teaql_core::{Entity,TeaqlEntity,Expr};
#[path="../../rust_field_layout/src/bin/original_like_privacy_5_0_6.rs"] mod driver;

fn snapshot<T:Entity+TeaqlEntity>(entity:T)->Json {
    let descriptor=T::entity_descriptor();
    let mut loaded=serde_json::Map::new();
    for property in &descriptor.properties {loaded.insert(property.name.clone(),json!(entity.is_field_loaded(&property.name)));}
    for relation in &descriptor.relations {loaded.insert(relation.name.clone(),json!(entity.is_field_loaded(&relation.name)));}
    json!({"loaded":loaded,"json":entity.into_json()})
}
fn row_snapshot(row:&teaql_core::CompactRow)->Json {
    json!(row.iter().map(|(key,value)|(key.clone(),format!("{value:?}"))).collect::<std::collections::BTreeMap<_,_>>())
}
fn observe(input:&Json)->Json {
    let pool=Arc::new(Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));
    pool.lock().unwrap().execute_batch(include_str!("../../../support/dynamic_json_schema.sql")).unwrap();
    let user_context=driver::block_on(cpp_dynamic_json_core::service_runtime_from_pool(pool.clone())).unwrap().with_active_root("Platform",1);
    pool.lock().unwrap().execute_batch(include_str!("../../../support/dynamic_json_dataset.sql")).unwrap();
    let kind=input["kind"].as_str().unwrap();
    let minimal=input["minimal"]==true;let child_minimal=input["child_minimal"]==true;
    macro_rules! configure {($child:expr)=>{{
        let child=$child.filter(Expr::gt("id",0_i64));
        match input["child_mode"].as_str().unwrap() {
            "deleted"=>child.with_deleted_rows(),"slice"=>child.order_by_id_desc().offset(1,2),
            "group"=>child.group_by("version").count_as("_childCount"),
            "options"=>child.comment("child analysis intent").create_property_as("_childComputed","id + 7").raw_sql_filter("1=1"),
            "relations"=>child.select_all(),_=>child,
        }
    }};}
    macro_rules! combine {($parent:expr,$child:expr,$apply:expr)=>{{
        let parent=$parent;let child=$child;
        let before:teaql_core::SelectQuery=child.clone().into();
        let request=match input["conversion"].as_str().unwrap() {
            "selection"=>{let selected:teaql_core::request::QuerySelection=child.clone().into();($apply)(parent,selected)},
            "query"=>{let selected:teaql_core::request::QuerySelection=child.clone().into();($apply)(parent,teaql_core::request::QuerySelection::from(selected.into_query()))},
            _=>{let selected:teaql_core::request::QuerySelection=child.clone().into();($apply)(parent,selected)},
        }.order_by_id_asc();
        let after:teaql_core::SelectQuery=child.into();
        let core:teaql_core::SelectQuery=request.clone().into();
        let mut output=json!({"query":format!("{core:?}"),"options":format!("{:?}",request.query_options()),"child_before":format!("{before:?}"),"child_after":format!("{after:?}"),"rows":[],"facets":{},"error":null,"total":null});
        let executable=request.comment("inspect analysis input conversion").purpose("qualify relation analysis conversions");
        if input["mode"]=="rows" {
            match driver::block_on(executable.execute_for_rows(&user_context)) {
                Ok(rows)=>output["rows"]=json!(rows.iter().map(row_snapshot).collect::<Vec<_>>()),
                Err(error)=>output["error"]=json!(error.to_string()),
            }
        }else{
            let result=if input["mode"]=="page" {driver::block_on(executable.execute_for_page(&user_context,0,2))}else{driver::block_on(executable.execute_for_list(&user_context))};
            match result {
                Ok(list)=>{
                    output["total"]=json!(list.total_count);
                    output["facets"]=json!(list.facets.iter().map(|(name,rows)|(name.clone(),rows.iter().map(row_snapshot).collect::<Vec<_>>())).collect::<std::collections::BTreeMap<_,_>>());
                    output["rows"]=json!(list.into_iter().map(snapshot).collect::<Vec<_>>());
                },
                Err(error)=>output["error"]=json!(error.to_string()),
            }
        }
        output["logs"]=json!(user_context.sql_logs().iter().map(|entry|json!({"sql":entry.sql,"params":entry.params.iter().map(|v|format!("{v:?}")).collect::<Vec<_>>(),"comment":entry.comment,"purpose":entry.purpose,"trace":entry.trace_path.iter().map(|v|format!("{v:?}")).collect::<Vec<_>>()})).collect::<Vec<_>>());
        output
    }};}
    if matches!(kind,"facet"|"facet_options"|"group"|"group_from") {
        combine!(if minimal{Q::orders_minimal()}else{Q::orders()},configure!(if child_minimal{Q::people_minimal()}else{Q::people()}),
            |parent:cpp_dynamic_json_core::OrderRequest,selected:teaql_core::request::QuerySelection|match kind {
                "facet"=>parent.facet_by_customer_as("_customers",selected),
                "facet_options"=>parent.facet_by_customer_as_with_options("_customers",selected,input["include_all"]==true),
                "group"=>parent.group_by_customer_with(selected),
                _=>parent.group_by_customer_with_details_from(selected),
            })
    }else{
        combine!(if minimal{Q::people_minimal()}else{Q::people()},configure!(if child_minimal{Q::orders_minimal()}else{Q::orders()}),
            |parent:cpp_dynamic_json_core::PersonRequest,selected:teaql_core::request::QuerySelection|match kind {
                "count"=>parent.count_orders_with("_metric",selected),
                "stats"=>parent.stats_from_orders(selected),
                "stats_alias"=>parent.stats_from_orders_as("_metric",selected),
                _=>parent.group_by_orders_with_details(selected),
            })
    }
}
fn main(){for line in io::stdin().lock().lines(){let input:Json=serde_json::from_str(&line.unwrap()).unwrap();println!("{}",observe(&input));}}
