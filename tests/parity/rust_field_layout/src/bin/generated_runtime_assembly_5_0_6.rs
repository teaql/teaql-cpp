//! Original ordinary-module registration and automatic schema bootstrap.
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
        if let Poll::Ready(value) = future.as_mut().poll(&mut context) {
            return value;
        }
    }
}
fn observe(input: &Value) -> Value {
    macro_rules! run { ($domain:ident, $root:literal, $entities:expr, $table:literal) => {{
        let module = match input["mode"].as_str().unwrap() {
            "plain" => $domain::module(), "checkers" => $domain::module_with_checkers(),
            "behaviors" => $domain::module_with_behaviors(), _ => $domain::module_with_behaviors_and_checkers(),
        };
        let pool = Arc::new(Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));
        let mut user_context = module.into_context();
        let transport = teaql_provider_sqlite::SqliteMutationExecutor::new(pool.clone());
        let ids = teaql_provider_sqlite::SqliteIdSpaceGenerator::from_executor(transport.clone());
        user_context.set_internal_id_generator(ids);
        use teaql_provider_sqlite::SqliteProviderExt as _;
        user_context.use_sqlite_provider(transport.clone());
        let executor = $domain::ServiceRuntimeExecutor::new(transport);
        user_context.register_executor(executor.clone()); user_context.insert_resource(executor);
        let registration = $entities.iter().map(|name| json!({"entity":name,"metadata":user_context.require_entity(name).is_ok(),"checker":user_context.has_checker(name),"behavior":user_context.entity_data_service_behavior(name).is_some()})).collect::<Vec<_>>();
        let mut rounds = Vec::new();
        for index in 0..input["rounds"].as_u64().unwrap() {
            let result = wait(user_context.ensure_schema());
            let connection = pool.lock().unwrap();
            let exists = connection.query_row("SELECT count(*) FROM sqlite_master WHERE type='table' AND name=?1",[$table],|row|row.get::<_,i64>(0)).unwrap()!=0;
            let rows = if exists { let mut statement = connection.prepare(concat!("SELECT id,name,version FROM ",$table," ORDER BY id")).unwrap(); let rows=statement.query_map([], |row| Ok(json!([row.get::<_,i64>(0)?,row.get::<_,String>(1)?,row.get::<_,i64>(2)?]))).unwrap();rows.map(Result::unwrap).collect::<Vec<_>>() } else {Vec::new()};
            rounds.push(json!({"error":result.err().map(|e|e.to_string()),"root":user_context.require_active_root($root).ok().map(|v|v.id),"rows":rows}));
            if index==0 && input["edit"]==true && exists { connection.execute(concat!("UPDATE ",$table," SET name='application owned root',version=7 WHERE id=1"),[]).unwrap(); }
        }
        json!({"registration":registration,"rounds":rounds})
    }}; }
    match input["model"].as_str().unwrap() {
        "checkers" => run!(
            cpp_checker_fixture_core,
            "Tenant",
            ["Tenant", "Note"],
            "tenant_data"
        ),
        "inheritance" => run!(
            cpp_inheritance_core,
            "Platform",
            ["Platform", "School", "Academy", "ResearchAcademy"],
            "platform_data"
        ),
        _ => run!(
            cpp_relations_core,
            "Platform",
            ["Platform", "School", "Profile", "Device"],
            "platform_data"
        ),
    }
}
fn main() {
    for line in io::stdin().lock().lines() {
        let input: Value = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", observe(&input));
    }
}
