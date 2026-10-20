//! Construct actual frozen generated Q surfaces; retain full SelectQuery Debug.
use cpp_dynamic_json_core::Q;
use serde_json::{Value as Json, json};
use std::io::{self, BufRead};
#[path = "original_like_privacy_5_0_6.rs"]
mod driver;
use teaql_core::{
    Expr,
    request::{FieldOperator, UnsafeRawSqlSegment},
};
fn operator(name: &str) -> FieldOperator {
    match name {
        "eq" => FieldOperator::Equal,
        "ne" => FieldOperator::NotEqual,
        "gt" => FieldOperator::GreaterThan,
        "gte" => FieldOperator::GreaterThanOrEqual,
        "lt" => FieldOperator::LessThan,
        "lte" => FieldOperator::LessThanOrEqual,
        "contain" => FieldOperator::Contain,
        "not_contain" => FieldOperator::NotContain,
        "begin" => FieldOperator::BeginWith,
        "not_begin" => FieldOperator::NotBeginWith,
        "end" => FieldOperator::EndWith,
        "not_end" => FieldOperator::NotEndWith,
        "between" => FieldOperator::Between,
        "in" => FieldOperator::In,
        "null" => FieldOperator::IsNull,
        _ => FieldOperator::SoundsLike,
    }
}
fn observe(input: &Json) -> Json {
    let user_context = if input["execute"] == true {
        let pool = std::sync::Arc::new(std::sync::Mutex::new(
            rusqlite::Connection::open_in_memory().unwrap(),
        ));
        pool.lock()
            .unwrap()
            .execute_batch(include_str!("../../../../support/dynamic_json_schema.sql"))
            .unwrap();
        pool.lock()
            .unwrap()
            .execute_batch(include_str!("../../../../support/dynamic_json_dataset.sql"))
            .unwrap();
        Some(
            driver::block_on(cpp_dynamic_json_core::service_runtime_from_pool(pool))
                .unwrap()
                .with_active_root("Platform", 1),
        )
    } else {
        None
    };
    macro_rules! apply {
        ($request:expr, $select:ident, $project:ident, $raw:ident, $unsafe_raw:ident, $unselect:ident,
         $negative:ident, $not_start:ident, $not_end:ident, $sound:ident, $gbk:ident) => {{
            let mut request = $request;
            for operation in input["ops"].as_array().unwrap() {
                let name=operation["op"].as_str().unwrap();
                let number=operation.get("number").and_then(Json::as_u64).unwrap_or(0);
                let size=operation.get("size").and_then(Json::as_u64).unwrap_or(0);
                let value=operation.get("value").and_then(Json::as_str).unwrap_or("Alpha");
                request = match name {
                    "filter"=>request.filter(Expr::eq("code",value)),
                    "live"=>request.and_filter(Expr::gt("version",0_i64)),
                    "or_live"=>request.or_filter(Expr::gt("version",0_i64)),
                    "append"=>request.append_search_criteria(Expr::eq("code",value)),
                    "with_deleted"=>request.with_deleted_rows(),"deleted_only"=>request.deleted_rows_only(),
                    "property"=>request.filter_property("priority",operator(value),"id"),
                    "page_offset"=>request.page_offset(number,size),"offset"=>request.offset(number,size),
                    "offset_only"=>request.offset_only(number),"offset_size"=>request.offset_size(number,size),
                    "top"=>request.top(size),"page_number"=>request.page_number(number,size),
                    "page_number_default"=>request.page_number_default(number),"page"=>request.page(number,size),
                    "page_default"=>request.page_default(number),"unlimited"=>request.unlimited(),
                    "stream_default"=>request.stream_default(),"threshold"=>request.top_n_probe_parent_threshold(number as usize),
                    "select"=>request.$select(),"project"=>request.$project(),"raw"=>request.$raw(value),
                    "unsafe_raw"=>request.$unsafe_raw(UnsafeRawSqlSegment::trusted(value)),"unselect"=>request.$unselect(),
                    "negative"=>request.$negative(value),"not_start"=>request.$not_start(value),"not_end"=>request.$not_end(value),
                    "sound"=>request.$sound(value),"gbk"=>request.$gbk(),
                    "raw_sql"=>request.raw_sql(value),"unsafe_sql"=>request.unsafe_raw_sql(UnsafeRawSqlSegment::trusted(value)),
                    "raw_filter"=>request.raw_sql_filter(value),"unsafe_filter"=>request.unsafe_raw_sql_filter(UnsafeRawSqlSegment::trusted(value)),
                    "property_as"=>request.create_property_as("_custom",value),
                    "unsafe_property_as"=>request.unsafe_create_property_as("_custom",UnsafeRawSqlSegment::trusted(value)),
                    _=>request,
                };
            }
            let query:teaql_core::SelectQuery=request.clone().into();
            let mut out=json!({"query":format!("{query:?}")});
            if let Some(user_context)=user_context.as_ref() {
                let result=driver::block_on(request.comment("inspect general query").purpose("qualify general query").execute_for_rows(user_context));
                match result {
                    Ok(rows)=>{out["rows"]=json!(rows.into_iter().map(|row|["id","version",if input["entity"]=="person"{"name"}else{"code"},"_custom"].map(|field|row.get(field).map(|value|format!("{value:?}")))).collect::<Vec<_>>());out["error"]=Json::Null;},
                    Err(error)=>{out["rows"]=json!([]);out["error"]=json!(error.to_string());}
                }
            }
            out
        }};
    }
    let minimal = input["minimal"].as_bool().unwrap();
    if input["entity"] == "person" {
        apply!(
            if minimal {
                Q::people_minimal()
            } else {
                Q::people()
            },
            select_name,
            project_name,
            select_name_raw,
            select_name_unsafe_raw,
            unselect_name,
            whose_name_not_contains,
            whose_name_not_starts_with,
            whose_name_not_ends_with,
            whose_name_sounds_like,
            order_by_name_asc_using_gbk
        )
    } else {
        apply!(
            if minimal {
                Q::orders_minimal()
            } else {
                Q::orders()
            },
            select_code,
            project_code,
            select_code_raw,
            select_code_unsafe_raw,
            unselect_code,
            with_code_not_containing,
            with_code_not_starting_with,
            with_code_not_ending_with,
            with_code_sounding_like,
            order_by_code_asc_using_gbk
        )
    }
}
fn main() {
    std::panic::set_hook(Box::new(|_| {}));
    for line in io::stdin().lock().lines() {
        let input: Json = serde_json::from_str(&line.unwrap()).unwrap();
        let result=std::panic::catch_unwind(std::panic::AssertUnwindSafe(||observe(&input)))
            .unwrap_or_else(|error|json!({"panic":if let Some(value)=error.downcast_ref::<String>(){value.clone()}else{error.downcast_ref::<&str>().unwrap().to_string()}}));
        println!("{result}");
    }
}
