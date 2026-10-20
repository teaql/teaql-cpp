use cpp_dynamic_json_core::{Q,request_support::{TeaqlRuntime,TeaqlEntityStream}};
use serde_json::{Value as Json,json};
use std::{future::Future,io::{self,BufRead},pin::Pin,sync::{Arc,Mutex},task::{Context,Poll,Wake,Waker}};
use teaql_core::{CompactRow,Entity,SmartList,RelationAggregate};
use teaql_runtime::{UserContext,PurposedSelectQuery,RuntimeError};
use teaql_data_service::{DataServiceExecutor,QueryExecutor,MutationExecutor};

#[derive(Default)]
struct Control {gate:String,ready:bool,used:bool,pending:bool,stops:u64,events:Vec<String>,wake:Option<Waker>}
struct Guard {control:Arc<Mutex<Control>>,active:bool}
impl Drop for Guard {fn drop(&mut self){if self.active{let mut c=self.control.lock().unwrap();c.stops+=1;c.pending=false;c.wake=None;}}}
struct Executor {inner:cpp_dynamic_json_core::ServiceRuntimeExecutor,control:Arc<Mutex<Control>>}
impl DataServiceExecutor for Executor {
    type Error=<cpp_dynamic_json_core::ServiceRuntimeExecutor as DataServiceExecutor>::Error;
    fn capabilities(&self)->teaql_data_service::DataServiceCapabilities {self.inner.capabilities()}
}
impl QueryExecutor for Executor {
    async fn query(&self,request:teaql_data_service::QueryRequest)->Result<teaql_data_service::QueryResult,Self::Error> {
        let phase=if request.query.aggregates.is_empty(){"rows"}else{"count"};
        self.control.lock().unwrap().events.push(phase.into());
        let mut guard=Guard{control:self.control.clone(),active:false};
        std::future::poll_fn(|cx|{
            let mut c=self.control.lock().unwrap();
            if c.gate==phase && (!c.used||guard.active) {
                c.used=true;
                if !c.ready {guard.active=true;c.pending=true;c.wake=Some(cx.waker().clone());return Poll::Pending;}
                guard.active=false;c.pending=false;c.wake=None;
            }
            Poll::Ready(())
        }).await;
        self.inner.query(request).await
    }
}
impl MutationExecutor for Executor {
    async fn mutate(&self,request:teaql_data_service::MutationRequest)->Result<teaql_data_service::MutationResult,Self::Error>{self.inner.mutate(request).await}
}
struct Runtime<'a>(&'a UserContext);
impl TeaqlRuntime for Runtime<'_> {
    fn user_context(&self)->&UserContext{self.0}
    fn save_audited_entity<'a,T>(&'a self,audited:teaql_core::Audited<T>)->Pin<Box<dyn Future<Output=Result<T,RuntimeError>>+Send+'a>> where T:teaql_runtime::LedgerEntity+Send+'static {TeaqlRuntime::save_audited_entity(self.0,audited)}
    fn fetch_entity_smart_list<'a,T>(&'a self,entity:&'static str,query:PurposedSelectQuery,aggregates:Vec<RelationAggregate>)->Pin<Box<dyn Future<Output=Result<SmartList<T>,RuntimeError>>+Send+'a>> where T:Entity+Send+'a {
        Box::pin(async move {self.0.entity_data_service::<Executor>(entity).map_err(|e|RuntimeError::Graph(e.to_string()))?.fetch_enhanced_entities_with_relation_aggregates_owned(query,&aggregates).await.map_err(|e|RuntimeError::Graph(e.to_string()))})
    }
    fn fetch_compact_smart_list<'a>(&'a self,entity:&'static str,query:&'a PurposedSelectQuery)->Pin<Box<dyn Future<Output=Result<SmartList<CompactRow>,RuntimeError>>+Send+'a>> {
        Box::pin(async move {self.0.entity_data_service::<Executor>(entity).map_err(|e|RuntimeError::Graph(e.to_string()))?.fetch_smart_list(query).await.map_err(|e|RuntimeError::Graph(e.to_string()))})
    }
    fn fetch_compact_rows<'a>(&'a self,entity:&'static str,query:&'a PurposedSelectQuery)->Pin<Box<dyn Future<Output=Result<Vec<CompactRow>,RuntimeError>>+Send+'a>> {
        Box::pin(async move {self.0.entity_data_service::<Executor>(entity).map_err(|e|RuntimeError::Graph(e.to_string()))?.fetch_all(query).await.map_err(|e|RuntimeError::Graph(e.to_string()))})
    }
    fn fetch_entity_stream<'a,T>(&'a self,entity:&'static str,query:PurposedSelectQuery)->TeaqlEntityStream<'a,T,RuntimeError> where T:Entity+Send+'a {TeaqlRuntime::fetch_entity_stream(self.0,entity,query)}
    fn fetch_facet_smart_list<'a>(&'a self,entity:&'a str,query:&'a PurposedSelectQuery,aggregates:&'a [RelationAggregate],trace:Vec<teaql_core::TraceNode>)->Pin<Box<dyn Future<Output=Result<SmartList<CompactRow>,RuntimeError>>+Send+'a>> {TeaqlRuntime::fetch_facet_smart_list(self.0,entity,query,aggregates,trace)}
}
struct Counter(Mutex<u64>);
impl Wake for Counter {fn wake(self:Arc<Self>){*self.0.lock().unwrap()+=1;}fn wake_by_ref(self:&Arc<Self>){*self.0.lock().unwrap()+=1;}}
fn wait<T>(future:impl Future<Output=T>)->T {let mut future=Box::pin(future);let waker=Waker::from(Arc::new(Counter(Mutex::new(0))));let mut cx=Context::from_waker(&waker);loop{if let Poll::Ready(v)=future.as_mut().poll(&mut cx){return v;}}}
fn observe(input:&Json)->Json {
    let pool=Arc::new(Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));
    pool.lock().unwrap().execute_batch(include_str!("../../../../support/dynamic_json_schema.sql")).unwrap();
    let mut user_context=wait(cpp_dynamic_json_core::service_runtime_from_pool(pool.clone())).unwrap().with_active_root("Platform",1);
    pool.lock().unwrap().execute_batch(include_str!("../../../../support/dynamic_json_dataset.sql")).unwrap();
    let control=Arc::new(Mutex::new(Control{gate:input["gate"].as_str().unwrap().into(),..Control::default()}));
    user_context.insert_resource(Executor{inner:cpp_dynamic_json_core::ServiceRuntimeExecutor::new(cpp_dynamic_json_core::DataServiceMutationExecutor::new(pool.clone())),control:control.clone()});
    let runtime=Runtime(&user_context);
    let request=||Q::orders().order_by_id_asc().comment("what: inspect ordinary deferred pages").purpose("why: qualify count and list cleanup");
    let offset=input["offset"].as_u64().unwrap();let limit=input["limit"].as_u64().unwrap();
    let mut future=Box::pin(request().execute_for_page(&runtime,offset,limit));
    let counter=Arc::new(Counter(Mutex::new(0)));let waker=Waker::from(counter.clone());let mut cx=Context::from_waker(&waker);
    let initial=match future.as_mut().poll(&mut cx){Poll::Ready(v)=>Some(v),Poll::Pending=>None};
    let pending=control.lock().unwrap().pending;
    let mut out=json!({"pending":pending,"before":control.lock().unwrap().events.clone()});
    if pending && input["action"]=="cancel" {drop(future);out["cancelled"]=json!(true);}
    else {
        if input["action"]=="fail" && pending {pool.lock().unwrap().execute_batch("ALTER TABLE order_data RENAME TO unavailable_order").unwrap();}
        let wake={let mut c=control.lock().unwrap();c.ready=true;c.wake.take()};if let Some(wake)=wake{wake.wake();}
        let result=initial.unwrap_or_else(||wait(future));
        out["result"]=match result{Ok(rows)=>json!({"rows":rows.iter().map(|v|json!([v.id(),v.version()])).collect::<Vec<_>>(),"total":rows.total_count}),Err(e)=>json!({"error":e.to_string()})};
        out["woke"]=json!(*counter.0.lock().unwrap()>0);out["cancelled"]=json!(false);
    }
    if input["action"]=="fail" && pending {pool.lock().unwrap().execute_batch("ALTER TABLE unavailable_order RENAME TO order_data").unwrap();}
    let retry=wait(request().execute_for_page(&runtime,offset,limit));
    out["retry"]=match retry{Ok(rows)=>json!({"rows":rows.iter().map(|v|json!([v.id(),v.version()])).collect::<Vec<_>>(),"total":rows.total_count}),Err(e)=>json!({"error":e.to_string()})};
    let c=control.lock().unwrap();out["events"]=json!(c.events);out["stops"]=json!(c.stops);out
}
fn main(){for line in io::stdin().lock().lines(){let input:Json=serde_json::from_str(&line.unwrap()).unwrap();println!("{}",observe(&input));}}
