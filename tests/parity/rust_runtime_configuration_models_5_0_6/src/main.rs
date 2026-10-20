//! Observe original Business-ID runtime entries and actual governed persistence.
use cpp_business_ids_core::{ServiceRuntimeConfig, Q};
use serde_json::{json, Value as Json};
use std::error::Error;
use std::io::{self, BufRead};
use std::sync::{Arc, Mutex};
use teaql_runtime::{AuditedSaveExt, RuntimeError};
use teaql_core::Entity;

#[path = "../../rust_field_layout/src/bin/original_like_privacy_5_0_6.rs"]
mod driver;

fn runtime_error(error: &RuntimeError) -> Json {
    json!({"display": error.to_string(), "debug": format!("{error:?}")})
}

fn observe(input: &Json) -> Json {
    let connection = Arc::new(Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));
    let mut rounds = Vec::new();
    for round in 0..input["rounds"].as_u64().unwrap() {
        let result = match input["entry"].as_str().unwrap() {
            "pool" => driver::block_on(cpp_business_ids_core::service_runtime_from_pool(connection.clone())),
            "config" => driver::block_on(cpp_business_ids_core::service_runtime(ServiceRuntimeConfig {
                database_url: ":memory:".to_owned(),
            })),
            "env" => driver::block_on(cpp_business_ids_core::service_runtime_from_env()),
            _ => panic!("Unknown runtime entry"),
        };
        let mut user_context = match result {
            Ok(value) => value,
            Err(error) => {
                rounds.push(json!({"assembled": false, "display": error.to_string(),
                    "debug": format!("{error:?}"), "source": error.source().map(ToString::to_string)}));
                continue;
            }
        };
        user_context.disable_sql_log();
        user_context.set_business_clock(teaql_runtime::FixedBusinessClock::new(
            chrono::DateTime::from_timestamp_millis(1_700_000_000_000).unwrap(),
        ));
        let before = driver::block_on(Q::receipts_minimal()
            .comment("Inspect configured numbered runtime")
            .purpose("Verify explicit schema and connection reuse")
            .execute_for_count(&user_context));
        let before = match before { Ok(count) => json!({"count":count}), Err(error) => runtime_error(&error) };
        match driver::block_on(user_context.ensure_schema()) {
            Err(error) => rounds.push(json!({"assembled":true,"before_schema":before,
                "schema_error":runtime_error(&error)})),
            Ok(()) => {
                let mut receipt = Q::receipts()
                    .comment("Create configured numbered receipt")
                    .purpose("Verify generated runtime infrastructure")
                    .new_entity(&user_context);
                receipt.update_name(format!("receipt-{round}"));
                let saved = driver::block_on(AuditedSaveExt::save(
                    receipt.audit_as("Save configured numbered receipt"), &user_context));
                let saved = match saved {
                    Ok(value) => json!({"id":value.id(),"name":value.name(),"number":value.receipt_number().as_str(),
                        "tenant":value.tenant_id(),"version":value.version()}),
                    Err(error) => runtime_error(&error),
                };
                let rows = driver::block_on(Q::receipts().order_by_id_asc()
                    .comment("Read configured numbered receipts")
                    .purpose("Verify persistence across runtime scopes")
                    .execute_for_list(&user_context)).unwrap();
                let rows = rows.iter().map(|value| json!({"id":value.id(),"name":value.name(),
                    "number":value.receipt_number().as_str(),"tenant":value.tenant_id(),"version":value.version()})).collect::<Vec<_>>();
                rounds.push(json!({"assembled":true,"before_schema":before,"saved":saved,"rows":rows}));
            }
        }
        drop(user_context);
    }
    json!({"rounds":rounds})
}

fn main() {
    for line in io::stdin().lock().lines() {
        let input: Json = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", observe(&input));
    }
}
