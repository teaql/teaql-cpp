//! Execute original sample generation with governed Business-ID allocation.
use cpp_business_ids_core as domain;
use serde_json::{json, Value as Json};
use std::io::{self, BufRead};
use std::sync::{Arc, Mutex};
use teaql_provider_sqlite::SqliteProviderExt;

#[path = "../../rust_field_layout/src/bin/original_like_privacy_5_0_6.rs"]
mod driver;

fn table(connection: &rusqlite::Connection, name: &str) -> Json {
    let mut query = connection.prepare(&format!("SELECT * FROM {name} ORDER BY 1")).unwrap();
    let names = query.column_names().into_iter().map(str::to_owned).collect::<Vec<_>>();
    let rows = query.query_map([], |row| {
        let mut output = serde_json::Map::new();
        for (index, name) in names.iter().enumerate() {
            let value = match row.get_ref(index).unwrap() {
                rusqlite::types::ValueRef::Null => Json::Null,
                rusqlite::types::ValueRef::Integer(value) => json!(value),
                rusqlite::types::ValueRef::Real(value) => json!(value),
                rusqlite::types::ValueRef::Text(value) => json!(String::from_utf8(value.to_vec()).unwrap()),
                rusqlite::types::ValueRef::Blob(_) => panic!("Unexpected sample blob"),
            };
            output.insert(name.clone(), value);
        }
        Ok(Json::Object(output))
    }).unwrap();
    json!(rows.map(Result::unwrap).collect::<Vec<_>>())
}

fn snapshot(connection: &rusqlite::Connection) -> Json {
    json!({"tenant":table(connection,"tenant_data"),
           "receipt":table(connection,"receipt_data"),
           "ticket":table(connection,"ticket_data")})
}

fn observe(input: &Json) -> Json {
    let pool = Arc::new(Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));
    let module = match input["module"].as_str().unwrap() {
        "plain" => domain::module(),
        "checker" => domain::module_with_checkers(),
        "behavior" => domain::module_with_behaviors(),
        "combined" => domain::module_with_behaviors_and_checkers(),
        _ => panic!("Unknown module"),
    };
    let mut user_context = module.into_context();
    user_context.disable_sql_log();
    let transport = domain::DataServiceMutationExecutor::new(pool.clone());
    user_context.set_internal_id_generator(domain::DataServiceIdGenerator::from_executor(transport.clone()));
    user_context.use_sqlite_provider(transport.clone());
    user_context.set_business_id_infrastructure(domain::DataServiceBusinessIdAllocator::from_executor(transport.clone()));
    if input["key"] == true {
        let bytes = std::array::from_fn(|index| index as u8);
        let key = teaql_runtime::BusinessIdEncodingKey::new(1, bytes).unwrap();
        user_context.set_business_id_key_provider(teaql_runtime::StaticBusinessIdKeyProvider::new(key)).unwrap();
    }
    user_context.set_business_clock(teaql_runtime::FixedBusinessClock::new(
        chrono::DateTime::from_timestamp_millis(1_700_000_000_000).unwrap()));
    let executor = domain::ServiceRuntimeExecutor::new(transport);
    user_context.register_executor(executor.clone());
    user_context.insert_resource(executor);
    if let Err(error) = driver::block_on(user_context.ensure_schema()) {
        return json!({"schema_error":error.to_string()});
    }
    {
        let connection = pool.lock().unwrap();
        if input["root"] == "missing" {
            connection.execute_batch("DELETE FROM tenant_data;").unwrap();
        }
        if let Some(target) = input.get("fault").and_then(Json::as_str) {
            assert!(["receipt", "ticket"].contains(&target));
            connection.execute_batch(&format!("CREATE TRIGGER sample_failure BEFORE INSERT ON {target}_data WHEN NEW.id=3 BEGIN SELECT RAISE(ABORT,'sample Business-ID fixture failed'); END;")).unwrap();
        }
    }
    user_context.enable_all_sql_log();
    let mut rounds = Vec::new();
    for _ in 0..input["rounds"].as_u64().unwrap() {
        user_context.clear_sql_logs();
        let before = snapshot(&pool.lock().unwrap());
        let scale = if input["scale"] == "Small" { domain::SampleDataScale::Small } else { domain::SampleDataScale::Tiny };
        let result = driver::block_on(domain::generate_sample_data(&user_context, domain::SampleDataPlan {
            scale, seed:input["seed"].as_u64().unwrap(),
        }));
        let report = match result {
            Ok(value) => json!({"generated":value.generated,
                "skipped":value.skipped.iter().map(|value|json!({"entity":value.entity,"reason":value.reason})).collect::<Vec<_>>(),"error":null}),
            Err(error) => json!({"error":error.to_string()}),
        };
        let logs = user_context.sql_logs().iter().map(|entry|json!({
            "context":format!("{:?}",entry.log_context),"operation":format!("{:?}",entry.operation),
            "sql":entry.sql,"params":entry.params.iter().map(|value|format!("{value:?}")).collect::<Vec<_>>(),
            "comment":entry.comment,"purpose":entry.purpose,"audit_reason":entry.audit_reason,
            "trace":format!("{:?}",entry.trace_path),"debug_sql":entry.debug_sql,"pretty_sql":entry.pretty_sql,
            "result_count":entry.result_count,"result_type":entry.result_type,"affected_rows":entry.affected_rows,"summary":entry.result_summary,
        })).collect::<Vec<_>>();
        rounds.push(json!({"before":before,"report":report,"after":snapshot(&pool.lock().unwrap()),"logs":logs}));
    }
    json!({"schema_error":null,"rounds":rounds})
}

fn main() {
    for line in io::stdin().lock().lines() {
        let input: Json = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", observe(&input));
    }
}
