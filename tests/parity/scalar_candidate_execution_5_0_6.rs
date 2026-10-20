use std::{future::Future, io::{self, BufRead}, sync::{Arc, Mutex}, task::{Context, Poll, Wake, Waker}};
use serde_json::{Value as Json, json};
use teaql_core::{Entity, SelectQuery};
use domain::Q;

include!("candidate_calls.rs");

fn wait<T>(future:impl Future<Output=T>)->T {
    struct NoWake;
    impl Wake for NoWake { fn wake(self:Arc<Self>) {} }
    let mut future=Box::pin(future);let waker=Waker::from(Arc::new(NoWake));let mut context=Context::from_waker(&waker);
    loop {if let Poll::Ready(value)=future.as_mut().poll(&mut context) {return value;}}
}
fn observe(input:&Json)->Json {
    let pool=Arc::new(Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));
    pool.lock().unwrap().execute_batch(CANDIDATE_SQL).unwrap();
    let mut user_context=wait(domain::service_runtime_from_pool(pool.clone())).unwrap().with_active_root("Platform",1);
    user_context.disable_sql_log();if input["logging"]==true {user_context.enable_select_sql_log();}
    let mut request=if input["minimal"]==true {Q::records_minimal()} else {Q::records()};
    for step in input["steps"].as_array().unwrap() {
        request=candidate_filter(request,step["index"].as_u64().unwrap() as usize,step["negative"]==true);
    }
    request=request.order_by_id_asc();
    let query:SelectQuery=request.clone().into();
    let mut out=json!({"filter":query.filter.as_ref().map(|value|format!("{value:?}"))});
    let request=request.comment("what: qualify scalar candidates").purpose("why: compare source runtime candidates");
    match wait(request.execute_for_list(&user_context)) {
        Ok(rows)=>{out["rows"]=json!(rows.iter().map(|entity|json!({"id":entity.id(),"version":entity.version(),"json":entity.borrowed_json(&mut teaql_core::EntityJsonTraversal::default())})).collect::<Vec<_>>());out["error"]=Json::Null;},
        Err(error)=>{out["rows"]=json!([]);out["error"]=json!(error.to_string());}
    }
    out["logs"]=json!(user_context.sql_logs().iter().map(|entry|json!({"sql":entry.sql,"comment":entry.comment,"purpose":entry.purpose,"params":entry.params.iter().map(|value|format!("{value:?}")).collect::<Vec<_>>()})).collect::<Vec<_>>());
    out
}
fn main() {for line in io::stdin().lock().lines() {let input:Json=serde_json::from_str(&line.unwrap()).unwrap();println!("{}",observe(&input));}}
