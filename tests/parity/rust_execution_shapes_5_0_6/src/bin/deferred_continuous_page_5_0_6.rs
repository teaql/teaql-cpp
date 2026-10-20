use cpp_dynamic_json_core::Q;
use serde_json::{Value as Json,json};
use std::{collections::HashMap,future::Future,io::{self,BufRead},sync::{Arc,Mutex},task::{Context,Poll,Wake,Waker},time::{Duration,SystemTime}};
use teaql_runtime::{ContinuousPageCursor,ContinuousPageCursorStore};
use teaql_core::{SortDirection,Value};

#[derive(Default)]
struct Control {mode:String,gate:String,ready:bool,used:bool,pending:bool,stops:u64,events:Vec<Json>,wake:Option<Waker>,cursors:HashMap<(String,u64),ContinuousPageCursor>}
struct Store{control:Arc<Mutex<Control>>,direction:SortDirection,size:u64}
struct Guard{control:Arc<Mutex<Control>>,active:bool}
impl Drop for Guard{fn drop(&mut self){if self.active{let mut c=self.control.lock().unwrap();c.stops+=1;c.pending=false;c.wake=None;}}}
impl Store{
    async fn gate(&self,name:&str){let mut guard=Guard{control:self.control.clone(),active:false};std::future::poll_fn(|cx|{let mut c=self.control.lock().unwrap();if c.gate==name&&(!c.used||guard.active){c.used=true;if !c.ready{guard.active=true;c.pending=true;c.wake=Some(cx.waker().clone());return Poll::Pending;}guard.active=false;c.pending=false;c.wake=None;}Poll::Ready(())}).await;}
}
#[async_trait::async_trait]
impl ContinuousPageCursorStore for Store{
    async fn get(&self,key:&str,offset:u64)->Result<Option<ContinuousPageCursor>,String>{
        self.control.lock().unwrap().events.push(json!({"get":key,"offset":offset}));self.gate("get").await;
        let c=self.control.lock().unwrap();if c.mode=="error_get"{return Err("fixture cursor read failure".into());}
        if c.mode=="record"{return Ok(c.cursors.get(&(key.into(),offset)).cloned());}
        if c.mode=="miss"||c.mode=="error_put"{return Ok(None);}
        let mut cursor=ContinuousPageCursor{cursor_id:"fixture_cursor".into(),query_key:key.into(),entity:"Order".into(),direction:self.direction.clone(),boundary:Value::I64(if self.direction==SortDirection::Asc{10}else{12}),page_size:self.size,next_offset:offset,expires_at:SystemTime::now()+Duration::from_secs(60)};
        match c.mode.as_str(){"wrong_entity"=>cursor.entity="Person".into(),"wrong_direction"=>cursor.direction=if self.direction==SortDirection::Asc{SortDirection::Desc}else{SortDirection::Asc},"wrong_size"=>cursor.page_size+=1,"wrong_offset"=>cursor.next_offset+=1,"expired"=>cursor.expires_at=SystemTime::UNIX_EPOCH,_=>{}}
        Ok(Some(cursor))
    }
    async fn put(&self,cursor:ContinuousPageCursor)->Result<(),String>{
        self.control.lock().unwrap().events.push(json!({"put":cursor.query_key,"entity":cursor.entity,"direction":format!("{:?}",cursor.direction),"boundary":cursor.boundary.to_json_value(),"size":cursor.page_size,"offset":cursor.next_offset,"id_valid":cursor.cursor_id.starts_with("cpg_")&&cursor.cursor_id[4..].chars().all(|c|c.is_ascii_hexdigit()),"future_expiry":cursor.expires_at>SystemTime::now()}));
        self.gate("put").await;let mut c=self.control.lock().unwrap();if c.mode=="error_put"{return Err("fixture cursor write failure".into());}c.cursors.insert((cursor.query_key.clone(),cursor.next_offset),cursor);Ok(())
    }
    async fn invalidate(&self,key:&str)->Result<(),String>{let mut c=self.control.lock().unwrap();c.events.push(json!({"invalidate":key}));c.cursors.retain(|(k,_),_|k!=key);Ok(())}
}
struct Counter(Mutex<u64>);impl Wake for Counter{fn wake(self:Arc<Self>){*self.0.lock().unwrap()+=1;}fn wake_by_ref(self:&Arc<Self>){*self.0.lock().unwrap()+=1;}}
fn wait<T>(future:impl Future<Output=T>)->T{let mut future=Box::pin(future);let waker=Waker::from(Arc::new(Counter(Mutex::new(0))));let mut cx=Context::from_waker(&waker);loop{if let Poll::Ready(v)=future.as_mut().poll(&mut cx){return v;}}}
fn observe(input:&Json)->Json{
    let pool=Arc::new(Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));pool.lock().unwrap().execute_batch(include_str!("../../../../support/dynamic_json_schema.sql")).unwrap();
    let mut user_context=wait(cpp_dynamic_json_core::service_runtime_from_pool(pool.clone())).unwrap().with_active_root("Platform",1);pool.lock().unwrap().execute_batch(include_str!("../../../../support/dynamic_json_dataset.sql")).unwrap();
    user_context.set_user_identifier("fixture");user_context.set_trace_id("continuous");
    let direction=if input["direction"]=="asc"{SortDirection::Asc}else{SortDirection::Desc};let size=input["size"].as_u64().unwrap();
    let control=Arc::new(Mutex::new(Control{mode:input["mode"].as_str().unwrap().into(),gate:input["gate"].as_str().unwrap().into(),..Control::default()}));
    user_context.set_continuous_page_cursor_store(Arc::new(Store{control:control.clone(),direction:direction.clone(),size}));
    if input["interleave"]==true {
        let request=|offset|{
            let request=if direction==SortDirection::Asc{Q::orders().order_by_id_asc()}else{Q::orders().order_by_id_desc()};
            request.optimize_for_continuous_page_fetch_with("deferred_cursor",60).page_offset(offset,size)
                .comment("what: inspect consecutive cursor pages").purpose("why: qualify cursor store lifetime")
        };
        let first_offset=if input["gate"]=="get"{size}else{0};let peer_offset=if first_offset==0{size}else{0};
        let mut first=Box::pin(request(first_offset).execute_for_list(&user_context));
        let counter=Arc::new(Counter(Mutex::new(0)));let waker=Waker::from(counter.clone());let mut cx=Context::from_waker(&waker);
        assert!(matches!(first.as_mut().poll(&mut cx),Poll::Pending));
        let saved_wake=control.lock().unwrap().wake.as_ref().unwrap().clone();
        let render=|result:Result<teaql_core::SmartList<cpp_dynamic_json_core::Order>,teaql_runtime::RuntimeError>|{
            let mut out=match result {Ok(rows)=>json!({"rows":rows.iter().map(|v|json!([v.id(),v.version()])).collect::<Vec<_>>(),"error":null}),Err(e)=>json!({"error":e.to_string()})};
            out["plan"]=json!(user_context.continuous_page_plan());out["has_cursor"]=json!(user_context.continuous_page_cursor_id().is_some());out
        };
        let peer=render(wait(request(peer_offset).execute_for_list(&user_context)));
        let result=if input["action"]=="cancel" {drop(first);json!({"cancelled":true})}
                   else {control.lock().unwrap().ready=true;saved_wake.wake();render(wait(first))};
        let retry=render(wait(request(size).execute_for_list(&user_context)));
        let c=control.lock().unwrap();return json!({"pending":true,"peer":peer,"first":result,"retry":retry,"events":c.events,"stops":c.stops});
    }
    let mut pages=Vec::new();
    for offset in [0,size,size.saturating_mul(2)]{
        let request=if direction==SortDirection::Asc{Q::orders().order_by_id_asc()}else{Q::orders().order_by_id_desc()};
        let request=request.optimize_for_continuous_page_fetch_with("deferred_cursor",60).page_offset(offset,size).comment("what: inspect consecutive cursor pages").purpose("why: qualify cursor store lifetime");
        let mut future=Box::pin(request.execute_for_list(&user_context));let counter=Arc::new(Counter(Mutex::new(0)));let waker=Waker::from(counter.clone());let mut cx=Context::from_waker(&waker);
        let initial=match future.as_mut().poll(&mut cx){Poll::Ready(v)=>Some(v),Poll::Pending=>None};let pending=control.lock().unwrap().pending;
        if pending&&input["action"]=="cancel"{drop(future);pages.push(json!({"pending":true,"cancelled":true}));continue;}
        let wake={let mut c=control.lock().unwrap();c.ready=true;c.wake.take()};if let Some(wake)=wake{wake.wake();}
        let result=initial.unwrap_or_else(||wait(future));
        let mut page=match result{Ok(rows)=>json!({"rows":rows.iter().map(|r|json!([r.id(),r.version()])).collect::<Vec<_>>(),"error":null}),Err(e)=>json!({"error":e.to_string()})};
        page["pending"]=json!(pending);page["woke"]=json!(*counter.0.lock().unwrap()>0);page["cancelled"]=json!(false);page["plan"]=json!(user_context.continuous_page_plan());page["has_cursor"]=json!(user_context.continuous_page_cursor_id().is_some());pages.push(page);
    }
    let c=control.lock().unwrap();json!({"pages":pages,"events":c.events,"stops":c.stops})
}
fn main(){for line in io::stdin().lock().lines(){let input:Json=serde_json::from_str(&line.unwrap()).unwrap();println!("{}",observe(&input));}}
