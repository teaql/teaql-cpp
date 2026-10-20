//! Actual unchanged generated execution methods on the existing SQLite fixture.
use cpp_dynamic_json_core::Q;
use serde_json::{Value as Json, json};
use std::io::{self, BufRead};
use std::sync::{Arc, Mutex};

#[path = "original_like_privacy_5_0_6.rs"]
mod driver;

fn observe(input: &Json) -> Json {
    let pool = Arc::new(Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));
    pool.lock()
        .unwrap()
        .execute_batch(include_str!("../../../../support/dynamic_json_schema.sql"))
        .unwrap();
    pool.lock()
        .unwrap()
        .execute_batch(include_str!("../../../../support/dynamic_json_dataset.sql"))
        .unwrap();
    let user_context = driver::block_on(cpp_dynamic_json_core::service_runtime_from_pool(
        pool.clone(),
    ))
    .unwrap()
    .with_active_root("Platform", 1);
    if input["missing_table"] == true {
        let table = match input["entity"].as_str().unwrap() {
            "person" => "person_data",
            "product" => "product_data",
            "line" => "line_data",
            _ => "order_data",
        };
        pool.lock()
            .unwrap()
            .execute_batch(&format!("ALTER TABLE {table} RENAME TO hidden_{table}"))
            .unwrap();
    }
    macro_rules! pagination {
        ($request:expr) => {{
            let mut request = $request;
            if let Some(operations) = input["pagination"].as_array() {
                for operation in operations {
                    let number = operation["number"].as_u64().unwrap();
                    let size = operation["size"].as_u64().unwrap();
                    request = match operation["op"].as_str().unwrap() {
                        "limit" => request.limit(size), "skip" => request.skip(number),
                        "offset_only" => request.offset_only(number), "offset" => request.offset(number,size),
                        "page_offset" => request.page_offset(number,size), "offset_size" => request.offset_size(number,size),
                        "top" => request.top(size), "page_number" => request.page_number(number,size),
                        "page_number_default" => request.page_number_default(number), "page" => request.page(number,size),
                        "page_default" => request.page_default(number), "unlimited" => request.unlimited(),
                        _ => panic!("unknown pagination operation"),
                    };
                }
            }
            request
        }};
    }
    macro_rules! execute {
        ($query:expr) => {{
            let mut request = $query;
            for filter in input["filters"].as_array().unwrap() {
                request = request.filter_with_json(filter.as_str().unwrap());
            }
            if input["optimization"] == "id_set" {
                request = request.optimize_pagination_with_id_set_config("execution_fixture", 60, input["max_ids"].as_u64().unwrap());
            } else if input["optimization"] == "continuous" {
                request = request.optimize_for_continuous_page_fetch_with("execution_fixture", 60);
            }
            let request = if input["after_purpose"]==true {
                // Original Rust exposes plain modifiers through public inner.
                // Restore the purpose after applying the agreed C++ staged facade adaptation.
                let staged = request.comment("inspect generated execution").purpose("qualify generated execution");
                let purpose = staged.purpose;
                pagination!(staged.inner).purpose(purpose)
            } else {
                pagination!(request).comment("inspect generated execution").purpose("qualify generated execution")
            };
            let mut out = json!({"value":null,"total_count":null,"error":null});
            match input["mode"].as_str().unwrap() {
                "count" => match driver::block_on(request.execute_for_count(&user_context)) {
                    Ok(value) => out["value"] = json!(value),
                    Err(error) => out["error"] = json!(error.to_string()),
                },
                "exists" => match driver::block_on(request.execute_for_exists(&user_context)) {
                    Ok(value) => out["value"] = json!(value),
                    Err(error) => out["error"] = json!(error.to_string()),
                },
                "first" | "one" => {
                    let result = if input["mode"] == "first" { driver::block_on(request.execute_for_first(&user_context)) }
                                 else { driver::block_on(request.execute_for_one(&user_context)) };
                    match result {
                        Ok(value) => out["value"] = json!(value.map(|entity|[entity.id() as i64, entity.version()])),
                        Err(error) => out["error"] = json!(error.to_string()),
                    }
                },
                mode => {
                    let result = if mode == "page" {
                        driver::block_on(request.execute_for_page(&user_context, input["offset"].as_u64().unwrap(), input["limit"].as_u64().unwrap()))
                    } else { driver::block_on(request.execute_for_list(&user_context)) };
                    match result {
                        Ok(list) => {
                            out["total_count"] = json!(list.total_count);
                            out["value"] = json!(list.into_iter().map(|entity|[entity.id() as i64, entity.version()]).collect::<Vec<_>>());
                        },
                        Err(error) => out["error"] = json!(error.to_string()),
                    }
                }
            }
            out["sql"] = json!(user_context.sql_logs().into_iter().map(|entry|json!({"sql":entry.sql,"params":entry.params.iter().map(|value|format!("{value:?}")).collect::<Vec<_>>(),"comment":entry.comment,"purpose":entry.purpose})).collect::<Vec<_>>());
            out
        }};
    }
    macro_rules! execute_result {
        ($query:expr) => {{
            if input["return_type"] == "person" {
                execute!($query.return_type::<cpp_dynamic_json_core::Person>())
            } else {
                execute!($query)
            }
        }};
    }
    let minimal = input["minimal"].as_bool().unwrap();
    match input["entity"].as_str().unwrap() {
        "person" => execute_result!(if minimal {
            Q::people_minimal()
        } else {
            Q::people()
        }),
        "product" => execute_result!(if minimal {
            Q::products_minimal()
        } else {
            Q::products()
        }),
        "line" => execute_result!(if minimal {
            Q::lines_minimal()
        } else {
            Q::lines()
        }),
        _ => execute_result!(if minimal {
            Q::orders_minimal()
        } else {
            Q::orders()
        }),
    }
}

fn main() {
    std::panic::set_hook(Box::new(|_| {}));
    for line in io::stdin().lock().lines() {
        let input: Json = serde_json::from_str(&line.unwrap()).unwrap();
        let output = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| observe(&input)))
            .unwrap_or_else(|error| json!({"panic": if let Some(message) = error.downcast_ref::<String>() {message.clone()} else {error.downcast_ref::<&str>().unwrap().to_string()}}));
        println!("{}", output);
    }
}
