use teaql_provider_sqlite::SqliteProviderExt;
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
struct BootstrapSink { events: Arc<Mutex<Vec<Json>>>, fail: bool }
impl teaql_runtime::SafeAuditEventSink for BootstrapSink {
    fn on_safe_event(&self,_:&teaql_runtime::UserContext,event:&teaql_runtime::SafeAuditEvent)->Result<(),teaql_runtime::RuntimeError> {
        let bootstrap=event.bootstrap_audit.as_ref().map(|identity|json!({"actor":identity.actor,"category":identity.category,"reason":identity.reason,"version":identity.resulting_version}));
        let fields:Vec<_>=event.fields.iter().map(|field|json!({"name":field.name,"value":field.value,"masked":field.masked,"truncated":field.truncated,"raw_length":field.raw_length,"output_length":field.output_length,"mask_reason":field.mask_reason,"truncate_reason":field.truncate_reason})).collect();
        self.events.lock().unwrap().push(json!({"kind":format!("{:?}",event.kind),"entity":event.entity,"id":event.entity_id,"fields":fields,"trace":format!("{:?}",event.trace_chain),"governance":event.mutation_governance.as_ref().map(|g|format!("{g:?}")),"bootstrap":bootstrap}));
        if self.fail && matches!(event.kind,teaql_runtime::RawAuditEventKind::Created|teaql_runtime::RawAuditEventKind::Updated) { Err(teaql_runtime::RuntimeError::Event("fixture bootstrap audit failure".into())) } else { Ok(()) }
    }
}
struct BootstrapPolicy { plans:Arc<Mutex<Vec<String>>>, deny:bool, target:String }
impl teaql_runtime::MutationPolicy for BootstrapPolicy {
    fn identity(&self)->teaql_runtime::MutationPolicyIdentity { teaql_runtime::MutationPolicyIdentity::new("bootstrap-policy","1","fingerprint") }
    fn review(&self,_:&teaql_runtime::UserContext,plan:&teaql_runtime::MutationPlan)->teaql_runtime::MutationDecision {
        self.plans.lock().unwrap().push(format!("{plan:?}"));
        if self.deny && plan.root_entity_type==self.target { teaql_runtime::MutationDecision::deny("BOOTSTRAP-DENIED","fixture bootstrap policy denial",std::iter::empty::<String>()) } else { teaql_runtime::MutationDecision::allow() }
    }
}
fn observe(input: &Json) -> Json {
    let pool = Arc::new(Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));
    let scalar = input["fixture"] == "numeric";
    let create = || {
        if input.get("module").is_none() {
            if scalar { wait(cpp_constants_numeric_core::service_runtime_from_pool(pool.clone())).unwrap() }
            else { wait(cpp_constants_catalog_core::service_runtime_from_pool(pool.clone())).unwrap() }
        } else {
            let mode=input["module"].as_str().unwrap();
            let module=if scalar {
                match mode { "plain"=>cpp_constants_numeric_core::module(),"checker"=>cpp_constants_numeric_core::module_with_checkers(),"behavior"=>cpp_constants_numeric_core::module_with_behaviors(),_=>cpp_constants_numeric_core::module_with_behaviors_and_checkers() }
            } else {
                match mode { "plain"=>cpp_constants_catalog_core::module(),"checker"=>cpp_constants_catalog_core::module_with_checkers(),"behavior"=>cpp_constants_catalog_core::module_with_behaviors(),_=>cpp_constants_catalog_core::module_with_behaviors_and_checkers() }
            };
            let transport=cpp_constants_numeric_core::DataServiceMutationExecutor::new(pool.clone());
            let mut context=module.into_context();
            context.set_internal_id_generator(cpp_constants_numeric_core::DataServiceIdGenerator::from_executor(transport.clone()));
            context.use_sqlite_provider(transport.clone());
            if scalar { let executor=cpp_constants_numeric_core::ServiceRuntimeExecutor::new(transport);context.register_executor(executor.clone());context.insert_resource(executor); }
            else { let executor=cpp_constants_catalog_core::ServiceRuntimeExecutor::new(transport);context.register_executor(executor.clone());context.insert_resource(executor); }
            context
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
    let mut user_context = create();
    let tables = if scalar {
        vec!["platform_data", "priority_data", "label_data"]
    } else {
        vec!["platform_data", "category_data"]
    };
    let events=Arc::new(Mutex::new(Vec::new()));
    let plans=Arc::new(Mutex::new(Vec::new()));
    if let Some(mode)=input["governance"].as_str() {
        user_context.set_trace_id("bootstrap");user_context.set_user_identifier("fixture");
        user_context.set_custom_event_sink(BootstrapSink{events:events.clone(),fail:input["audit_fail"].as_bool().unwrap()});
        if mode!="none" {
            let target=if mode=="deny_root" {"Platform"} else if scalar {"Priority"} else {"Category"};
            let policy:Arc<dyn teaql_runtime::MutationPolicy>=Arc::new(BootstrapPolicy{plans:plans.clone(),deny:mode=="deny_root"||mode=="deny_constant",target:target.into()});
            user_context.set_mutation_policy_registry(move|_:&str|Some(policy.clone()));
            if mode=="approved" { user_context.set_mutation_policy_approval_provider(|identity:&teaql_runtime::MutationPolicyIdentity|Some(teaql_runtime::MutationPolicyApproval::new(identity.clone(),"reviewer",std::time::UNIX_EPOCH))); }
        }
    }
    let mut rounds = Vec::new();
    for _ in 0..input["rounds"].as_u64().unwrap() {
        let before = database(&pool.lock().unwrap(), &tables);
        let result = wait(user_context.ensure_schema());
        rounds.push(json!({"before":before,"after":database(&pool.lock().unwrap(),&tables),"error":result.err().map(|error|error.to_string()),
            "root":user_context.require_active_root("Platform").ok().map(|root|root.id)}));
    }
    let mut out=json!({"rounds":rounds});
    if input.get("governance").is_some() {out["events"]=json!(*events.lock().unwrap());out["plans"]=json!(*plans.lock().unwrap());}
    out
}
fn main() {
    for line in io::stdin().lock().lines() {
        let input: Json = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", observe(&input));
    }
}
