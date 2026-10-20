use serde_json::{json, Value as Json};
use std::{future::Future, io::{self, BufRead}, sync::{Arc, Mutex}, task::{Context, Poll, Wake, Waker}};
use teaql_provider_sqlite::SqliteProviderExt;

fn wait<T>(future: impl Future<Output = T>) -> T {
    struct NoWake;
    impl Wake for NoWake { fn wake(self: Arc<Self>) {} }
    let mut future = Box::pin(future);
    let waker = Waker::from(Arc::new(NoWake));
    let mut context = Context::from_waker(&waker);
    loop {
        if let Poll::Ready(value) = future.as_mut().poll(&mut context) {
            return value;
        }
    }
}

fn database(connection: &rusqlite::Connection) -> Json {
    let mut result = serde_json::Map::new();
    for table in ["platform_data", "priority_data", "label_data", "teaql_id_space"] {
        let order = if table == "teaql_id_space" { "type_name" } else { "id" };
        let mut statement = connection.prepare(&format!("SELECT * FROM {table} ORDER BY {order}")).unwrap();
        let names = statement.column_names().into_iter().map(str::to_owned).collect::<Vec<_>>();
        let rows = statement.query_map([], |row| {
            let mut object = serde_json::Map::new();
            for (index, name) in names.iter().enumerate() {
                let value = match row.get_ref(index).unwrap() {
                    rusqlite::types::ValueRef::Null => Json::Null,
                    rusqlite::types::ValueRef::Integer(value) => json!(value),
                    rusqlite::types::ValueRef::Real(value) => json!(value),
                    rusqlite::types::ValueRef::Text(value) => json!(std::str::from_utf8(value).unwrap()),
                    _ => panic!("unexpected temporal fixture SQLite value"),
                };
                object.insert(name.clone(), value);
            }
            Ok(Json::Object(object))
        }).unwrap().map(Result::unwrap).collect::<Vec<_>>();
        result.insert(table.into(), json!(rows));
    }
    Json::Object(result)
}

fn create(pool: &Arc<Mutex<rusqlite::Connection>>, mode: &str, business_millis: i64) -> teaql_runtime::UserContext {
    let module = match mode {
        "plain" => domain::module(),
        "checker" => domain::module_with_checkers(),
        "behavior" => domain::module_with_behaviors(),
        "combined" => domain::module_with_behaviors_and_checkers(),
        _ => panic!("unknown temporal module mode"),
    };
    let transport = domain::DataServiceMutationExecutor::new(pool.clone());
    let mut user_context = module.into_context();
    user_context.set_internal_id_generator(domain::DataServiceIdGenerator::from_executor(transport.clone()));
    user_context.use_sqlite_provider(transport.clone());
    let executor = domain::ServiceRuntimeExecutor::new(transport);
    user_context.register_executor(executor.clone());
    user_context.insert_resource(executor);
    user_context.set_business_clock(teaql_runtime::FixedBusinessClock::new(
        chrono::DateTime::from_timestamp_millis(business_millis).unwrap()));
    user_context.disable_sql_log();
    user_context
}

fn observe(input: &Json) -> Json {
    let pool = Arc::new(Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));
    let business_millis = input["business_millis"].as_i64().unwrap();
    let warmup = create(&pool, "combined", business_millis);
    let result = wait(warmup.ensure_schema());
    if let Err(error) = result {
        return json!({"wall_millis":teaql_core::time::Timestamp::now().as_millis(), "warmup_error":error.to_string()});
    }
    drop(warmup);
    let sql = match input["state"].as_str().unwrap() {
        "empty" => "DELETE FROM label_data;DELETE FROM priority_data;DELETE FROM platform_data;",
        "existing" => "",
        "corrupt" => "UPDATE priority_data SET name='stale',instant=0,clock='stale',optional_clock=NULL,created_at=0,updated_at=0 WHERE id=1;",
        "missing" => "DELETE FROM label_data WHERE id=32;DELETE FROM priority_data WHERE id=22;",
        "deleted" => "UPDATE priority_data SET version=-1 WHERE id=1;",
        "extra" => "INSERT INTO label_data(id,code,platform,priority,version) VALUES(99,'CUSTOM',1,1,7);",
        _ => panic!("unknown temporal fixture state"),
    };
    pool.lock().unwrap().execute_batch(sql).unwrap();
    let mut rounds = Vec::new();
    for index in 0..input["rounds"].as_u64().unwrap() {
        let wall = input["wall_millis"].as_i64().unwrap() + (index as i64 + 1) * input["tick_per_round"].as_i64().unwrap();
        // Single-threaded observer using the pre-existing test-only process clock.
        unsafe { std::env::set_var("TEAQL_TEST_SAMPLE_WALL_MILLIS", wall.to_string()); }
        let mut user_context = create(&pool, input["module"].as_str().unwrap(), business_millis);
        if input["logging"] == true { user_context.enable_all_sql_log(); }
        let before = database(&pool.lock().unwrap());
        let result = wait(user_context.ensure_schema());
        let evidence = user_context.last_fix_evidence().iter().map(|item| json!({
            "entity":item.entity_type,"path":item.model_path,"source":format!("{:?}",item.source),"label":item.source_label
        })).collect::<Vec<_>>();
        let logs = user_context.sql_logs().iter().map(|entry| json!({
            "sql":entry.sql,"params":entry.params.iter().map(|value|format!("{value:?}")).collect::<Vec<_>>(),
            "comment":entry.comment,"purpose":entry.purpose,"audit_as":entry.audit_reason
        })).collect::<Vec<_>>();
        rounds.push(json!({"before":before,"after":database(&pool.lock().unwrap()),
            "error":result.err().map(|error|error.to_string()),
            "root":user_context.require_active_root("Platform").ok().map(|root|root.id),
            "business_millis":user_context.business_time().timestamp_millis(),"evidence":evidence,"logs":logs}));
    }
    unsafe { std::env::set_var("TEAQL_TEST_SAMPLE_WALL_MILLIS", input["wall_millis"].as_i64().unwrap().to_string()); }
    json!({"wall_millis":teaql_core::time::Timestamp::now().as_millis(),"rounds":rounds})
}

fn main() {
    for line in io::stdin().lock().lines() {
        let input: Json = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", observe(&input));
    }
}
