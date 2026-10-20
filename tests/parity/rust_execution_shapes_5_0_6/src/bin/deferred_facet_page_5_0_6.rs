//! Deferred boundaries over original generated execution and real provider/store algorithms.
use cpp_dynamic_json_core::{Q,request_support::{TeaqlRuntime,TeaqlEntityStream}};
use serde_json::{Value as Json,json};
use std::{future::Future,io::{self,BufRead},pin::Pin,sync::{Arc,Mutex},task::{Context,Poll,Wake,Waker}};
use teaql_core::{CompactRow,Entity,TeaqlEntity,SmartList,RelationAggregate};
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
        let phase=if request.query.entity=="Person"{"facet"}else if request.query.aggregates.is_empty(){"rows"}else{"count"};
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
#[derive(Clone,Copy)]
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
    fn fetch_facet_smart_list<'a>(&'a self,entity:&'a str,query:&'a PurposedSelectQuery,aggregates:&'a [RelationAggregate],trace:Vec<teaql_core::TraceNode>)->Pin<Box<dyn Future<Output=Result<SmartList<CompactRow>,RuntimeError>>+Send+'a>> {Box::pin(async move {self.0.entity_data_service::<Executor>(entity).map_err(|e|RuntimeError::Graph(e.to_string()))?.with_trace_context(trace).fetch_smart_list_with_relation_aggregates(query,aggregates).await.map_err(|e|RuntimeError::Graph(e.to_string()))})}
}
struct Counter(Mutex<u64>);
impl Wake for Counter {fn wake(self:Arc<Self>){*self.0.lock().unwrap()+=1;}fn wake_by_ref(self:&Arc<Self>){*self.0.lock().unwrap()+=1;}}
fn wait<T>(future:impl Future<Output=T>)->T {let mut future=Box::pin(future);let waker=Waker::from(Arc::new(Counter(Mutex::new(0))));let mut cx=Context::from_waker(&waker);loop{if let Poll::Ready(v)=future.as_mut().poll(&mut cx){return v;}}}

