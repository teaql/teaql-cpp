use cpp_dynamic_json_core::Q;
use serde_json::{Value as Json,json};
use std::{future::Future,io::{self,BufRead},sync::{Arc,Mutex},task::{Context,Poll,Wake,Waker},time::{Duration,SystemTime}};
use teaql_runtime::{IdSetStore,RetainedIdSet};

#[derive(Default)]
struct Control {mode:String,gate:String,ready:bool,used:bool,pending:bool,gets:u64,cancelled:u64,events:Vec<Json>,wake:Option<Waker>,retained:Option<RetainedIdSet>}
struct Store {control:Arc<Mutex<Control>>}
struct Guard {control:Arc<Mutex<Control>>,active:bool}
impl Drop for Guard {fn drop(&mut self) {if self.active {let mut c=self.control.lock().unwrap();c.cancelled+=1;c.pending=false;c.wake=None;}}}
impl Store {
    async fn gate(&self,name:String) {
        let mut guard=Guard{control:self.control.clone(),active:false};
        std::future::poll_fn(|context|{
            let mut c=self.control.lock().unwrap();
            if c.gate==name && (!c.used || guard.active) {
                c.used=true;
                if !c.ready {guard.active=true;c.pending=true;c.wake=Some(context.waker().clone());return Poll::Pending;}
                guard.active=false;c.pending=false;c.wake=None;
            }
            Poll::Ready(())
        }).await;
    }
}
#[async_trait::async_trait]
impl IdSetStore for Store {
    async fn get(&self,key:&str)->Result<Option<RetainedIdSet>,String> {
        let ordinal={let mut c=self.control.lock().unwrap();c.gets+=1;let n=c.gets;c.events.push(json!({"get":key,"ordinal":n}));n};
        self.gate(format!("get{ordinal}")).await;
        let c=self.control.lock().unwrap();
        if c.mode=="error_get" {return Err("fixture ID store read failure".into());}
        if let Some(value)=&c.retained {return Ok(Some(value.clone()));}
        if c.mode=="hit" || c.mode=="empty" {
            return Ok(Some(RetainedIdSet{query_key:key.into(),ids:Arc::new(if c.mode=="hit" {vec![10,11,12]} else {vec![]}),expires_at:SystemTime::now()+Duration::from_secs(60)}));
        }
        Ok(None)
    }
    async fn put(&self,value:RetainedIdSet)->Result<(),String> {
        self.control.lock().unwrap().events.push(json!({"put":value.query_key,"ids":value.ids.as_ref()}));
        self.gate("put".into()).await;
        let mut c=self.control.lock().unwrap();
        if c.mode=="error_put" {return Err("fixture ID store write failure".into());}
        c.retained=Some(value);Ok(())
    }
    async fn invalidate(&self,key:&str)->Result<(),String> {let mut c=self.control.lock().unwrap();c.events.push(json!({"invalidate":key}));c.retained=None;Ok(())}
}
struct Counter(Mutex<u64>);
impl Wake for Counter {fn wake(self:Arc<Self>) {*self.0.lock().unwrap()+=1;}fn wake_by_ref(self:&Arc<Self>){*self.0.lock().unwrap()+=1;}}
fn wait<T>(future:impl Future<Output=T>)->T {let mut future=Box::pin(future);let waker=Waker::from(Arc::new(Counter(Mutex::new(0))));let mut cx=Context::from_waker(&waker);loop{if let Poll::Ready(v)=future.as_mut().poll(&mut cx){return v;}}}
fn observe(input:&Json)->Json {
    let pool=Arc::new(Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));
    pool.lock().unwrap().execute_batch(include_str!("../../../../support/dynamic_json_schema.sql")).unwrap();
    let mut user_context=wait(cpp_dynamic_json_core::service_runtime_from_pool(pool.clone())).unwrap().with_active_root("Platform",1);
    pool.lock().unwrap().execute_batch(include_str!("../../../../support/dynamic_json_dataset.sql")).unwrap();
    user_context.set_user_identifier("fixture");user_context.set_trace_id("page");
    let control=Arc::new(Mutex::new(Control{mode:input["mode"].as_str().unwrap().into(),gate:input["gate"].as_str().unwrap().into(),..Control::default()}));
    user_context.set_id_set_store(Arc::new(Store{control:control.clone()}));
    let request=||Q::orders().order_by_id_asc().optimize_pagination_with_id_set_config("deferred_page",60,30)
        .comment("what: inspect deferred typed pages").purpose("why: qualify ID store lifetime");
    let offset=input["offset"].as_u64().unwrap();let limit=input["limit"].as_u64().unwrap();
    let mut future=Box::pin(request().execute_for_page(&user_context,offset,limit));
    let counter=Arc::new(Counter(Mutex::new(0)));let waker=Waker::from(counter.clone());let mut cx=Context::from_waker(&waker);
    let mut initial=None;
    for _ in 0..8 {match future.as_mut().poll(&mut cx){Poll::Ready(value)=>{initial=Some(value);break;},Poll::Pending=>{if control.lock().unwrap().pending{break;}}}}
    let pending=control.lock().unwrap().pending;
    let before=control.lock().unwrap().events.clone();
    let mut out=json!({"pending":pending,"before":before});
    if pending && input["action"]=="cancel" {drop(future);out["cancelled"]=json!(true);}
    else {
        let wake={let mut c=control.lock().unwrap();c.ready=true;c.wake.take()};
        if let Some(wake)=wake {wake.wake();}
        let woke=*counter.0.lock().unwrap()>0;
        let result=initial.unwrap_or_else(||wait(future));
        match result {Ok(rows)=>{out["rows"]=json!(rows.iter().map(|v|json!([v.id(),v.version()])).collect::<Vec<_>>());out["total"]=json!(rows.total_count);out["error"]=Json::Null;},Err(error)=>out["error"]=json!(error.to_string())}
        out["woke"]=json!(woke);out["cancelled"]=json!(false);
    }
    let retry=wait(request().execute_for_page(&user_context,offset,limit));
    out["retry"]=match retry {Ok(rows)=>json!({"rows":rows.iter().map(|v|json!([v.id(),v.version()])).collect::<Vec<_>>(),"total":rows.total_count}),Err(error)=>json!({"error":error.to_string()})};
    let c=control.lock().unwrap();out["events"]=json!(c.events);out["stops"]=json!(c.cancelled);
    out
}
fn main(){for line in io::stdin().lock().lines(){let input:Json=serde_json::from_str(&line.unwrap()).unwrap();println!("{}",observe(&input));}}
