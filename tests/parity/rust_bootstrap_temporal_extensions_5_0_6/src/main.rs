use serde_json::{json, Value as Json};
use std::{future::Future, io::{self, BufRead}, sync::{Arc, Mutex}, task::{Context, Poll, Wake, Waker}};
use teaql_provider_sqlite::SqliteProviderExt;
use teaql_core::Entity;

struct TemporalChecker {
    mode: Arc<Mutex<String>>,
    hooks: Arc<Mutex<Vec<Json>>>,
}
impl domain::PriorityCheckerLogic for TemporalChecker {
    fn check_and_fix_priority(&self, _user_context: &teaql_runtime::UserContext,
        entity: &mut domain::Priority, status: teaql_runtime::CheckObjectStatus,
        location: &teaql_runtime::ObjectLocation, results: &mut teaql_runtime::CheckResults) {
        let before = json!({"instant":entity.instant().as_millis(),
            "created":entity.created_at().as_millis(),"updated":entity.updated_at().as_millis(),
            "clock":entity.clock()});
        let mode = self.mode.lock().unwrap().clone();
        if mode == "fix" {
            entity.update_created_at(teaql_core::time::Timestamp(777)).update_clock("custom-clock");
        } else if mode == "deny" {
            results.push(teaql_runtime::CheckResult::required(location.clone().member("clock")));
        }
        self.hooks.lock().unwrap().push(json!({"status":format!("{status:?}"),
            "before":before,"after_created":entity.created_at().as_millis(),"after_clock":entity.clock()}));
    }
}

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

fn create(pool: &Arc<Mutex<rusqlite::Connection>>, mode: &str, business_millis: i64,
    extension: Option<(Arc<Mutex<String>>, Arc<Mutex<Vec<Json>>>)>) -> teaql_runtime::UserContext {
    let mut module = match mode {
        "plain" => domain::module(),
        "checker" => domain::module_with_checkers(),
        "behavior" => domain::module_with_behaviors(),
        "combined" => domain::module_with_behaviors_and_checkers(),
        _ => panic!("unknown temporal module mode"),
    };
    if let Some((mode,hooks)) = extension {
        if *mode.lock().unwrap() != "none" {
            module = module.checker(teaql_runtime::TypedEntityChecker::<domain::Priority,_>::new(
                domain::PriorityChecker::new(TemporalChecker{mode,hooks})));
        }
    }
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
    let warmup = create(&pool, "combined", business_millis, None);
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
        let hooks = Arc::new(Mutex::new(Vec::new()));
        let requested = input["checker"].as_str().unwrap();
        let mode = Arc::new(Mutex::new(if requested == "late_deny" { "observe" } else { requested }.to_owned()));
        let mut user_context = create(&pool, input["module"].as_str().unwrap(), business_millis,
            Some((mode.clone(),hooks.clone())));
        let before_clock = user_context.business_time().timestamp_millis();
        let replacement = input["replacement_business_millis"].as_i64().unwrap();
        user_context.set_business_clock(teaql_runtime::FixedBusinessClock::new(
            chrono::DateTime::from_timestamp_millis(replacement).unwrap()));
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
        let bootstrap_after = database(&pool.lock().unwrap());
        let bootstrap_hooks = hooks.lock().unwrap().clone();
        let mut save_error = None;
        let mut save_diagnostics = Vec::new();
        if result.is_ok() {
            if requested == "late_deny" { *mode.lock().unwrap() = "deny".to_owned(); }
            let mut entity = wait(domain::Q::priorities().with_id_is(1)
                .comment("Read temporal extension target").purpose("Qualify clock and Checker overrides")
                .execute_for_one(&user_context)).unwrap().unwrap();
            entity.update_clock("business-change");
            match wait(teaql_runtime::save_audited_ledger_entity(
                entity.audit_as("Verify temporal extension on ordinary update"), &user_context)) {
                Ok(_) => {}
                Err(teaql_runtime::RuntimeError::Check(values)) => {
                    save_error = Some("Check".to_owned());
                    save_diagnostics = values.iter().map(|value|value.to_wire(
                        teaql_runtime::JsonFieldNamingProfile::SnakeCase)).collect();
                }
                Err(error) => { save_error = Some(error.to_string()); }
            }
        }
        rounds.push(json!({"before":before,"after":bootstrap_after,
            "error":result.err().map(|error|error.to_string()),
            "root":user_context.require_active_root("Platform").ok().map(|root|root.id),
            "business_before":before_clock,"business_millis":user_context.business_time().timestamp_millis(),
            "evidence":evidence,"logs":logs,"bootstrap_hooks":bootstrap_hooks,
            "hooks":hooks.lock().unwrap().clone(),"save_error":save_error,"save_diagnostics":save_diagnostics,
            "after_save":database(&pool.lock().unwrap())}));
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
