use serde_json::{Value as Json, json};
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
        if let Poll::Ready(value) = future.as_mut().poll(&mut context) {
            return value;
        }
    }
}
fn database(connection: &rusqlite::Connection, tables: &[&str]) -> Json {
    let mut out = serde_json::Map::new();
    for table in tables {
        let mut statement = connection
            .prepare(&format!("SELECT * FROM {table} ORDER BY id"))
            .unwrap();
        let names = statement
            .column_names()
            .into_iter()
            .map(str::to_owned)
            .collect::<Vec<_>>();
        let rows = statement
            .query_map([], |row| {
                let mut values = serde_json::Map::new();
                for (index, name) in names.iter().enumerate() {
                    let value = match row.get_ref(index).unwrap() {
                        rusqlite::types::ValueRef::Null => Json::Null,
                        rusqlite::types::ValueRef::Integer(value) => json!(value),
                        rusqlite::types::ValueRef::Real(value) => json!(value),
                        rusqlite::types::ValueRef::Text(value) => {
                            json!(std::str::from_utf8(value).unwrap())
                        }
                        _ => unreachable!(),
                    };
                    values.insert(name.clone(), value);
                }
                Ok(values)
            })
            .unwrap();
        out.insert(
            (*table).into(),
            json!(rows.map(Result::unwrap).collect::<Vec<_>>()),
        );
    }
    let exists = connection
        .query_row(
            "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='teaql_id_space'",
            [],
            |row| row.get::<_, i64>(0),
        )
        .unwrap()
        != 0;
    let levels = if exists {
        let mut statement = connection
            .prepare("SELECT type_name,current_level FROM teaql_id_space ORDER BY type_name")
            .unwrap();
        statement
            .query_map([], |row| {
                Ok(json!([row.get::<_, String>(0)?, row.get::<_, i64>(1)?]))
            })
            .unwrap()
            .map(Result::unwrap)
            .collect::<Vec<_>>()
    } else {
        Vec::new()
    };
    out.insert("ids".into(), json!(levels));
    Json::Object(out)
}
fn observe(input: &Json) -> Json {
    let pool = Arc::new(Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));
    let scalar = input["fixture"] == "numeric";
    let create = || {
        if scalar {
            wait(cpp_constants_numeric_core::service_runtime_from_pool(
                pool.clone(),
            ))
            .unwrap()
        } else {
            wait(cpp_constants_catalog_core::service_runtime_from_pool(
                pool.clone(),
            ))
            .unwrap()
        }
    };
    // Real provider bootstrap creates the authentic schema. A fresh Context
    // then observes the controlled stored-state fixture on the same connection.
    let warmup = create();
    let warmup_result = wait(warmup.ensure_schema());
    if let Err(error) = warmup_result {
        return json!({"warmup_error":error.to_string()});
    }
    drop(warmup);
    let state = input["state"].as_str().unwrap();
    {
        let connection = pool.lock().unwrap();
        if scalar {
            match state {
                "empty"=>connection.execute_batch("DELETE FROM label_data;DELETE FROM priority_data;DELETE FROM platform_data;").unwrap(),
                "corrupt"=>connection.execute_batch("UPDATE priority_data SET rank=55,weight=8.5,ratio=0.25,cost='9.00',day='2001-01-01',note='stale',optional_rank=NULL WHERE id=1;").unwrap(),
                "missing"=>connection.execute_batch("DELETE FROM label_data WHERE id=32;DELETE FROM priority_data WHERE id=22;").unwrap(),
                "deleted"=>connection.execute_batch("UPDATE priority_data SET version=-1 WHERE id=1;").unwrap(),
                "extra"=>connection.execute_batch("INSERT INTO label_data(id,code,platform,priority,version) VALUES(99,'CUSTOM',1,1,7);").unwrap(),_=>{},
            }
        } else {
            match state {
                "empty"=>connection.execute_batch("DELETE FROM category_data;DELETE FROM platform_data;").unwrap(),
                "corrupt"=>connection.execute_batch("UPDATE category_data SET name='stale',code='OLD' WHERE id=1;").unwrap(),
                "missing"=>connection.execute_batch("DELETE FROM category_data WHERE id=22;").unwrap(),
                "deleted"=>connection.execute_batch("UPDATE category_data SET version=-1 WHERE id=22;").unwrap(),
                "extra"=>connection.execute_batch("INSERT INTO category_data(id,name,code,version) VALUES(99,'Custom','CUSTOM',7);").unwrap(),_=>{},
            }
        }
        connection
            .execute_batch("DROP TABLE teaql_id_space;")
            .unwrap();
    }
    let user_context = create();
    let tables = if scalar {
        vec!["platform_data", "priority_data", "label_data"]
    } else {
        vec!["platform_data", "category_data"]
    };
    let mut rounds = Vec::new();
    for _ in 0..input["rounds"].as_u64().unwrap() {
        let before = database(&pool.lock().unwrap(), &tables);
        let result = wait(user_context.ensure_schema());
        rounds.push(json!({"before":before,"after":database(&pool.lock().unwrap(),&tables),"error":result.err().map(|error|error.to_string()),
            "root":user_context.require_active_root("Platform").ok().map(|root|root.id)}));
    }
    json!({"rounds":rounds})
}
fn main() {
    for line in io::stdin().lock().lines() {
        let input: Json = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", observe(&input));
    }
}
