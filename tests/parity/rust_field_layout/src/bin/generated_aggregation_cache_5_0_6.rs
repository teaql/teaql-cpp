//! Observe cache SPI events around actual frozen generated request execution.
use cpp_dynamic_json_core::Q;
use serde_json::{Value as Json, json};
use std::io::{self, BufRead};
use std::sync::{Arc, Mutex};
use teaql_core::{CompactRow, Expr};
use teaql_runtime::{AggregationCacheBackend, InMemoryAggregationCache};
#[path = "original_like_privacy_5_0_6.rs"]
mod driver;
struct RecordingCache {
    inner: InMemoryAggregationCache,
    events: Mutex<Vec<Json>>,
}
impl AggregationCacheBackend for RecordingCache {
    fn namespace(&self) -> &str {
        self.inner.namespace()
    }
    fn get(&self, key: &str, age: u64) -> Option<Vec<CompactRow>> {
        let rows = self.inner.get(key, age);
        self.events
            .lock()
            .unwrap()
            .push(json!({"op":"get","key":key,"age":age,"hit":rows.is_some()}));
        rows
    }
    fn put(&self, key: String, rows: Vec<CompactRow>) {
        self.events
            .lock()
            .unwrap()
            .push(json!({"op":"put","key":key,"rows":rows.len()}));
        self.inner.put(key, rows);
    }
    fn invalidate_namespace(&self, namespace: &str) {
        self.events
            .lock()
            .unwrap()
            .push(json!({"op":"invalidate","namespace":namespace}));
        self.inner.invalidate_namespace(namespace);
    }
}
fn flag(input: &Json, key: &str) -> bool {
    input[key] == true
}
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
    let mut user_context = driver::block_on(cpp_dynamic_json_core::service_runtime_from_pool(
        pool.clone(),
    ))
    .unwrap()
    .with_active_root("Platform", 1);
    let cache = Arc::new(RecordingCache {
        inner: InMemoryAggregationCache::with_namespace("fixture"),
        events: Mutex::new(Vec::new()),
    });
    let backend: Arc<dyn AggregationCacheBackend> = cache.clone();
    if !flag(input, "no_backend") {
        user_context.insert_resource(backend);
    }
    let mut rounds = Vec::new();
    for step in input["steps"].as_array().unwrap() {
        if flag(step, "clear") {
            cache.inner.clear();
        }
        if let Some(delay) = step["delay"].as_u64() {
            std::thread::sleep(std::time::Duration::from_millis(delay));
        }
        if flag(step, "update") {
            pool.lock()
                .unwrap()
                .execute_batch("UPDATE order_data SET amount=amount+1 WHERE id=10")
                .unwrap();
        }
        if flag(step, "save") {
            use teaql_core::Entity;
            let mut entity = driver::block_on(
                Q::orders()
                    .with_id_is(10_u64)
                    .comment("load cache mutation")
                    .purpose("invalidate aggregation cache")
                    .execute_for_one(&user_context),
            )
            .unwrap()
            .unwrap();
            entity.update_amount(teaql_core::Decimal::from(99_i64));
            driver::block_on(teaql_runtime::save_audited_ledger_entity(
                entity.audit_as("update cached aggregate member"),
                &user_context,
            ))
            .unwrap();
        }
        let mut request = Q::people_minimal();
        let mut child = Q::orders().unlimited();
        if let Some(age) = step["child_age"].as_u64() {
            child = child.enable_aggregation_cache_for(age);
        }
        if flag(step, "filter") {
            child = child.filter(Expr::gt("amount", 9_i64));
        }
        request = if input["kind"] == "sum" {
            request.sum_amount_of_orders_as("_metric", child)
        } else {
            request.count_orders_with("_metric", child)
        };
        if let Some(age) = step["age"].as_u64() {
            request = request.enable_aggregation_cache_for(age);
        }
        if let Some(age) = step["propagate"].as_u64() {
            request = request.propagate_aggregation_cache(age);
        }
        if flag(step, "empty") {
            request = request.filter(Expr::lt("id", 0_i64));
        }
        request = request.order_by_id_asc();
        let comment = step["comment"].as_str().unwrap_or("inspect cache");
        let purpose = step["purpose"].as_str().unwrap_or("qualify cache");
        let result = driver::block_on(
            request
                .comment(comment)
                .purpose(purpose)
                .execute_for_list(&user_context),
        );
        let mut out = json!({"values":[],"error":null});
        match result {
            Ok(rows) => {
                out["values"] = json!(
                    rows.into_iter()
                        .map(|entity| {
                            use teaql_core::Entity;
                            let id = entity.id();
                            let values = entity.into_json_values();
                            json!([id, values.get("_metric").map(|v| format!("{v:?}"))])
                        })
                        .collect::<Vec<_>>()
                )
            }
            Err(error) => out["error"] = json!(error.to_string()),
        }
        out["events"] = json!(std::mem::take(&mut *cache.events.lock().unwrap()));
        out["sql"]=json!(user_context.sql_logs().into_iter().map(|entry|json!({"sql":entry.sql,"params":entry.params.iter().map(|v|format!("{v:?}")).collect::<Vec<_>>(),"comment":entry.comment,"purpose":entry.purpose})).collect::<Vec<_>>());
        rounds.push(out);
    }
    json!(rounds)
}
fn main() {
    for line in io::stdin().lock().lines() {
        let input: Json = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", observe(&input));
    }
}
