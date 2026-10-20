use cpp_inheritance_core as domain;
use serde_json::{Value as Json,json};
use teaql_provider_sqlite::SqliteProviderExt;
use std::{future::Future,io::{self,BufRead},sync::{Arc,Mutex},task::{Context,Poll,Wake,Waker}};
struct NoWake;
impl Wake for NoWake {fn wake(self:Arc<Self>) {}}
fn wait<T>(future:impl Future<Output=T>)->T {
    let mut future=Box::pin(future);let waker=Waker::from(Arc::new(NoWake));let mut context=Context::from_waker(&waker);
    loop {if let Poll::Ready(value)=future.as_mut().poll(&mut context) {return value;}}
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

fn snapshot(connection:&rusqlite::Connection)->Json {
    json!({"platform":table(connection,"platform_data"),"school":table(connection,"school_data"),
           "academy":table(connection,"academy_data"),"research":table(connection,"research_academy_data"),
           "ids":table(connection,"teaql_id_space")})
}
fn observe(input:&Json)->Json {
    let pool=Arc::new(Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));
    let module=match input["module"].as_str().unwrap() {
        "plain"=>domain::module(),"checker"=>domain::module_with_checkers(),
        "behavior"=>domain::module_with_behaviors(),_=>domain::module_with_behaviors_and_checkers(),
    };
    let mut user_context=module.into_context();
    let transport=domain::DataServiceMutationExecutor::new(pool.clone());
    user_context.set_internal_id_generator(domain::DataServiceIdGenerator::from_executor(transport.clone()));
    user_context.use_sqlite_provider(transport.clone());
    let executor=domain::ServiceRuntimeExecutor::new(transport);user_context.register_executor(executor.clone());user_context.insert_resource(executor);
    user_context.disable_sql_log();
    let schema_error=wait(user_context.ensure_schema()).err().map(|e|e.to_string());
    if schema_error.is_some() {return json!({"schema_error":schema_error});}
    {
        let connection=pool.lock().unwrap();
        match input["root"].as_str().unwrap() {
            "missing"=>connection.execute_batch("DELETE FROM platform_data;").unwrap(),
            "modified"=>connection.execute_batch("UPDATE platform_data SET name='application root',version=4 WHERE id=1;").unwrap(),
            _=>{},
        }
        if let Some(target)=input.get("fault").and_then(Json::as_str) {
            assert!(["school","academy","research_academy"].contains(&target));
            connection.execute_batch(&format!("CREATE TRIGGER sample_failure BEFORE INSERT ON {target}_data WHEN NEW.id=3 BEGIN SELECT RAISE(ABORT,'sample fixture write failed'); END;")).unwrap();
        }
    }
    user_context.enable_all_sql_log();
    let mut rounds=Vec::new();
    for _ in 0..input["rounds"].as_u64().unwrap() {
        user_context.clear_sql_logs();
        let before=snapshot(&pool.lock().unwrap());
        let scale=if input["scale"]=="Small" {domain::SampleDataScale::Small} else {domain::SampleDataScale::Tiny};
        let result=wait(domain::generate_sample_data(&user_context,domain::SampleDataPlan {scale,seed:input["seed"].as_u64().unwrap()}));
        let report=match result {
            Ok(report)=>json!({"generated":report.generated,"skipped":report.skipped.iter().map(|s|json!({"entity":s.entity,"reason":s.reason})).collect::<Vec<_>>(),"error":null}),
            Err(error)=>json!({"error":error.to_string()}),
        };
        let logs=user_context.sql_logs().iter().map(|e|json!({
            "context":format!("{:?}",e.log_context),"operation":format!("{:?}",e.operation),
            "sql":e.sql,"params":e.params.iter().map(|v|format!("{v:?}")).collect::<Vec<_>>(),
            "comment":e.comment,"purpose":e.purpose,"audit_reason":e.audit_reason,
            "trace":format!("{:?}",e.trace_path),"debug_sql":e.debug_sql,"pretty_sql":e.pretty_sql,
            "result_count":e.result_count,"result_type":e.result_type,"affected_rows":e.affected_rows,"summary":e.result_summary,
        })).collect::<Vec<_>>();
        rounds.push(json!({"before":before,"report":report,"after":snapshot(&pool.lock().unwrap()),"logs":logs}));
    }
    json!({"schema_error":null,"rounds":rounds})
}
fn main() {
    for line in io::stdin().lock().lines() {let input:Json=serde_json::from_str(&line.unwrap()).unwrap();println!("{}",observe(&input));}
}
