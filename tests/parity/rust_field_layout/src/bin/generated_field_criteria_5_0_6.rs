//! Invoke unchanged generated field/operator and criterion APIs.
use cpp_dynamic_json_core::Q;
use serde_json::{Value as Json, json};
use std::io::{self, BufRead};
use teaql_core::{
    Value,
    request::{DateRange, FieldOperator},
};
#[path = "original_like_privacy_5_0_6.rs"]
mod driver;
#[path = "../../../rust_values/src/fixture_value.rs"]
mod fixture_value;

fn operator(name: &str) -> FieldOperator {
    match name {
        "eq" => FieldOperator::Equal,
        "ne" => FieldOperator::NotEqual,
        "gt" => FieldOperator::GreaterThan,
        "gte" => FieldOperator::GreaterThanOrEqual,
        "lt" => FieldOperator::LessThan,
        "lte" => FieldOperator::LessThanOrEqual,
        "between" => FieldOperator::Between,
        "in" => FieldOperator::In,
        "not_in" => FieldOperator::NotIn,
        "contain" => FieldOperator::Contain,
        "not_contain" => FieldOperator::NotContain,
        "begin" => FieldOperator::BeginWith,
        "not_begin" => FieldOperator::NotBeginWith,
        "end" => FieldOperator::EndWith,
        "not_end" => FieldOperator::NotEndWith,
        "sound" => FieldOperator::SoundsLike,
        "null" => FieldOperator::IsNull,
        "not_null" => FieldOperator::IsNotNull,
        _ => panic!("unknown operator"),
    }
}

fn observe(input: &Json) -> Json {
    let values: Vec<Value> = input["values"]
        .as_array()
        .unwrap()
        .iter()
        .map(fixture_value::value)
        .collect();
    let op = operator(input["operator"].as_str().unwrap());
    let route = input["route"].as_str().unwrap();
    macro_rules! finish {
        ($request:expr, $type:ty, $direct:ident, $create:ident) => {{
            let mut request = $request;
            let mut criterion = Json::Null;
            if route == "criteria" {
                let expr = <$type>::$create(op, values.clone());
                criterion = json!(format!("{expr:?}"));
                request = request.append_search_criteria(expr.clone());
                if input["repeat"] == true {
                    request = request.append_search_criteria(expr);
                }
            } else {
                request = request.$direct(op, values.clone());
                if input["repeat"] == true {
                    request = request.$direct(op, values.clone());
                }
            }
            emit!(request, criterion)
        }};
    }
    macro_rules! range_finish {
        ($request:expr, $before:ident, $after:ident, $range:ident) => {{
            let request = match route {
                "before" => $request.$before(values[0].clone()),
                "after" => $request.$after(values[0].clone()),
                "range" => $request.$range(DateRange::new(values[0].clone(), values[1].clone())),
                _ => panic!("invalid range route"),
            };
            emit!(request, Json::Null)
        }};
    }
    macro_rules! emit {
        ($request:expr, $criterion:expr) => {{
            let request = $request;
            let query: teaql_core::SelectQuery = request.clone().into();
            let mut out = json!({"query":format!("{query:?}"),"criterion":$criterion});
            if input["execute"] == true {
                let pool = std::sync::Arc::new(std::sync::Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));
                pool.lock().unwrap().execute_batch(include_str!("../../../../support/dynamic_json_schema.sql")).unwrap();
                pool.lock().unwrap().execute_batch(include_str!("../../../../support/dynamic_json_dataset.sql")).unwrap();
                let user_context = driver::block_on(cpp_dynamic_json_core::service_runtime_from_pool(pool)).unwrap().with_active_root("Platform",1);
                match driver::block_on(request.comment("inspect field criteria").purpose("qualify field criteria").execute_for_rows(&user_context)) {
                    Ok(rows) => { out["rows"] = json!(rows.into_iter().map(|row|["id","version"].map(|key|row.get(key).map(|value|format!("{value:?}")))).collect::<Vec<_>>()); out["error"] = Json::Null; },
                    Err(error) => { out["rows"] = json!([]); out["error"] = json!(error.to_string()); },
                }
            }
            out
        }};
    }
    let minimal = input["minimal"] == true;
    let ordinary = route == "direct" || route == "criteria";
    if input["entity"] == "person" {
        let request = if minimal {
            Q::people_minimal()
        } else {
            Q::people()
        };
        if ordinary {
            match input["field"].as_str().unwrap() {
                "id" => finish!(
                    request,
                    cpp_dynamic_json_core::PersonRequest<cpp_dynamic_json_core::Person>,
                    whose_id,
                    create_id_criteria
                ),
                "email" => finish!(
                    request,
                    cpp_dynamic_json_core::PersonRequest<cpp_dynamic_json_core::Person>,
                    whose_email,
                    create_email_criteria
                ),
                _ => finish!(
                    request,
                    cpp_dynamic_json_core::PersonRequest<cpp_dynamic_json_core::Person>,
                    whose_name,
                    create_name_criteria
                ),
            }
        } else {
            match input["field"].as_str().unwrap() {
                "email" => range_finish!(
                    request,
                    whose_email_before,
                    whose_email_after,
                    whose_email_between_range
                ),
                _ => range_finish!(
                    request,
                    whose_name_before,
                    whose_name_after,
                    whose_name_between_range
                ),
            }
        }
    } else {
        let request = if minimal {
            Q::orders_minimal()
        } else {
            Q::orders()
        };
        if ordinary {
            match input["field"].as_str().unwrap() {
                "id" => finish!(
                    request,
                    cpp_dynamic_json_core::OrderRequest<cpp_dynamic_json_core::Order>,
                    with_id,
                    create_id_criteria
                ),
                "priority" => finish!(
                    request,
                    cpp_dynamic_json_core::OrderRequest<cpp_dynamic_json_core::Order>,
                    with_priority,
                    create_priority_criteria
                ),
                _ => finish!(
                    request,
                    cpp_dynamic_json_core::OrderRequest<cpp_dynamic_json_core::Order>,
                    with_code,
                    create_code_criteria
                ),
            }
        } else {
            match input["field"].as_str().unwrap() {
                "priority" => range_finish!(
                    request,
                    with_priority_before,
                    with_priority_after,
                    with_priority_between_range
                ),
                _ => range_finish!(
                    request,
                    with_code_before,
                    with_code_after,
                    with_code_between_range
                ),
            }
        }
    }
}

fn main() {
    std::panic::set_hook(Box::new(|_| {}));
    for line in io::stdin().lock().lines() {
        let input: Json = serde_json::from_str(&line.unwrap()).unwrap();
        let out = std::panic::catch_unwind(std::panic::AssertUnwindSafe(||observe(&input))).unwrap_or_else(|error|json!({"panic":if let Some(s)=error.downcast_ref::<String>() {s.clone()} else {error.downcast_ref::<&str>().unwrap().to_string()}}));
        println!("{out}");
    }
}
