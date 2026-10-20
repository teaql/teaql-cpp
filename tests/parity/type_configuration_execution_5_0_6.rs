use teaql_core::Entity;
use domain::{Q,E};
include!("type_calls.rs");
use serde_json::{Value as Json, json};
use teaql_provider_sqlite::SqliteProviderExt;
use std::{future::Future, io::{self, BufRead}, sync::{Arc, Mutex}, task::{Context, Poll, Wake, Waker}};

struct NoWake;
impl Wake for NoWake { fn wake(self: Arc<Self>) {} }
fn wait<T>(future: impl Future<Output=T>) -> T {
    let mut future=Box::pin(future);
    let waker=Waker::from(Arc::new(NoWake));
    let mut context=Context::from_waker(&waker);
    loop { if let Poll::Ready(value)=future.as_mut().poll(&mut context) { return value; } }
}
fn table(connection:&rusqlite::Connection,name:&str)->Json {
    let exists=connection.query_row("SELECT count(*) FROM sqlite_master WHERE type='table' AND name=?1",[name],|row|row.get::<_,i64>(0)).unwrap()!=0;
    if !exists { return Json::Null; }
    let mut statement=connection.prepare(&format!("SELECT * FROM {name} ORDER BY 1")).unwrap();
    let names=statement.column_names().into_iter().map(str::to_owned).collect::<Vec<_>>();
    let values=statement.query_map([],|row|{
        let mut result=serde_json::Map::new();
        for (index,name) in names.iter().enumerate() {
            let value=match row.get_ref(index).unwrap() {
                rusqlite::types::ValueRef::Null=>Json::Null,
                rusqlite::types::ValueRef::Integer(v)=>json!(v),
                rusqlite::types::ValueRef::Real(v)=>json!(v),
                rusqlite::types::ValueRef::Text(v)=>json!(std::str::from_utf8(v).unwrap()),
                _=>unreachable!(),
            };
            result.insert(name.clone(),value);
        }
        Ok(Json::Object(result))
    }).unwrap().map(Result::unwrap).collect::<Vec<_>>();
    json!(values)
}
fn snapshot(pool:&Arc<Mutex<rusqlite::Connection>>)->Json {
    let connection=pool.lock().unwrap();
    json!({"platform":table(&connection,"platform_data"),"priority":table(&connection,"priority_data"),"ids":table(&connection,"teaql_id_space")})
}

fn reads(user_context:&teaql_runtime::UserContext)->Json {
    let mut output=Vec::new();
    for filter in [-1,7,9,8] {
        let mut request=Q::priorities().select_value().order_by_id_asc();
        if filter>=0 { request=typed_filter(request,filter); }
        let result=wait(request.comment("what: read configured scalar values")
                       .purpose("why: verify generated type and expression semantics")
                       .execute_for_list(user_context));
        let (rows,error)=match result {
            Ok(rows)=>(rows.iter().map(|entity|json!({
                "id":entity.id(),"version":entity.version(),"value":entity.value(),
                "e":E::priority(entity).get_value().eval(),
                "json":entity.borrowed_json(&mut teaql_core::EntityJsonTraversal::default())
            })).collect::<Vec<_>>(),None),
            Err(error)=>(Vec::new(),Some(error.to_string())),
        };
        output.push(json!({"filter":filter,"rows":rows,"error":error}));
    }
    json!(output)
}

fn observe(input:&Json)->Json {
    let pool=Arc::new(Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));
    let create=||{
        let module=match input["module"].as_str().unwrap() {
            "plain"=>domain::module(),"checker"=>domain::module_with_checkers(),
            "behavior"=>domain::module_with_behaviors(),_=>domain::module_with_behaviors_and_checkers(),
        };
        let mut user_context=module.into_context();
        let transport=domain::DataServiceMutationExecutor::new(pool.clone());
        user_context.set_internal_id_generator(domain::DataServiceIdGenerator::from_executor(transport.clone()));
        user_context.use_sqlite_provider(transport.clone());
        let executor=domain::ServiceRuntimeExecutor::new(transport);
        user_context.register_executor(executor.clone());user_context.insert_resource(executor);
        user_context.disable_sql_log();if input["logging"]==true {user_context.enable_all_sql_log();}
        user_context
    };
    let state=input["state"].as_str().unwrap();
    let mut warmup_error=Json::Null;
    if state!="fresh" {
        let user_context=create();
        if let Err(error)=wait(user_context.ensure_schema()) {warmup_error=json!(error.to_string());}
        drop(user_context);
        if warmup_error.is_null() {
            let connection=pool.lock().unwrap();
            match state {
                "existing"=>{},
                "drift"=>connection.execute_batch("UPDATE priority_data SET name='stale',value=8,version=3 WHERE id=1").unwrap(),
                "missing"=>connection.execute_batch("DELETE FROM priority_data WHERE id=22").unwrap(),
                "deleted"=>connection.execute_batch("UPDATE priority_data SET version=-1 WHERE id=1").unwrap(),
                "extra"=>connection.execute_batch("INSERT INTO priority_data(id,platform,name,code,value,version) VALUES(777,1,'Extra','EXTRA',8,4)").unwrap(),
                _=>panic!("unknown configured-type state"),
            }
        }
    }
    let mut rounds=Vec::new();
    for _ in 0..input["rounds"].as_u64().unwrap() {
        let before=snapshot(&pool);
        let user_context=create();
        let error=wait(user_context.ensure_schema()).err().map(|error|error.to_string());
        let root=user_context.require_active_root("Platform").map(|root|json!(root.id)).unwrap_or_else(|error|json!(error.to_string()));
        let after=snapshot(&pool);
        let queries=reads(&user_context);
        let logs=user_context.sql_logs().iter().map(|entry|json!({
            "sql":entry.sql,"comment":entry.comment,"purpose":entry.purpose,"audit_as":entry.audit_reason,
            "params":entry.params.iter().map(|value|format!("{value:?}")).collect::<Vec<_>>()
        })).collect::<Vec<_>>();
        drop(user_context);
        rounds.push(json!({"before":before,"after":after,"error":error,"root":root,"reads":queries,"logs":logs}));
    }
    json!({"warmup_error":warmup_error,"rounds":rounds})
}

fn main() {
    for line in io::stdin().lock().lines() {
        let input:Json=serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}",observe(&input));
    }
}