fn snapshot<T:Entity+TeaqlEntity>(entity:T)->Json {
    let mut loaded=serde_json::Map::new();let descriptor=T::entity_descriptor();
    for field in &descriptor.properties {loaded.insert(field.name.clone(),json!(entity.is_field_loaded(&field.name)));}
    for relation in &descriptor.relations {loaded.insert(relation.name.clone(),json!(entity.is_field_loaded(&relation.name)));}
    json!({"loaded":loaded,"json":entity.into_json()})
}
fn result<T:Entity+TeaqlEntity+Clone>(value:Result<SmartList<T>,RuntimeError>)->Json {
    match value {
        Err(e)=>json!({"error":e.to_string()}),
        Ok(rows)=>{
            let facets=rows.facets.iter().map(|(name,list)|(name.clone(),json!(list.iter().map(|row|
                row.iter().map(|(key,value)|(key.clone(),format!("{value:?}"))).collect::<std::collections::BTreeMap<_,_>>()
            ).collect::<Vec<_>>()))).collect::<std::collections::BTreeMap<_,_>>();
            json!({"rows":rows.iter().cloned().map(snapshot).collect::<Vec<_>>(),"total":rows.total_count,"facets":facets})
        }
    }
}
fn observe(input:&Json)->Json {
    let pool=Arc::new(Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));
    pool.lock().unwrap().execute_batch(include_str!("../../../../support/dynamic_json_schema.sql")).unwrap();
    let mut user_context=wait(cpp_dynamic_json_core::service_runtime_from_pool(pool.clone())).unwrap().with_active_root("Platform",1);
    pool.lock().unwrap().execute_batch(include_str!("../../../../support/dynamic_json_dataset.sql")).unwrap();
    user_context.set_id_set_store(Arc::new(teaql_runtime::InMemoryIdSetStore::default()));
    user_context.set_continuous_page_cursor_store(Arc::new(teaql_runtime::InMemoryContinuousPageCursorStore::default()));
    let control=Arc::new(Mutex::new(Control{gate:input["gate"].as_str().unwrap().into(),..Control::default()}));
    user_context.insert_resource(Executor{inner:cpp_dynamic_json_core::ServiceRuntimeExecutor::new(cpp_dynamic_json_core::DataServiceMutationExecutor::new(pool.clone())),control:control.clone()});
    let runtime=Runtime(&user_context);
    let offset=input["offset"].as_u64().unwrap();let limit=input["limit"].as_u64().unwrap();
    let request=|| {
        let facet=Q::people_minimal().order_by_id_asc().comment("Inspect deferred customers");
        let mut query=Q::orders().order_by_id_asc().skip(offset).limit(limit)
            .facet_by_customer_as_with_options("_customers",facet,input["include_all"]==true);
        query=match input["optimization"].as_str().unwrap(){
            "id_set"=>query.optimize_pagination_with_id_set_config("deferred_facets",60,100),
            "continuous"=>query.optimize_for_continuous_page_fetch_with("deferred_facets",60),
            _=>query,
        };
        query.comment("Inspect deferred facet pages").purpose("Qualify composed query cleanup")
    };
    let execute=|| {
        let query=request();
        async move {
            match input["mode"].as_str().unwrap() {
                "list"=>result(query.execute_for_list(&runtime).await),
                "one"=>match query.execute_for_one(&runtime).await {
                    Err(e)=>json!({"error":e.to_string()}),
                    Ok(row)=>json!({"rows":row.into_iter().map(snapshot).collect::<Vec<_>>(),"total":null,"facets":{}}),
                },
                _=>result(query.execute_for_page(&runtime,offset,limit).await),
            }
        }
    };
    let mut future=Box::pin(execute());
    let counter=Arc::new(Counter(Mutex::new(0)));let waker=Waker::from(counter.clone());let mut cx=Context::from_waker(&waker);
    let initial=match future.as_mut().poll(&mut cx){Poll::Ready(v)=>Some(v),Poll::Pending=>None};
    let pending=control.lock().unwrap().pending;
    let mut out=json!({"pending":pending,"before":control.lock().unwrap().events.clone()});
    let failed_table=if input["gate"]=="facet"{"person_data"}else{"order_data"};
    if pending && input["action"]=="cancel" {drop(future);out["cancelled"]=json!(true);}
    else {
        if input["action"]=="fail" && pending {pool.lock().unwrap().execute_batch(&format!("ALTER TABLE {failed_table} RENAME TO unavailable_table")).unwrap();}
        let wake={let mut c=control.lock().unwrap();c.ready=true;c.wake.take()};if let Some(wake)=wake{wake.wake();}
        out["result"]=initial.unwrap_or_else(||wait(future));
        out["woke"]=json!(*counter.0.lock().unwrap()>0);out["cancelled"]=json!(false);
    }
    if input["action"]=="fail" && pending {pool.lock().unwrap().execute_batch(&format!("ALTER TABLE unavailable_table RENAME TO {failed_table}")).unwrap();}
    out["retry"]=wait(execute());
    let c=control.lock().unwrap();out["events"]=json!(c.events);out["stops"]=json!(c.stops);drop(c);
    out["logs"]=json!(user_context.sql_logs().iter().map(|entry|json!({"sql":entry.sql,
        "params":entry.params.iter().map(|v|format!("{v:?}")).collect::<Vec<_>>(),
        "comment":entry.comment,"purpose":entry.purpose,"debug_sql":entry.debug_sql,
        "pretty_sql":entry.pretty_sql,"context":format!("{:?}",entry.log_context),
        "trace":entry.trace_path.iter().map(|v|format!("{v:?}")).collect::<Vec<_>>(),
        "result_count":entry.result_count,"result_type":entry.result_type,
        "affected_rows":entry.affected_rows,"summary":entry.result_summary})).collect::<Vec<_>>());
    out
}
fn main(){for line in io::stdin().lock().lines(){let input:Json=serde_json::from_str(&line.unwrap()).unwrap();println!("{}",observe(&input));}}
