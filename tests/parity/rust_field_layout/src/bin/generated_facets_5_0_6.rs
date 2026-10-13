//! Frozen generated facet requests, exact query state and actual SQL/attachment.
use cpp_dynamic_json_core::Q;
use serde_json::{Value as Json, json};
use std::io::{self, BufRead};
use teaql_core::Expr;
#[path = "original_like_privacy_5_0_6.rs"]
mod driver;
fn flag(input: &Json, name: &str) -> bool {
    input[name] == true
}
struct WithoutPerson;
impl teaql_runtime::EntityRegistry for WithoutPerson {
    fn contains(&self, entity: &str) -> bool {
        entity != "Person"
    }
}
fn observe(input: &Json) -> Json {
    let mut facet = if flag(input, "facet_minimal") {
        Q::people_minimal()
    } else {
        Q::people()
    };
    if flag(input, "facet_filter") {
        facet = facet.filter(Expr::gt("id", 2_i64));
    }
    if flag(input, "facet_deleted") {
        facet = facet.with_deleted_rows();
    }
    if flag(input, "facet_slice") {
        facet = facet.page_offset(1, 1);
    }
    if flag(input, "facet_comment") {
        facet = facet.comment("facet own comment");
    }
    if flag(input, "facet_raw") {
        facet = facet.create_property_as("_custom", "42");
    }
    if flag(input, "facet_membership") {
        facet = facet.with_orders_matching(Q::orders().filter(Expr::gt("amount", 9_i64)));
    }
    let mut aggregate = Q::orders().unlimited();
    if flag(input, "aggregate_filter") {
        aggregate = aggregate.filter(Expr::gt("amount", 9_i64));
    }
    if flag(input, "aggregate_options") {
        aggregate = aggregate
            .comment("aggregate own comment")
            .create_property_as("_custom", "43");
    }
    if flag(input, "aggregate_group") {
        aggregate = aggregate.group_by_priority().count_as("_group_count");
    }
    if flag(input, "aggregate_nested") {
        aggregate = aggregate.group_by_customer_with_details_from(Q::people_minimal());
    }
    if flag(input, "with_count") {
        facet = facet.count_orders_with("_metric", aggregate.clone());
    }
    if flag(input, "with_stats") {
        facet = facet.stats_from_orders_as("_stats", aggregate.clone());
    }
    if flag(input, "with_sum") {
        facet = facet.sum_amount_of_orders_as("_sum", aggregate.clone());
    }
    if flag(input, "with_nested_facet") {
        facet = facet.facet_by_platform_as("_nested", Q::platforms());
    }
    if flag(input, "facet_order") {
        facet = facet.order_by_id_desc();
    }
    let mut request = if flag(input, "minimal") {
        Q::orders_minimal()
    } else {
        Q::orders()
    };
    if flag(input, "outer_filter") {
        request = request.filter(Expr::gt("amount", 9_i64));
    }
    if flag(input, "outer_deleted") {
        request = request.with_deleted_rows();
    }
    if flag(input, "empty") {
        request = request.filter(Expr::lt("id", 0_i64));
    }
    request = request.order_by_id_asc();
    let name = input["name"].as_str().unwrap_or("_customers");
    request = if flag(input, "default_options") {
        request.facet_by_customer_as(name, facet.clone())
    } else {
        request.facet_by_customer_as_with_options(name, facet.clone(), flag(input, "include_all"))
    };
    if flag(input, "repeat") {
        request = request.facet_by_customer_as_with_options(
            name,
            facet.clone().with_deleted_rows(),
            true,
        );
    }
    if flag(input, "second") {
        request = request.facet_by_seller_as("_sellers", Q::companies());
    }
    let query: teaql_core::SelectQuery = request.clone().into();
    let mut out = json!({"query":format!("{query:?}")});
    if flag(input, "execute") {
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
        if flag(input, "missing_outer_table") {
            pool.lock()
                .unwrap()
                .execute_batch("ALTER TABLE order_data RENAME TO hidden_order")
                .unwrap();
        }
        if flag(input, "missing_facet_table") {
            pool.lock()
                .unwrap()
                .execute_batch("ALTER TABLE person_data RENAME TO hidden_person")
                .unwrap();
        }
        let mut user_context =
            driver::block_on(cpp_dynamic_json_core::service_runtime_from_pool(pool))
                .unwrap()
                .with_active_root("Platform", 1);
        if flag(input, "missing_relation") {
            use teaql_core::TeaqlEntity;
            let mut metadata = teaql_runtime::InMemoryMetadataStore::new();
            let mut outer = cpp_dynamic_json_core::Order::entity_descriptor();
            outer
                .relations
                .retain(|relation| relation.name != "customer");
            metadata.register(outer);
            metadata.register(cpp_dynamic_json_core::Person::entity_descriptor());
            user_context.set_metadata(metadata);
        }
        if flag(input, "missing_facet_registry") {
            user_context.set_entity_registry(WithoutPerson);
        }
        let request = request.comment("inspect facets").purpose("qualify facets");
        out["error"] = Json::Null;
        out["ids"] = json!([]);
        out["facets"] = json!({});
        out["total"] = Json::Null;
        let mode = input["mode"].as_str().unwrap_or("list");
        if mode == "one" {
            match driver::block_on(request.execute_for_one(&user_context)) {
                Ok(row) => out["ids"] = json!(row.into_iter().map(|v| v.id()).collect::<Vec<_>>()),
                Err(error) => out["error"] = json!(error.to_string()),
            }
        } else if mode == "rows" {
            match driver::block_on(request.execute_for_rows(&user_context)) {
                Ok(rows) => {
                    out["ids"] = json!(
                        rows.into_iter()
                            .map(|r| r.get("id").unwrap().try_u64().unwrap())
                            .collect::<Vec<_>>()
                    )
                }
                Err(error) => out["error"] = json!(error.to_string()),
            }
        } else if mode == "count" {
            match driver::block_on(request.execute_for_count(&user_context)) {
                Ok(value) => out["count"] = json!(value),
                Err(error) => out["error"] = json!(error.to_string()),
            }
        } else if mode == "exists" {
            match driver::block_on(request.execute_for_exists(&user_context)) {
                Ok(value) => out["exists"] = json!(value),
                Err(error) => out["error"] = json!(error.to_string()),
            }
        } else {
            let result = if mode == "page" {
                driver::block_on(request.execute_for_page(&user_context, 1, 1))
            } else {
                driver::block_on(request.execute_for_list(&user_context))
            };
            match result {
                Ok(rows) => {
                    out["total"] = json!(rows.total_count);
                    out["ids"] = json!(rows.iter().map(|v| v.id()).collect::<Vec<_>>());
                    for (name, facet) in rows.facets {
                        out["facets"][name] = json!(
                            facet
                                .iter()
                                .map(|row| [
                                    "id",
                                    "version",
                                    "name",
                                    "platform_id",
                                    "_metric",
                                    "_stats",
                                    "_sum",
                                    "_custom",
                                    "_nested"
                                ]
                                .map(|key| row.get(key).map(|v| format!("{v:?}"))))
                                .collect::<Vec<_>>()
                        );
                    }
                }
                Err(error) => out["error"] = json!(error.to_string()),
            }
        }
        out["sql"]=json!(user_context.sql_logs().into_iter().map(|entry|json!({"sql":entry.sql,"params":entry.params.iter().map(|v|format!("{v:?}")).collect::<Vec<_>>(),"comment":entry.comment,"purpose":entry.purpose})).collect::<Vec<_>>());
    }
    out
}
fn main() {
    std::panic::set_hook(Box::new(|_| {}));
    for line in io::stdin().lock().lines() {
        let input: Json = serde_json::from_str(&line.unwrap()).unwrap();
        let out=std::panic::catch_unwind(std::panic::AssertUnwindSafe(||observe(&input))).unwrap_or_else(|error|json!({"panic":if let Some(v)=error.downcast_ref::<String>(){v.clone()}else{error.downcast_ref::<&str>().unwrap().to_string()}}));
        println!("{out}");
    }
}
