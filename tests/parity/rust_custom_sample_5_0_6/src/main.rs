//! Execute original sample entry with application-owned Checker, Behavior and Policy.
use serde_json::{json, Value as Json};
use std::io::{self, BufRead};
use std::sync::{Arc, Mutex};
use teaql_provider_sqlite::SqliteProviderExt;
use cpp_candidates_core as domain;
use teaql_core::Entity as _;
use teaql_runtime::{CheckObjectStatus, CheckResults, ObjectLocation, UserContext};

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

struct Collector { enabled:std::sync::atomic::AtomicBool, records:Mutex<Vec<Json>> }
static LOGGER:Collector=Collector{enabled:std::sync::atomic::AtomicBool::new(true),records:Mutex::new(Vec::new())};
impl log::Log for Collector {
    fn enabled(&self,m:&log::Metadata)->bool { self.enabled.load(std::sync::atomic::Ordering::Relaxed)&&m.level()==log::Level::Info&&m.target().ends_with("::sample_data") }
    fn log(&self,r:&log::Record) { if self.enabled(r.metadata()) { self.records.lock().unwrap().push(json!({"level":r.level().to_string(),"target":r.target(),"message":r.args().to_string()})); } }
    fn flush(&self) {}
}
#[derive(Clone)]
struct CustomState { calls:Arc<Mutex<Vec<Json>>>, checker:String, behavior:String }
struct PersonRules(CustomState);
impl domain::PersonCheckerLogic for PersonRules {
    fn check_and_fix_person(&self,_:&UserContext,e:&mut domain::Person,status:CheckObjectStatus,location:&ObjectLocation,results:&mut CheckResults) {
        self.0.calls.lock().unwrap().push(json!({"hook":"checker","entity":"Person","id":e.id(),"name":e.name(),"status":format!("{status:?}"),"before":results.len(),"location":location.model_path()}));
        if self.0.checker=="fix_person" { e.update_name(format!("fixed {}",e.id()));e.update_phase("CUSTOM"); }
        if self.0.checker=="reject_person"&&e.id()==3 { self.required(false,"custom_field",location,results); }
    }
}
struct RecordRules(CustomState);
impl domain::RecordCheckerLogic for RecordRules {
    fn check_and_fix_record(&self,_:&UserContext,e:&mut domain::Record,status:CheckObjectStatus,location:&ObjectLocation,results:&mut CheckResults) {
        self.0.calls.lock().unwrap().push(json!({"hook":"checker","entity":"Record","id":e.id(),"name":e.name(),"status":format!("{status:?}"),"before":results.len(),"location":location.model_path()}));
        if self.0.checker=="reject_record"&&e.id()==3 { self.required(false,"custom_field",location,results); }
    }
}
struct Behavior(CustomState);
impl teaql_runtime::EntityDataServiceBehavior for Behavior {
    fn before_select(&self,_:&UserContext,q:&mut teaql_core::SelectQuery)->Result<(),teaql_runtime::RuntimeError> {
        self.0.calls.lock().unwrap().push(json!({"hook":"select","entity":q.entity}));
        if q.entity=="OrderStatus" {
            if self.0.behavior=="reverse_constants" { q.order_by.push(teaql_core::OrderBy::desc("id")); }
            if self.0.behavior=="deny_query" { return Err(teaql_runtime::RuntimeError::Behavior("fixture sample query denied".into())); }
        }
        Ok(())
    }
    fn before_insert(&self,_:&UserContext,c:&mut teaql_core::InsertCommand)->Result<(),teaql_runtime::RuntimeError> {
        let values=c.values.iter().map(|(name,value)|(name.clone(),json!(format!("{value:?}")))).collect::<serde_json::Map<_,_>>();
        self.0.calls.lock().unwrap().push(json!({"hook":"insert","entity":c.entity,"values":values}));
        let id=c.values.get("id").and_then(|v|v.try_u64()).unwrap_or_default();
        if id==3 && ((self.0.behavior=="deny_insert_person"&&c.entity=="Person")||(self.0.behavior=="deny_insert_record"&&c.entity=="Record")) {
            return Err(teaql_runtime::RuntimeError::Behavior("fixture sample insert denied".into()));
        }
        Ok(())
    }
}

fn observe(input: &Json) -> Json {
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
    let custom=CustomState{calls:Arc::new(Mutex::new(Vec::new())),checker:input["checker"].as_str().unwrap().into(),behavior:input["behavior"].as_str().unwrap().into()};
    let mut application_module=match input["module"].as_str().unwrap() { "plain"=>domain::module(),"checker"=>domain::module_with_checkers(),"behavior"=>domain::module_with_behaviors(),_=>domain::module_with_behaviors_and_checkers() };
    if custom.checker!="none" {
        application_module=application_module.checker(teaql_runtime::TypedEntityChecker::<domain::Person,_>::new(domain::PersonChecker::new(PersonRules(custom.clone()))));
        application_module=application_module.checker(teaql_runtime::TypedEntityChecker::<domain::Record,_>::new(domain::RecordChecker::new(RecordRules(custom.clone()))));
    }
    if custom.behavior!="none" { for name in ["Platform","OrderStatus","Person","Record"] { application_module=application_module.behavior(name,Behavior(custom.clone())); } }
    application_module.apply_to(&mut user_context);
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
        custom.calls.lock().unwrap().clear();
        LOGGER.records.lock().unwrap().clear();
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
        let source_records=LOGGER.records.lock().unwrap().clone();
        let messages=source_records.iter().map(|r|r["message"].clone()).collect::<Vec<_>>();
        rounds.push(json!({"calls":custom.calls.lock().unwrap().clone(),"messages":messages,"source_records":source_records,"before":before,"report":report,"after":snapshot(&pool.lock().unwrap(), kind),"logs":logs,"plans":plans.lock().unwrap().clone()}));
    }
    json!({"schema_error":null,"rounds":rounds})
}


fn main() {
    log::set_logger(&LOGGER).unwrap(); log::set_max_level(log::LevelFilter::Info);
    for line in io::stdin().lock().lines() {
        let input: Json = serde_json::from_str(&line.unwrap()).unwrap();
        LOGGER.enabled.store(input["enabled"].as_bool().unwrap(), std::sync::atomic::Ordering::Relaxed);
        println!("{}", observe(&input));
    }
}
