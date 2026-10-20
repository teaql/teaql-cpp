//! Execute unchanged candidate/constant sample entries with application policy.
use serde_json::{json, Value as Json};
use std::io::{self, BufRead};
use std::sync::{Arc, Mutex};
use teaql_provider_sqlite::SqliteProviderExt;

#[path = "../../rust_field_layout/src/bin/original_like_privacy_5_0_6.rs"]
mod driver;

fn table(connection: &rusqlite::Connection, name: &str) -> Json {
    let exists: i64 = connection.query_row("SELECT count(*) FROM sqlite_master WHERE type='table' AND name=?1", [name], |row| row.get(0)).unwrap();
    if exists == 0 { return Json::Null; }
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

fn snapshot(connection: &rusqlite::Connection, kind: &str) -> Json {
    let names = if kind == "candidates" { vec!["platform", "order_status", "person", "record"] } else { vec!["platform", "category"] };
    let mut output = serde_json::Map::new();
    for name in names { output.insert(name.into(), table(connection, &format!("{name}_data"))); }
    Json::Object(output)
}
struct SamplePolicy { plans: Arc<Mutex<Vec<String>>>, target: String }
impl teaql_runtime::MutationPolicy for SamplePolicy {
    fn identity(&self) -> teaql_runtime::MutationPolicyIdentity { teaql_runtime::MutationPolicyIdentity::new("sample-policy","1","fingerprint") }
    fn review(&self, _: &teaql_runtime::UserContext, plan: &teaql_runtime::MutationPlan) -> teaql_runtime::MutationDecision {
        self.plans.lock().unwrap().push(format!("{plan:?}"));
        if plan.root_entity_type == self.target { teaql_runtime::MutationDecision::deny("SAMPLE-DENIED", "fixture sample policy denial", std::iter::empty::<String>()) }
        else { teaql_runtime::MutationDecision::allow() }
    }
}
macro_rules! observer { ($name:ident, $domain:ident) => {
fn $name(input: &Json) -> Json {
    use $domain as domain;
    let kind = input["fixture"].as_str().unwrap();
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
        connection.execute_batch("PRAGMA foreign_keys=OFF;").unwrap();
        let state = input["state"].as_str().unwrap();
        let constant = if kind == "candidates" { "order_status_data" } else { "category_data" };
        match state {
            "missing_root" => connection.execute_batch("DELETE FROM platform_data;").unwrap(),
            "missing_constant" => connection.execute_batch(&format!("DELETE FROM {constant};")).unwrap(),
            "deleted_constant" => connection.execute_batch(&format!("UPDATE {constant} SET version=-1;")).unwrap(),
            "failed_constant_query" => connection.execute_batch(&format!("DROP TABLE {constant};")).unwrap(),
            "normal" => (),
            _ => panic!("Invalid sample state"),
        }
        if let Some(target) = input.get("fault").and_then(Json::as_str) {
            assert!(kind == "candidates" && ["person", "record"].contains(&target));
            connection.execute_batch(&format!("CREATE TRIGGER sample_failure BEFORE INSERT ON {target}_data WHEN NEW.id=3 BEGIN SELECT RAISE(ABORT,'sample catalog fixture failed'); END;")).unwrap();
        }
    }
    pool.lock().unwrap().execute_batch("PRAGMA foreign_keys=ON;").unwrap();
    user_context.set_trace_id("sample");
    user_context.set_user_identifier("fixture");
    let plans = Arc::new(Mutex::new(Vec::new()));
    let governance = input["governance"].as_str().unwrap();
    if governance != "default" {
        let target = match governance { "deny_person" => "Person", "deny_record" => "Record", _ => "" };
        let policy: Arc<dyn teaql_runtime::MutationPolicy> = Arc::new(SamplePolicy { plans: plans.clone(), target: target.into() });
        user_context.set_mutation_policy_registry(move |_: &str| Some(policy.clone()));
        if governance == "approved" { user_context.set_mutation_policy_approval_provider(|identity: &teaql_runtime::MutationPolicyIdentity| Some(teaql_runtime::MutationPolicyApproval::new(identity.clone(), "reviewer", std::time::UNIX_EPOCH))); }
    }
    user_context.enable_all_sql_log();
    let mut rounds = Vec::new();
    for _ in 0..input["rounds"].as_u64().unwrap() {
        user_context.clear_sql_logs();
        plans.lock().unwrap().clear();
        let before = snapshot(&pool.lock().unwrap(), kind);
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
        rounds.push(json!({"before":before,"report":report,"after":snapshot(&pool.lock().unwrap(), kind),"logs":logs,"plans":plans.lock().unwrap().clone()}));
    }
    json!({"schema_error":null,"rounds":rounds})
}

};
}
observer!(observe_candidates, cpp_candidates_core);
observer!(observe_catalog, cpp_constants_catalog_core);

fn main() {
    for line in io::stdin().lock().lines() {
        let input: Json = serde_json::from_str(&line.unwrap()).unwrap();
        let result = match input["fixture"].as_str().unwrap() { "candidates" => observe_candidates(&input), "catalog" => observe_catalog(&input), _ => panic!("Invalid sample fixture") };
        println!("{result}");
    }
}
