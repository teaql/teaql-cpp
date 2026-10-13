//! Actual frozen sample entry with test-controlled process wall clock.
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
const SCHEMA: &str = "CREATE TABLE platform_data(id INTEGER PRIMARY KEY,name TEXT,version INTEGER);CREATE TABLE sample_record_data(id INTEGER PRIMARY KEY,platform INTEGER,label TEXT,quantity INTEGER,zero_bound INTEGER,negative_max INTEGER,signed_max INTEGER,wide_max INTEGER,negative_bound TEXT,amount NUMERIC(19,7),height REAL,optional_count INTEGER,active BOOLEAN,disabled BOOLEAN,optional_active BOOLEAN,day DATE,optional_day DATE,recorded_at INTEGER,optional_recorded_at INTEGER,clock_text TEXT,optional_clock_text TEXT,ratio REAL,payload TEXT,version INTEGER);CREATE TABLE sample_link_data(id INTEGER PRIMARY KEY,first_record INTEGER,second_record INTEGER,name TEXT,version INTEGER);";
fn table(connection: &rusqlite::Connection, name: &str) -> Value {
    let mut statement = connection
        .prepare(&format!("SELECT * FROM {name} ORDER BY id"))
        .unwrap();
    let names = statement
        .column_names()
        .iter()
        .map(|name| name.to_string())
        .collect::<Vec<_>>();
    let rows = statement
        .query_map([], |row| {
            let mut values = serde_json::Map::new();
            for (index, name) in names.iter().enumerate() {
                let value = match row.get_ref(index).unwrap() {
                    rusqlite::types::ValueRef::Null => Value::Null,
                    rusqlite::types::ValueRef::Integer(value) => json!(value),
                    rusqlite::types::ValueRef::Real(value) => json!(value),
                    rusqlite::types::ValueRef::Text(value) => {
                        json!(std::str::from_utf8(value).unwrap())
                    }
                    _ => panic!("unexpected sample storage"),
                };
                values.insert(name.clone(), value);
            }
            Ok(Value::Object(values))
        })
        .unwrap();
    json!(rows.map(Result::unwrap).collect::<Vec<_>>())
}
fn observe(input: &Value) -> Value {
    macro_rules! run { ($domain:ident) => {{
    let wall = chrono::Utc::now().timestamp_millis();
    let pool = Arc::new(Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));
    let user_context = wait($domain::service_runtime_from_pool(
        pool.clone(),
    ))
    .unwrap()
    .with_active_root("Platform", 1);
    {
        let connection = pool.lock().unwrap();
        connection.execute_batch(&if input["kind"] == "temporal" { SCHEMA.replace(",payload TEXT", "") } else { SCHEMA.to_owned() }).unwrap();
        if input["root"] == true {
            connection
                .execute_batch("INSERT INTO platform_data VALUES(1,'existing root',1);")
                .unwrap();
        }
    }
    let mut rounds = Vec::new();
    for _ in 0..input["rounds"].as_u64().unwrap() {
        let scale = match input["scale"].as_str().unwrap() {
            "Tiny" => $domain::SampleDataScale::Tiny,
            "Small" => $domain::SampleDataScale::Small,
            _ => $domain::SampleDataScale::Medium,
        };
        let result = wait( $domain::generate_sample_data(
            &user_context,
             $domain::SampleDataPlan {
                scale,
                seed: input["seed"].as_u64().unwrap(),
            },
        ));
        let report = match result {
            Ok(report) => {
                json!({"generated":report.generated,"skipped":report.skipped.iter().map(|skip|json!({"entity":skip.entity,"reason":skip.reason})).collect::<Vec<_>>(),"error":null})
            }
            Err(error) => json!({"error":error.to_string()}),
        };
        let connection = pool.lock().unwrap();
        rounds.push(json!({"report":report,"records":table(&connection,"sample_record_data"),"links":table(&connection,"sample_link_data")}));
    }
    json!({"wall_millis":wall,"rounds":rounds})
    }}; }
    if input["kind"] == "temporal" {
        run!(cpp_sample_temporal_core)
    } else {
        run!(cpp_sample_runtime_core)
    }
}
fn main() {
    for line in io::stdin().lock().lines() {
        let input: Value = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", observe(&input));
    }
}
