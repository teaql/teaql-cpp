use cpp_inheritance_core::{Q,School,Academy,ResearchAcademy};
use serde_json::{json,Value as Json};
use teaql_core::Entity;
use std::{future::Future,io::{self,BufRead},sync::{Arc,Mutex},task::{Context,Poll,Wake,Waker}};

struct NoWake;
impl Wake for NoWake { fn wake(self:Arc<Self>) {} }
fn wait<T>(future:impl Future<Output=T>)->T {
    let mut future=Box::pin(future);let waker=Waker::from(Arc::new(NoWake));let mut context=Context::from_waker(&waker);
    loop {if let Poll::Ready(value)=future.as_mut().poll(&mut context) {return value;}}
}
fn native<T:Entity>(entity:T)->Json {
    json!(entity.into_values().into_iter().filter(|(name,_)|!name.starts_with('_')).map(|(name,value)|(name,value.to_json_value())).collect::<serde_json::Map<_,_>>())
}
fn observe(input:&Json)->Json {
    let pool=Arc::new(Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));
    let user_context=wait(cpp_inheritance_core::service_runtime_from_pool(pool.clone())).unwrap();
    wait(user_context.ensure_schema()).unwrap();
    pool.lock().unwrap().execute_batch("INSERT INTO school_data(id,name,rank,platform,version) VALUES(7,'school',1,1,3),(8,'next',2,1,3),(9,'last',3,1,3);
        INSERT INTO academy_data(id,name,rank,platform,version,campus_code) VALUES(7,'academy',7,1,4,'north'),(8,'next',8,1,4,'south'),(9,'last',9,1,4,'east');
        INSERT INTO research_academy_data(id,name,rank,platform,version,campus_code,budget) VALUES(7,'research',17,1,5,'lab','12.50'),(8,'next',18,1,5,'lab','10.50'),(9,'last',19,1,5,'lab','8.50');").unwrap();
    if input["fault"]=="bad_rank" {
        let table=match input["source"].as_str().unwrap() {"school"=>"school_data","academy"=>"academy_data",_=>"research_academy_data"};
        pool.lock().unwrap().execute(&format!("UPDATE {table} SET rank='invalid rank' WHERE id=8"),[]).unwrap();
    }
    macro_rules! run {
        ($request:expr,$model:ty)=>{{
            let request=$request.return_type::<$model>().order_by_id_asc().stream(input["chunk"].as_u64().unwrap() as usize)
                .purpose("Qualify inherited stream cleanup").comment("Early close and decode failure");
            let mut rows=Vec::new();let mut error=None;
            let mut stream=wait(request.execute_for_stream(&user_context)).unwrap();
            for _ in 0..input["stop"].as_u64().unwrap() {
                match wait(std::future::poll_fn(|context|stream.as_mut().poll_next(context))) {
                    Some(Ok(entity))=>rows.push(native(entity)),Some(Err(value))=>{error=Some(value.to_string());break;},None=>break,
                }
            }
            drop(stream);
            let available=pool.try_lock().is_ok();
            let reused=if available {wait(Q::schools().with_id_is(7_u64).purpose("Reuse executor after stream").comment("No remaining cursor lease").execute_for_one(&user_context)).unwrap().is_some()} else {false};
            json!({"rows":rows,"error":error,"available":available,"reused":reused})
        }};
    }
    macro_rules! target {($request:expr)=>{match input["target"].as_str().unwrap() {"school"=>run!($request,School),"academy"=>run!($request,Academy),_=>run!($request,ResearchAcademy)}};}
    macro_rules! entry {($normal:expr,$minimal:expr)=>{if input["entry"]=="minimal" {$minimal} else {$normal}};}
    match input["source"].as_str().unwrap() {
        "school"=>target!(entry!(Q::schools(),Q::schools_minimal())),
        "academy"=>target!(entry!(Q::academies(),Q::academies_minimal())),
        _=>target!(entry!(Q::research_academies(),Q::research_academies_minimal())),
    }
}
fn main() {
    for line in io::stdin().lock().lines() {
        let input:Json=serde_json::from_str(&line.unwrap()).unwrap();println!("{}",observe(&input));
    }
}
