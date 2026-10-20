//! Invoke original generated membership methods without copying their implementation.
use cpp_dynamic_json_core::Q;
use serde_json::{Value as Json, json};
use std::io::{self, BufRead};
use teaql_core::Expr;
#[path = "original_like_privacy_5_0_6.rs"]
mod driver;

fn flag(input: &Json, key: &str) -> bool {
    input[key].as_bool().unwrap_or(false)
}

fn observe(input: &Json) -> Json {
    macro_rules! child {
        ($query:expr) => {{
            let mut query = $query.filter(Expr::gt("id", input["cutoff"].as_i64().unwrap()));
            if flag(input, "child_deleted") {
                query = query.with_deleted_rows();
            }
            if flag(input, "child_slice") {
                query = query.order_by_id_desc().offset(1, 2);
            }
            if flag(input, "child_group") {
                query = query.group_by("version").count_as("_count");
            }
            if flag(input, "child_relations") {
                query = query.select_all();
            }
            if flag(input, "child_options") {
                query = query
                    .comment("child comment")
                    .create_property_as("_ignored", "77")
                    .raw_sql_filter("1=0")
                    .raw_sql("SELECT 999 AS id");
            }
            query
        }};
    }
    macro_rules! combine {
        ($request:expr, $candidate:expr, $alias:expr) => {{
            match input["conversion"].as_str().unwrap_or("request") {
                "selection" => {
                    let selection: teaql_core::request::QuerySelection = $candidate.clone().into();
                    if $alias {
                        $request.enhance_child(selection)
                    } else {
                        $request.enhance_child(selection)
                    }
                }
                "query" => {
                    let query: teaql_core::SelectQuery = $candidate.clone().into();
                    if $alias {
                        $request.enhance_child(query)
                    } else {
                        $request.enhance_child(query)
                    }
                }
                _ => {
                    if $alias {
                        $request.enhance_child($candidate.clone())
                    } else {
                        $request.enhance_child($candidate.clone())
                    }
                }
            }
        }};
    }
    macro_rules! finish {
        ($parent:expr, $child:expr) => {{
            let child = $child;
            let before: teaql_core::SelectQuery = child.clone().into();
            let mut parent = $parent.filter(Expr::lt("id", 100_i64));
            if flag(input, "outer_deleted") { parent = parent.with_deleted_rows(); }
            parent = combine!(parent, child, flag(input, "alias"));
            if flag(input, "repeat") { parent = combine!(parent, child, false); }
            let after: teaql_core::SelectQuery = child.into();
            let query: teaql_core::SelectQuery = parent.clone().into();
            let mut out = json!({"query":format!("{query:?}"),
                "child_before":format!("{before:?}"), "child_after":format!("{after:?}"),"enhancements":format!("{:?}",parent.child_enhancements())});
            if flag(input, "execute") {
                let pool = std::sync::Arc::new(std::sync::Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));
                pool.lock().unwrap().execute_batch(include_str!("../../../../support/dynamic_json_schema.sql")).unwrap();
                pool.lock().unwrap().execute_batch(include_str!("../../../../support/dynamic_json_dataset.sql")).unwrap();
                let user_context = driver::block_on(cpp_dynamic_json_core::service_runtime_from_pool(pool))
                    .unwrap().with_active_root("Platform", 1);
                match driver::block_on(parent.comment("inspect child enhancement input").purpose("qualify child enhancement conversion").execute_for_rows(&user_context)) {
                    Ok(rows) => { out["rows"] = json!(rows.into_iter().map(|row|
                        ["id", "version"].map(|key|row.get(key).map(|value|format!("{value:?}")))).collect::<Vec<_>>()); out["error"] = Json::Null; }
                    Err(error) => { out["rows"] = json!([]); out["error"] = json!(error.to_string()); }
                }
            }
            out
        }};
    }
    macro_rules! choose_child {
        ($parent:expr) => {{
            match input["child"].as_str().unwrap() {
                "person" => finish!(
                    $parent,
                    child!(if flag(input, "child_minimal") {
                        Q::people_minimal()
                    } else {
                        Q::people()
                    })
                ),
                "order" => finish!(
                    $parent,
                    child!(if flag(input, "child_minimal") {
                        Q::orders_minimal()
                    } else {
                        Q::orders()
                    })
                ),
                "product" => finish!(
                    $parent,
                    child!(if flag(input, "child_minimal") {
                        Q::products_minimal()
                    } else {
                        Q::products()
                    })
                ),
                "school" => finish!(
                    $parent,
                    child!(if flag(input, "child_minimal") {
                        cpp_relations_core::Q::schools_minimal()
                    } else {
                        cpp_relations_core::Q::schools()
                    })
                ),
                _ => unreachable!(),
            }
        }};
    }
    if input["outer"] == "person" {
        choose_child!(if flag(input, "outer_minimal") {
            Q::people_minimal()
        } else {
            Q::people()
        })
    } else {
        choose_child!(if flag(input, "outer_minimal") {
            Q::orders_minimal()
        } else {
            Q::orders()
        })
    }
}

fn main() {
    for line in io::stdin().lock().lines() {
        let input: Json = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", observe(&input));
    }
}
