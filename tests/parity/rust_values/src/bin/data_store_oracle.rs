//! Direct original DataStore and UserContext store entry points.
use std::{io::{self, BufRead}, panic::AssertUnwindSafe, sync::Arc, time::Duration};
use futures_util::FutureExt;
use serde_json::{json, Value as J};
use teaql_runtime::{DataStore, InMemoryDataStore, UserContext};
#[path = "../fixture_value.rs"] mod fixture_value;

async fn evaluate(input: J) -> J {
    let store: Arc<dyn DataStore> = Arc::new(InMemoryDataStore::default());
    let mut users = [UserContext::new(), UserContext::new()];
    let mut outputs = vec![];
    for step in input["steps"].as_array().unwrap() {
        let user = step["user"].as_u64().unwrap_or(0) as usize;
        let key = step["key"].as_str().unwrap_or("");
        let direct = step["direct"].as_bool().unwrap_or(false);
        let outcome = AssertUnwindSafe(async {
            match step["op"].as_str().unwrap() {
                "install" => { users[user].insert_resource(Box::new(Shared(store.clone())) as Box<dyn DataStore>); J::Null },
                "new" => { users[user] = UserContext::new(); J::Null },
                "put" => {
                    let value = fixture_value::value(&step["value"]);
                    let timeout = step["timeout"].as_u64();
                    if direct { store.put(key, value, timeout).await; }
                    else { users[user].put_in_store(key, value, timeout).await; }
                    J::Null
                },
                "get" => {
                    let value = if direct { store.get(key).await } else { users[user].get_in_store(key).await };
                    match value { Some(value) => json!({"some":true,"debug":format!("{value:?}"),"json":value.to_json_value()}), None => json!({"some":false}) }
                },
                "remove" => {
                    if direct { store.remove(key).await; } else { users[user].clear_in_store(key).await; }
                    J::Null
                },
                "sleep" => { std::thread::sleep(Duration::from_millis(2)); J::Null },
                _ => panic!("unknown action"),
            }
        }).catch_unwind().await;
        outputs.push(outcome.unwrap_or_else(|_| json!({"failure":true})));
    }
    json!(outputs)
}

// Source stores accept Box<dyn DataStore>; the fixture forwards without changing
// the original implementation or relying on a Context-owned concrete type.
#[async_trait::async_trait]
impl DataStore for Shared {
    async fn get(&self, key: &str) -> Option<teaql_core::Value> { self.0.get(key).await }
    async fn put(&self, key: &str, value: teaql_core::Value, timeout: Option<u64>) { self.0.put(key, value, timeout).await }
    async fn remove(&self, key: &str) { self.0.remove(key).await }
}
struct Shared(Arc<dyn DataStore>);

fn main() {
    std::panic::set_hook(Box::new(|_| {}));
    unsafe { std::env::set_var("TEAQL_LOG_ENDPOINT", "off"); }
    let runtime = tokio::runtime::Builder::new_current_thread().enable_all().build().unwrap();
    for line in io::stdin().lock().lines() {
        println!("{}", runtime.block_on(evaluate(serde_json::from_str(&line.unwrap()).unwrap())));
    }
}
