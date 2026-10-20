use cpp_dynamic_json_core::Q;
use serde_json::{json,Value as Json};
use std::{io::{self,BufRead},sync::{Arc,Mutex}};
use teaql_core::{Entity,TeaqlEntity};
#[path="../../rust_field_layout/src/bin/original_like_privacy_5_0_6.rs"] mod driver;
fn snapshot<T:Entity+TeaqlEntity>(entity:T)->Json {
    let descriptor=T::entity_descriptor();
    let mut loaded=serde_json::Map::new();
    for property in &descriptor.properties {loaded.insert(property.name.clone(),json!(entity.is_field_loaded(&property.name)));}
    for relation in &descriptor.relations {loaded.insert(relation.name.clone(),json!(entity.is_field_loaded(&relation.name)));}
    json!({"loaded":loaded,"json":entity.into_json()})
}
fn observe(input:&Json)->Json {
    let pool=Arc::new(Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));
    pool.lock().unwrap().execute_batch(include_str!("../../../support/dynamic_json_schema.sql")).unwrap();
    let user_context=driver::block_on(cpp_dynamic_json_core::service_runtime_from_pool(pool.clone())).unwrap().with_active_root("Platform",1);
    pool.lock().unwrap().execute_batch(include_str!("../../../support/dynamic_json_dataset.sql")).unwrap();
    let minimal=input["minimal"]==true;
    macro_rules! apply {($request:expr,$scalar:ident,$raw:ident,$remove:ident,$group:ident,$relation:ident,$with:ident,$remove_relation:ident,$child:expr)=>{{
        let mut request=$request;
        for op in input["ops"].as_array().unwrap() {
            request=match op.as_str().unwrap() {
                "self"=>request.select_self(),"self_fields"=>request.select_self_fields(),
                "without_parent"=>request.select_self_without_parent(),"all"=>request.select_all(),
                "children"=>request.select_children(),"any"=>request.select_any(),
                "scalar"=>request.$scalar(),"raw"=>request.$raw("'fixed'"),"remove"=>request.$remove(),
                "group_alias"=>request.$group("_scalarAlias"),"relation"=>request.$relation(),
                "relation_with"=>{
                    let mut child=$child.select_name().comment("child projection intent").raw_sql_filter("1=1");
                    child=match input["child_mode"].as_str().unwrap() {
                        "options"=>child.create_property_as("_childComputed","id + 7").raw_sql_filter("1=1"),
                        "deleted"=>child.with_deleted_rows(),
                        "slice"=>child.order_by_id_desc().offset(1,2),
                        _=>child,
                    };
                    match input["conversion"].as_str().unwrap() {
                        "selection"=>{let selected:teaql_core::request::QuerySelection=child.into();request.$with(selected)},
                        "query"=>{let selected:teaql_core::request::QuerySelection=child.into();request.$with(selected.into_query())},
                        _=>request.$with(child),
                    }
                },
                "remove_relation"=>request.$remove_relation(),
                "computed"=>request.create_property_as("_computed","id + 7"),
                "raw_filter"=>request.raw_sql_filter("1=1"),
                "search"=>request.search_with_text("Alpha"),
                "deleted"=>request.with_deleted_rows(),
                _=>panic!("unknown fixture operation"),
            };
        }
        let core:teaql_core::SelectQuery=request.clone().into();
        let mut output=json!({"query":format!("{core:?}"),"options":format!("{:?}",request.query_options()),"selections":format!("{:?}",request.relation_selections()),"rows":[],"error":null});
        let executable=request.comment("inspect projection selection scope").purpose("qualify complete selection composition");
        if input["mode"]=="list" {
            match driver::block_on(executable.execute_for_list(&user_context)) {
                Ok(list)=>output["rows"]=json!(list.into_iter().map(snapshot).collect::<Vec<_>>()),
                Err(error)=>output["error"]=json!(error.to_string()),
            }
        }else{
            match driver::block_on(executable.execute_for_rows(&user_context)) {
                Ok(rows)=>output["rows"]=json!(rows.iter().map(|row|row.iter().map(|(key,value)|(key.clone(),format!("{value:?}"))).collect::<std::collections::BTreeMap<_,_>>()).collect::<Vec<_>>()),
                Err(error)=>output["error"]=json!(error.to_string()),
            }
        }
        output["logs"]=json!(user_context.sql_logs().iter().map(|entry|json!({"sql":entry.sql,"params":entry.params.iter().map(|v|format!("{v:?}")).collect::<Vec<_>>(),"comment":entry.comment,"purpose":entry.purpose,"trace":entry.trace_path.iter().map(|v|format!("{v:?}")).collect::<Vec<_>>()})).collect::<Vec<_>>());
        output
    }};}
    match input["entity"].as_str().unwrap() {
        "person"=>apply!(if minimal{Q::people_minimal()}else{Q::people()},select_name,select_name_raw,unselect_name,group_by_name_as,select_platform,select_platform_with,unselect_platform,Q::platforms()),
        "product"=>apply!(if minimal{Q::products_minimal()}else{Q::products()},select_sku,select_sku_raw,unselect_sku,group_by_sku_as,select_platform,select_platform_with,unselect_platform,Q::platforms()),
        _=>apply!(if minimal{Q::orders_minimal()}else{Q::orders()},select_code,select_code_raw,unselect_code,group_by_code_as,select_customer,select_customer_with,unselect_customer,Q::people()),
    }
}
fn main(){for line in io::stdin().lock().lines(){let input:Json=serde_json::from_str(&line.unwrap()).unwrap();println!("{}",observe(&input));}}
