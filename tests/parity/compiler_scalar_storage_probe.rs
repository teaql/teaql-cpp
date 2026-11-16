use domain::{E, Q, Reading, ServiceRuntimeExecutor};
use serde_json::{Value as Json, json};
use std::{
    future::Future,
    io::{self, BufRead},
    pin::pin,
    sync::{Arc, Mutex},
    task::{Context, Poll, Wake, Waker},
};
use teaql_core::{Entity, SelectQuery};
use teaql_runtime::PurposedSelectQuery;

struct Notify;
impl Wake for Notify {
    fn wake(self: Arc<Self>) {}
}
fn wait<T>(future: impl Future<Output = T>) -> T {
    let mut future = pin!(future);
    let waker = Waker::from(Arc::new(Notify));
    let mut context = Context::from_waker(&waker);
    loop {
        if let Poll::Ready(value) = future.as_mut().poll(&mut context) {
            return value;
        }
    }
}

fn observe(input: &Json) -> Json {
    let pool = Arc::new(Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));
    pool.lock().unwrap().execute_batch(input["sql"].as_str().unwrap()).unwrap();
    let user_context = wait(domain::service_runtime_from_pool(pool)).unwrap()
        .with_active_root("Gateway", 1);
    let query: SelectQuery = Q::readings().into();
    let query = PurposedSelectQuery::new(
        query.order_asc("id").comment("Read compiler-resolved scalar storage"),
        "Verify typed values through SQLite and expressions",
    );
    let service = user_context.entity_data_service::<ServiceRuntimeExecutor>("Reading").unwrap();
    let result = wait(service.fetch_enhanced_entities::<Reading>(&query)).unwrap();
    let rows: Vec<_> = result.data.iter().map(|reading| json!({
        "getter": reading.value(),
        "expression": E::reading(reading).get_value().eval(),
        "json": reading.clone().into_json(),
    })).collect();
    let sql: Vec<_> = user_context.sql_logs().iter().map(|entry| json!({
        "sql": entry.sql, "comment": entry.comment, "purpose": entry.purpose,
        "count": entry.result_count,
    })).collect();
    json!({"rows": rows, "sql": sql})
}

fn main() {
    for line in io::stdin().lock().lines() {
        println!("{}", observe(&serde_json::from_str::<Json>(&line.unwrap()).unwrap()));
    }
}
