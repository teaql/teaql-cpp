use cpp_relation_identities_core::Q;
use serde_json::{json, Value as Json};
use std::{io::{self, BufRead}, sync::{Arc, Mutex}};
use teaql_core::{Entity, TeaqlEntity};
use teaql_runtime::{LedgerEntity, RuntimeError, UserContext, SafeAuditEvent, SafeAuditEventSink,
                    MutationPolicy, MutationPolicyIdentity, MutationPlan, MutationDecision};
#[path = "../../../rust_field_layout/src/bin/original_like_privacy_5_0_6.rs"]
mod driver;

struct Sink { events: Arc<Mutex<Vec<String>>>, fail: bool }
impl SafeAuditEventSink for Sink {
    fn on_safe_event(&self, _: &UserContext, event: &SafeAuditEvent) -> Result<(), RuntimeError> {
        self.events.lock().unwrap().push(format!("{event:?}"));
        if self.fail { Err(RuntimeError::Event("keyword audit failure".into())) } else { Ok(()) }
    }
}
struct Policy { plans: Arc<Mutex<Vec<String>>>, deny: bool }
impl MutationPolicy for Policy {
    fn identity(&self) -> MutationPolicyIdentity { MutationPolicyIdentity::new("keyword-graph", "1", "canonical") }
    fn review(&self, _: &UserContext, plan: &MutationPlan) -> MutationDecision {
        self.plans.lock().unwrap().push(format!("{plan:?}"));
        if self.deny { MutationDecision::deny("KEYWORD-DENIED", "keyword policy denial", ["name"]) }
        else { MutationDecision::allow() }
    }
}
fn native<T: Entity + TeaqlEntity>(entity: T) -> Json { entity.into_json() }
fn ledger(root: &teaql_runtime::EntityRuntimeState) -> Json {
    json!({"changes":format!("{:?}", root.current_change_set()),
        "new":root.new_keys().iter().map(|v|format!("{v:?}")).collect::<Vec<_>>(),
        "deleted":root.deleted_keys().iter().map(|v|format!("{v:?}")).collect::<Vec<_>>()})
}
fn database(db: &rusqlite::Connection) -> Json {
    let mut out = serde_json::Map::new();
    for table in ["platform_data", "owner_data", "entry_data", "profile_data"] {
        let mut statement = db.prepare(&format!("SELECT * FROM {table} ORDER BY id")).unwrap();
        let names = statement.column_names().iter().map(|v|v.to_string()).collect::<Vec<_>>();
        let rows = statement.query_map([], |row| {
            let mut data = serde_json::Map::new();
            for (index, name) in names.iter().enumerate() {
                let value = match row.get_ref(index)? {
                    rusqlite::types::ValueRef::Null => Json::Null,
                    rusqlite::types::ValueRef::Integer(v) => json!(v),
                    rusqlite::types::ValueRef::Text(v) => json!(std::str::from_utf8(v).unwrap()),
                    _ => panic!("unexpected fixture SQLite type"),
                };
                data.insert(name.clone(), value);
            }
            Ok(Json::Object(data))
        }).unwrap().collect::<Result<Vec<_>, _>>().unwrap();
        out.insert(table.into(), json!(rows));
    }
    Json::Object(out)
}
fn finish<T: Entity + TeaqlEntity + LedgerEntity + Clone + Send + 'static>(entity: T, user_context: &UserContext,
    pool: &Arc<Mutex<rusqlite::Connection>>, input: &Json,
    events: &Arc<Mutex<Vec<String>>>, plans: &Arc<Mutex<Vec<String>>>) -> Json {
    let root = entity.entity_runtime_state().unwrap();
    let before = native(entity.clone());
    let pending_before = ledger(&root);
    let fault = input["fault"].as_str().unwrap();
    let (table, id) = if fault.starts_with("root") {
        match input["target"].as_str().unwrap() { "owner" => ("owner_data", 10), _ => ("entry_data", 20) }
    } else { ("entry_data", if fault.starts_with("second") {21} else {20}) };
    {
        let db = pool.lock().unwrap();
        if fault.ends_with("conflict") { db.execute(&format!("UPDATE {table} SET version=version+1 WHERE id=?1"), [id]).unwrap(); }
        if fault.ends_with("trigger") {
            db.execute_batch(&format!("CREATE TRIGGER keyword_failure BEFORE UPDATE ON {table} WHEN OLD.id={id} BEGIN SELECT RAISE(ABORT,'keyword write failure'); END;")).unwrap();
        }
        if fault == "insert_trigger" { db.execute_batch("CREATE TRIGGER keyword_insert_failure BEFORE INSERT ON entry_data BEGIN SELECT RAISE(ABORT,'keyword insert failure'); END;").unwrap(); }
    }
    let db_before = database(&pool.lock().unwrap());
    user_context.clear_sql_logs();
    let mut saved = None;
    let mut error = None;
    if input["op"] != "read" {
        match driver::block_on(teaql_runtime::save_audited_ledger_entity(entity.audit_as("Save canonical keyword graph"), user_context)) {
            Ok(value) => saved = Some(native(value)),
            Err(value) => error = Some(value.to_string()),
        }
    }
    json!({"before":before,"pending_before":pending_before,"saved":saved,"error":error,
        "db_before":db_before,"db_after":database(&pool.lock().unwrap()),"pending_after":ledger(&root),
        "events":*events.lock().unwrap(),"plans":*plans.lock().unwrap(),
        "logs":user_context.sql_logs().iter().map(|v|json!({"sql":v.sql,"params":v.params.iter().map(|v|format!("{v:?}")).collect::<Vec<_>>(),
            "operation":format!("{:?}",v.operation),"comment":v.comment,"purpose":v.purpose,"audit_reason":v.audit_reason,
            "context":format!("{:?}",v.log_context),"debug_sql":v.debug_sql,"pretty_sql":v.pretty_sql,
            "result_count":v.result_count,"result_type":v.result_type,"affected_rows":v.affected_rows,"result_summary":v.result_summary,
            "trace":v.trace_path.iter().map(|v|format!("{v:?}")).collect::<Vec<_>>()})).collect::<Vec<_>>()})
}
fn observe(input: &Json) -> Json {
    let pool = Arc::new(Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));
    pool.lock().unwrap().execute_batch(include_str!("../../../../support/relation_identity_sql.sql")).unwrap();
    let mut user_context = driver::block_on(cpp_relation_identities_core::service_runtime_from_pool(pool.clone())).unwrap().with_active_root("Platform", 1);
    user_context.set_trace_id("keyword-graph"); user_context.set_user_identifier("fixture");
    user_context.set_internal_id_generator(teaql_runtime::AtomicCounterIdGenerator::new(1000));
    let events = Arc::new(Mutex::new(Vec::new()));
    user_context.set_custom_event_sink(Sink{events:events.clone(),fail:input["audit_fail"]==true});
    let plans = Arc::new(Mutex::new(Vec::new()));
    if input["policy"] != "none" {
        let policy:Arc<dyn MutationPolicy> = Arc::new(Policy{plans:plans.clone(),deny:input["policy"]=="deny"});
        user_context.set_mutation_policy_registry(move |_:&str|Some(policy.clone()));
    }
    let sparse = input["minimal"]==true;
    let selection = input["selection"].as_str().unwrap();
    let op = input["op"].as_str().unwrap();
    macro_rules! compose {($root:expr,$child:expr)=>{{
        if input["composition"]=="attach" { $child.attach_runtime_state_recursive($root.entity_runtime_state().unwrap()); }
        else if input["composition"]!="none" { $root.include_pending_mutations_from(&$child).unwrap(); }
    }};}
    if input["target"] == "owner" {
        let mut request = if sparse {Q::owners_minimal()} else {Q::owners()};
        if selection=="first" || selection=="both" || selection=="minimal_child" {
            request=request.select_class_with(if selection=="minimal_child" {Q::entries_minimal()} else {Q::entries()});
        }
        if selection=="second" || selection=="both" { request=request.select_template().select_operator(); }
        let mut entity=driver::block_on(request.with_id_is(10_u64).comment("Load keyword graph").purpose("Review graph change boundary").execute_for_one(&user_context)).unwrap().unwrap();
        if op!="read" { entity.update_name("Updated owner"); }
        if matches!(op,"related"|"mixed"|"delete"|"foreign") {
            if let Some(children)=entity.class().value() {
                let mut children=children.clone();
                for (index,child) in children.data.iter_mut().enumerate() {
                    if op=="delete" || (op=="mixed" && index==1) {child.mark_for_deletion();}
                    else if op=="foreign" {child.update_namespace_id(11_u64);}
                    else {child.update_name("Updated class member");}
                    compose!(entity,*child);
                }
            }
            if matches!(op,"related"|"mixed") {
                if let Some(children)=entity.template().value() {
                    let mut children=children.clone();
                    for child in &mut children.data { child.update_name("Updated template member"); compose!(entity,*child); }
                }
                if let Some(mut child)=entity.operator().value().cloned() {child.update_name("Updated profile");compose!(entity,child);}
            }
        }
        if matches!(op,"create"|"mixed") {
            let mut child=Q::entries().comment("Construct keyword child").purpose("Extend reviewed aggregate").new_entity(&user_context);
            child.update_name("New class member").update_namespace_id(10_u64).update_operator_id(11_u64);
            compose!(entity,child);
        }
        finish(entity,&user_context,&pool,input,&events,&plans)
    } else {
        let mut request=if sparse {Q::entries_minimal()} else {Q::entries()};
        if selection=="first" || selection=="both" || selection=="minimal_child" {
            request=request.select_namespace_with(if selection=="minimal_child" {Q::owners_minimal()} else {Q::owners()});
        }
        if selection=="second" || selection=="both" {request=request.select_operator();}
        let mut entity=driver::block_on(request.with_id_is(20_u64).comment("Load keyword graph").purpose("Review graph change boundary").execute_for_one(&user_context)).unwrap().unwrap();
        if op=="delete" { entity.mark_for_deletion(); }
        else if op!="read" {entity.update_name("Updated entry");}
        if matches!(op,"related"|"mixed") {
            if let Some(mut child)=entity.namespace().cloned(){child.update_name("Updated namespace owner");compose!(entity,child);}
            if let Some(mut child)=entity.operator().cloned(){child.update_name("Updated operator owner");compose!(entity,child);}
        }
        if op=="foreign" {entity.update_namespace_id(11_u64).update_operator_id(teaql_core::Value::Null);}
        if matches!(op,"create"|"mixed") {
            let mut child=Q::owners().comment("Construct keyword owner").purpose("Extend reviewed aggregate").new_entity(&user_context);
            child.update_name("New namespace owner");entity.update_namespace_id(child.id());compose!(entity,child);
        }
        finish(entity,&user_context,&pool,input,&events,&plans)
    }
}
fn main(){for line in io::stdin().lock().lines(){let input:Json=serde_json::from_str(&line.unwrap()).unwrap();println!("{}",observe(&input));}}
