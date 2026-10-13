//! Invoke frozen generated aggregation methods and observe full queries and SQLite rows.
use cpp_dynamic_json_core::Q;
use serde_json::{Value as Json, json};
use std::io::{self, BufRead};
#[path = "original_like_privacy_5_0_6.rs"]
mod driver;
use teaql_core::{AggregateFunction, Expr};
fn observe(input: &Json) -> Json {
    if input["entity"] == "profile" {
        return observe_nullable(input);
    }
    let mut request = if input["minimal"] == true {
        Q::orders_minimal()
    } else {
        Q::orders()
    };
    for operation in input["ops"].as_array().unwrap() {
        let field = operation["field"].as_str().unwrap_or("priority");
        let alias = operation["alias"].as_str().unwrap_or("metric");
        let duration = operation["duration"].as_u64().unwrap_or(0);
        let function = match operation["function"].as_str().unwrap_or("Count") {
            "Count" => AggregateFunction::Count,
            "Sum" => AggregateFunction::Sum,
            "Avg" => AggregateFunction::Avg,
            "Min" => AggregateFunction::Min,
            "Max" => AggregateFunction::Max,
            "Stddev" => AggregateFunction::Stddev,
            "StddevPop" => AggregateFunction::StddevPop,
            "VarSamp" => AggregateFunction::VarSamp,
            "VarPop" => AggregateFunction::VarPop,
            "BitAnd" => AggregateFunction::BitAnd,
            "BitOr" => AggregateFunction::BitOr,
            "BitXor" => AggregateFunction::BitXor,
            _ => panic!("unknown function"),
        };
        request = match operation["op"].as_str().unwrap() {
            "group" => request.group_by(field),
            "count" => request.count(),
            "count_as" => request.count_as(alias),
            "aggregate_count" => request.aggregate_count(alias),
            "count_field" => request.aggregate_count_field(field, alias),
            "function" => request.aggregate_with_function(field, alias, function),
            "cache" => request.enable_aggregation_cache(),
            "cache_for" => request.enable_aggregation_cache_for(duration),
            "propagate" => request.propagate_aggregation_cache(duration),
            "empty" => request.filter(Expr::lt("id", 0_i64)),
            "with_deleted" => request.with_deleted_rows(),
            "unlimited" => request.unlimited(),
            "aggregate_sum" => request.aggregate_sum(field, alias),
            "aggregate_avg" => request.aggregate_avg(field, alias),
            "aggregate_min" => request.aggregate_min(field, alias),
            "aggregate_max" => request.aggregate_max(field, alias),
            "aggregate_stddev" => request.aggregate_stddev(field, alias),
            "aggregate_stddev_pop" => request.aggregate_stddev_pop(field, alias),
            "aggregate_var_samp" => request.aggregate_var_samp(field, alias),
            "aggregate_var_pop" => request.aggregate_var_pop(field, alias),
            "aggregate_bit_and" => request.aggregate_bit_and(field, alias),
            "aggregate_bit_or" => request.aggregate_bit_or(field, alias),
            "aggregate_bit_xor" => request.aggregate_bit_xor(field, alias),
            "group_by_id" => request.group_by_id(),
            "group_by_id_as" => request.group_by_id_as(alias),
            "group_by_id_with_function" => request.group_by_id_with_function(alias, function),
            "count_id" => request.count_id(),
            "count_id_as" => request.count_id_as(alias),
            "sum_id" => request.sum_id(),
            "sum_id_as" => request.sum_id_as(alias),
            "avg_id" => request.avg_id(),
            "avg_id_as" => request.avg_id_as(alias),
            "min_id" => request.min_id(),
            "min_id_as" => request.min_id_as(alias),
            "max_id" => request.max_id(),
            "max_id_as" => request.max_id_as(alias),
            "group_by_version" => request.group_by_version(),
            "group_by_version_as" => request.group_by_version_as(alias),
            "group_by_version_with_function" => {
                request.group_by_version_with_function(alias, function)
            }
            "count_version" => request.count_version(),
            "count_version_as" => request.count_version_as(alias),
            "sum_version" => request.sum_version(),
            "sum_version_as" => request.sum_version_as(alias),
            "avg_version" => request.avg_version(),
            "avg_version_as" => request.avg_version_as(alias),
            "min_version" => request.min_version(),
            "min_version_as" => request.min_version_as(alias),
            "max_version" => request.max_version(),
            "max_version_as" => request.max_version_as(alias),
            "group_by_priority" => request.group_by_priority(),
            "group_by_priority_as" => request.group_by_priority_as(alias),
            "group_by_priority_with_function" => {
                request.group_by_priority_with_function(alias, function)
            }
            "count_priority" => request.count_priority(),
            "count_priority_as" => request.count_priority_as(alias),
            "sum_priority" => request.sum_priority(),
            "sum_priority_as" => request.sum_priority_as(alias),
            "avg_priority" => request.avg_priority(),
            "avg_priority_as" => request.avg_priority_as(alias),
            "min_priority" => request.min_priority(),
            "min_priority_as" => request.min_priority_as(alias),
            "max_priority" => request.max_priority(),
            "max_priority_as" => request.max_priority_as(alias),
            "group_by_amount" => request.group_by_amount(),
            "group_by_amount_as" => request.group_by_amount_as(alias),
            "group_by_amount_with_function" => {
                request.group_by_amount_with_function(alias, function)
            }
            "count_amount" => request.count_amount(),
            "count_amount_as" => request.count_amount_as(alias),
            "sum_amount" => request.sum_amount(),
            "sum_amount_as" => request.sum_amount_as(alias),
            "avg_amount" => request.avg_amount(),
            "avg_amount_as" => request.avg_amount_as(alias),
            "min_amount" => request.min_amount(),
            "min_amount_as" => request.min_amount_as(alias),
            "max_amount" => request.max_amount(),
            "max_amount_as" => request.max_amount_as(alias),
            "group_by_code" => request.group_by_code(),
            "group_by_code_as" => request.group_by_code_as(alias),
            "group_by_code_with_function" => request.group_by_code_with_function(alias, function),
            "count_code" => request.count_code(),
            "count_code_as" => request.count_code_as(alias),
            "sum_code" => request.sum_code(),
            "sum_code_as" => request.sum_code_as(alias),
            "avg_code" => request.avg_code(),
            "avg_code_as" => request.avg_code_as(alias),
            "min_code" => request.min_code(),
            "min_code_as" => request.min_code_as(alias),
            "max_code" => request.max_code(),
            "max_code_as" => request.max_code_as(alias),
            "group_by_customer" => request.group_by_customer(),
            "group_by_customer_as" => request.group_by_customer_as(alias),
            "group_by_customer_with_function" => {
                request.group_by_customer_with_function(alias, function)
            }
            "count_customer" => request.count_customer(),
            "count_customer_as" => request.count_customer_as(alias),
            "standard_deviation_amount" => request.standard_deviation_amount(),
            "standard_deviation_amount_as" => request.standard_deviation_amount_as(alias),
            "square_root_of_population_standard_deviation_amount" => {
                request.square_root_of_population_standard_deviation_amount()
            }
            "square_root_of_population_standard_deviation_amount_as" => {
                request.square_root_of_population_standard_deviation_amount_as(alias)
            }
            "sample_variance_amount" => request.sample_variance_amount(),
            "sample_variance_amount_as" => request.sample_variance_amount_as(alias),
            "sample_population_variance_amount" => request.sample_population_variance_amount(),
            "sample_population_variance_amount_as" => {
                request.sample_population_variance_amount_as(alias)
            }
            "standard_deviation_priority" => request.standard_deviation_priority(),
            "standard_deviation_priority_as" => request.standard_deviation_priority_as(alias),
            "square_root_of_population_standard_deviation_priority" => {
                request.square_root_of_population_standard_deviation_priority()
            }
            "square_root_of_population_standard_deviation_priority_as" => {
                request.square_root_of_population_standard_deviation_priority_as(alias)
            }
            "sample_variance_priority" => request.sample_variance_priority(),
            "sample_variance_priority_as" => request.sample_variance_priority_as(alias),
            "sample_population_variance_priority" => request.sample_population_variance_priority(),
            "sample_population_variance_priority_as" => {
                request.sample_population_variance_priority_as(alias)
            }
            _ => panic!("unknown operation"),
        };
    }
    let query: teaql_core::SelectQuery = request.clone().into();
    let mut out = json!({"query": format!("{query:?}")});
    if input["execute"] == true {
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
        let user_context = driver::block_on(cpp_dynamic_json_core::service_runtime_from_pool(pool))
            .unwrap()
            .with_active_root("Platform", 1);
        let result = driver::block_on(
            request
                .comment("inspect aggregation")
                .purpose("qualify aggregation")
                .execute_for_rows(&user_context),
        );
        match result {
            Ok(rows) => {
                out["rows"] = json!(
                    rows.into_iter()
                        .map(|row| input["columns"]
                            .as_array()
                            .unwrap()
                            .iter()
                            .map(|field| row
                                .get(field.as_str().unwrap())
                                .map(|value| format!("{value:?}")))
                            .collect::<Vec<_>>())
                        .collect::<Vec<_>>()
                );
                out["error"] = Json::Null;
            }
            Err(error) => {
                out["rows"] = json!([]);
                out["error"] = json!(error.to_string());
            }
        }
    }
    out
}
fn observe_nullable(input: &Json) -> Json {
    let mut request = if input["minimal"] == true {
        Q::profiles_minimal()
    } else {
        Q::profiles()
    };
    for operation in input["ops"].as_array().unwrap() {
        let alias = operation["alias"].as_str().unwrap_or("metric");
        request = match operation["op"].as_str().unwrap() {
            "count" => request.count(),
            "count_as" => request.count_as(alias),
            "empty" => request.filter(Expr::lt("id", 0_i64)),
            "all_null" => request.filter(Expr::is_null("note")),
            "with_deleted" => request.with_deleted_rows(),
            "unlimited" => request.unlimited(),
            "count_note" => request.count_note(),
            "count_note_as" => request.count_note_as(alias),
            "sum_note" => request.sum_note(),
            "sum_note_as" => request.sum_note_as(alias),
            "avg_note" => request.avg_note(),
            "avg_note_as" => request.avg_note_as(alias),
            "min_note" => request.min_note(),
            "min_note_as" => request.min_note_as(alias),
            "max_note" => request.max_note(),
            "max_note_as" => request.max_note_as(alias),
            "group_by_note" => request.group_by_note(),
            "group_by_note_as" => request.group_by_note_as(alias),
            "group_by_note_with_function" => {
                request.group_by_note_with_function(alias, AggregateFunction::Count)
            }
            _ => panic!("unknown nullable operation"),
        };
    }
    let query: teaql_core::SelectQuery = request.clone().into();
    let mut out = json!({"query": format!("{query:?}")});
    if input["execute"] == true {
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
        let user_context = driver::block_on(cpp_dynamic_json_core::service_runtime_from_pool(pool))
            .unwrap()
            .with_active_root("Platform", 1);
        let result = driver::block_on(
            request
                .comment("inspect aggregation")
                .purpose("qualify aggregation")
                .execute_for_rows(&user_context),
        );
        match result {
            Ok(rows) => {
                out["rows"] = json!(
                    rows.into_iter()
                        .map(|row| input["columns"]
                            .as_array()
                            .unwrap()
                            .iter()
                            .map(|field| row
                                .get(field.as_str().unwrap())
                                .map(|value| format!("{value:?}")))
                            .collect::<Vec<_>>())
                        .collect::<Vec<_>>()
                );
                out["error"] = Json::Null;
            }
            Err(error) => {
                out["rows"] = json!([]);
                out["error"] = json!(error.to_string());
            }
        }
    }
    out
}

fn main() {
    std::panic::set_hook(Box::new(|_| {}));
    for line in io::stdin().lock().lines() {
        let input: Json = serde_json::from_str(&line.unwrap()).unwrap();
        let out = std::panic::catch_unwind(std::panic::AssertUnwindSafe(||observe(&input))).unwrap_or_else(|error|json!({"panic":if let Some(value)=error.downcast_ref::<String>() {value.clone()} else {error.downcast_ref::<&str>().unwrap().to_string()}}));
        println!("{out}");
    }
}
