use cpp_constants_numeric_core as domain;
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

struct RaceState { remaining:u64, reviews:Vec<Json> }
struct RacePolicy { rival:Arc<Mutex<rusqlite::Connection>>, state:Arc<Mutex<RaceState>>, target:u64, repair:bool }
impl teaql_runtime::MutationPolicy for RacePolicy {
    fn identity(&self)->teaql_runtime::MutationPolicyIdentity {
        teaql_runtime::MutationPolicyIdentity::new("bootstrap-race","1","fixture")
    }
    fn review(&self,_:&teaql_runtime::UserContext,plan:&teaql_runtime::MutationPlan)->teaql_runtime::MutationDecision {
        let connection=self.rival.lock().unwrap();
        let mut state=self.state.lock().unwrap();
        let before=database(&connection,&["platform_data","priority_data","label_data"]);
        let mut injected=false;
        for operation in &plan.operations {
            if state.remaining!=0 && matches!(operation.kind,teaql_runtime::MutationOperationKind::Update)
                && operation.entity.entity=="Priority" && operation.entity.id.try_u64()==Some(self.target) {
                let repair=if self.repair {if self.target==1 {",rank=-7"}else{",rank=9"}}else{""};
                connection.execute_batch(&format!("UPDATE priority_data SET version=version+1{repair} WHERE id={}",self.target)).unwrap();
                state.remaining-=1;injected=true;break;
            }
        }
        let remaining=state.remaining;
        state.reviews.push(json!({"plan":format!("{plan:?}"),"before":before,"after":database(&connection,&["platform_data","priority_data","label_data"]),"injected":injected,"remaining":remaining}));
        teaql_runtime::MutationDecision::allow()
    }
}

fn observe(input:&Json)->Json {
    let uri="file:teaql_bootstrap_conflict?mode=memory&cache=shared";
    let flags=rusqlite::OpenFlags::SQLITE_OPEN_READ_WRITE|rusqlite::OpenFlags::SQLITE_OPEN_CREATE|rusqlite::OpenFlags::SQLITE_OPEN_URI;
    let pool=Arc::new(Mutex::new(rusqlite::Connection::open_with_flags(uri,flags).unwrap()));
    let rival=Arc::new(Mutex::new(rusqlite::Connection::open_with_flags(uri,flags).unwrap()));
    let create=|mode:&str|{
        let module=match mode {"plain"=>domain::module(),"checker"=>domain::module_with_checkers(),"behavior"=>domain::module_with_behaviors(),_=>domain::module_with_behaviors_and_checkers()};
        let mut user_context=module.into_context();
        let transport=domain::DataServiceMutationExecutor::new(pool.clone());
        user_context.set_internal_id_generator(domain::DataServiceIdGenerator::from_executor(transport.clone()));
        user_context.use_sqlite_provider(transport.clone());
        let executor=domain::ServiceRuntimeExecutor::new(transport);
        user_context.register_executor(executor.clone());user_context.insert_resource(executor);
        user_context.set_trace_id("bootstrap-conflict");user_context.set_user_identifier("fixture");user_context.disable_sql_log();
        user_context
    };
    let warmup=create("combined");wait(warmup.ensure_schema()).unwrap();drop(warmup);
    let target=input["target"].as_u64().unwrap();
    rival.lock().unwrap().execute_batch(&format!("UPDATE priority_data SET rank=55,version={} WHERE id={target}",input["version"].as_u64().unwrap())).unwrap();
    let mut user_context=create(input["module"].as_str().unwrap());
    if input["logging"]==true {user_context.enable_all_sql_log();}
    let state=Arc::new(Mutex::new(RaceState{remaining:input["conflicts"].as_u64().unwrap(),reviews:Vec::new()}));
    let policy:Arc<dyn teaql_runtime::MutationPolicy>=Arc::new(RacePolicy{rival:rival.clone(),state:state.clone(),target,repair:input["repair"]==true});
    user_context.set_mutation_policy_registry(move|_:&str|Some(policy.clone()));
    let mut rounds=Vec::new();
    for _ in 0..input["rounds"].as_u64().unwrap() {
        let before=database(&pool.lock().unwrap(),&["platform_data","priority_data","label_data"]);
        let start=state.lock().unwrap().reviews.len();
        let result=wait(user_context.ensure_schema());
        let state=state.lock().unwrap();
        let after=database(&pool.lock().unwrap(),&["platform_data","priority_data","label_data"]);
        let autocommit=pool.lock().unwrap().is_autocommit();
        rounds.push(json!({"before":before,"after":after,"error":result.err().map(|e|e.to_string()),"reviews":state.reviews.len()-start,"remaining":state.remaining,"autocommit":autocommit}));
    }
    let logs=user_context.sql_logs().iter().map(|entry|json!({"sql":entry.sql,"comment":entry.comment,"purpose":entry.purpose,"audit_as":entry.audit_reason,"params":entry.params.iter().map(|value|format!("{value:?}")).collect::<Vec<_>>()})).collect::<Vec<_>>();
    drop(user_context);
    json!({"rounds":rounds,"plans":state.lock().unwrap().reviews,"logs":logs,"after_close":database(&rival.lock().unwrap(),&["platform_data","priority_data","label_data"])})
}

fn main() {
    for line in io::stdin().lock().lines() {
        let input:Json=serde_json::from_str(&line.unwrap()).unwrap();println!("{}",observe(&input));
    }
}
