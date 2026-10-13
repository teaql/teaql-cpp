//! Run the unchanged generated sample-data entry over real SQLite.
use cpp_relations_core::{SampleDataPlan, SampleDataScale};
use serde_json::{Value, json};
use std::{
    future::Future,
    io::{self, BufRead},
    sync::{Arc, Mutex},
    task::{Context, Poll, Wake, Waker},
};
struct NoWake;
impl Wake for NoWake {
    fn wake(self: Arc<Self>) {}
}
fn wait<T>(future: impl Future<Output = T>) -> T {
    let mut future = Box::pin(future);
    let waker = Waker::from(Arc::new(NoWake));
    let mut context = Context::from_waker(&waker);
    loop {
        if let Poll::Ready(result) = future.as_mut().poll(&mut context) {
            return result;
        }
    }
}
fn observe(input: &Value) -> Value {
    let pool = Arc::new(Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));
    let user_context = wait(cpp_relations_core::service_runtime_from_pool(pool.clone()))
        .unwrap()
        .with_active_root("Platform", 1);
    {
        let connection = pool.lock().unwrap();
        connection.execute_batch("CREATE TABLE platform_data(id INTEGER PRIMARY KEY,name TEXT,version INTEGER);CREATE TABLE device_data(id INTEGER PRIMARY KEY,name TEXT,platform INTEGER,version INTEGER);CREATE TABLE profile_data(id INTEGER PRIMARY KEY,name TEXT,platform INTEGER,version INTEGER);CREATE TABLE school_data(id INTEGER PRIMARY KEY,name TEXT,platform INTEGER,version INTEGER);").unwrap();
        if input["root"] == true {
            connection
                .execute_batch("INSERT INTO platform_data VALUES(1,'existing root',1);")
                .unwrap();
        }
        if let Some(model) = input.get("fault_model").and_then(Value::as_str) {
            assert!(["device", "profile", "school"].contains(&model));
            let fail_at = input["fail_at"].as_u64().unwrap();
            connection.execute_batch(&format!("CREATE TRIGGER sample_failure BEFORE INSERT ON {model}_data WHEN NEW.id={fail_at} BEGIN SELECT RAISE(ABORT,'sample fixture write failed'); END;")).unwrap();
        }
        if input["root_query_error"] == true {
            connection
                .execute_batch("DROP TABLE platform_data;")
                .unwrap();
        }
    }
    let mut rounds = Vec::new();
    for _ in 0..input["rounds"].as_u64().unwrap() {
        let scale = match input["scale"].as_str().unwrap() {
            "Tiny" => SampleDataScale::Tiny,
            "Small" => SampleDataScale::Small,
            _ => SampleDataScale::Medium,
        };
        let result = wait(cpp_relations_core::generate_sample_data(
            &user_context,
            SampleDataPlan {
                scale,
                seed: input["seed"].as_u64().unwrap(),
            },
        ));
        let report = match result {
            Ok(report) => {
                json!({"generated":report.generated,"skipped":report.skipped.iter().map(|s|json!({"entity":s.entity,"reason":s.reason})).collect::<Vec<_>>(),"error":null})
            }
            Err(error) => json!({"error":error.to_string()}),
        };
        let connection = pool.lock().unwrap();
        let mut tables = serde_json::Map::new();
        for table in ["device_data", "profile_data", "school_data"] {
            let mut statement = connection
                .prepare(&format!(
                    "SELECT id,name,platform,version FROM {table} ORDER BY id"
                ))
                .unwrap();
            let rows = statement
                .query_map([], |row| {
                    Ok(json!([
                        row.get::<_, i64>(0)?,
                        row.get::<_, String>(1)?,
                        row.get::<_, Option<i64>>(2)?,
                        row.get::<_, i64>(3)?
                    ]))
                })
                .unwrap();
            tables.insert(
                table.into(),
                json!(rows.map(Result::unwrap).collect::<Vec<_>>()),
            );
        }
        rounds.push(json!({"report":report,"tables":tables}));
    }
    json!(rounds)
}
fn source_failures() -> Value {
    let relations_pool = Arc::new(Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));
    let relations = wait(cpp_relations_core::service_runtime_from_pool(
        relations_pool,
    ))
    .unwrap();
    let schema = wait(relations.ensure_schema())
        .err()
        .map(|error| error.to_string());
    let inherited_pool = Arc::new(Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));
    let inherited = wait(cpp_inheritance_core::service_runtime_from_pool(
        inherited_pool.clone(),
    ))
    .unwrap();
    wait(inherited.ensure_schema()).unwrap();
    let result = wait(cpp_inheritance_core::generate_sample_data(
        &inherited,
        cpp_inheritance_core::SampleDataPlan {
            scale: cpp_inheritance_core::SampleDataScale::Tiny,
            seed: 7,
        },
    ));
    let error = result.err().map(|error| error.to_string());
    let mut counts = serde_json::Map::new();
    for table in ["school_data", "academy_data", "research_academy_data"] {
        let count = inherited_pool
            .lock()
            .unwrap()
            .query_row(&format!("SELECT count(*) FROM {table}"), [], |row| {
                row.get::<_, i64>(0)
            })
            .unwrap();
        counts.insert(table.into(), json!(count));
    }
    json!({"relations_automatic_schema_error":schema,"inherited_sample_error":error,"inherited_persisted_rows":counts})
}
fn main() {
    if std::env::args().any(|arg| arg == "--source-failures") {
        println!("{}", source_failures());
        return;
    }
    for line in io::stdin().lock().lines() {
        let input: Value = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", observe(&input));
    }
}
