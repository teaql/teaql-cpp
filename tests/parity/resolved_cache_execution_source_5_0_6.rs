// Appended observer; every original runtime body remains unchanged.
#[cfg(test)]
mod cpp_resolved_cache_execution_observer {
    use super::*;
    use serde_json::json;
    use std::{sync::{Arc,Mutex},future::Future,task::{Context,Poll,Wake,Waker},collections::BTreeMap};
    use teaql_data_service::*;
    use crate::{RuntimeTelemetry,RuntimeTelemetryScope,RuntimeOperation,RuntimeAttributeValue};
    #[derive(Default)]
    struct State { events:Mutex<Vec<String>>, active:Mutex<Vec<String>>, classified:Mutex<u64> }
    impl State {
        fn note(&self, text:String) { self.events.lock().unwrap().push(text); }
        fn path(&self)->String { self.active.lock().unwrap().join("/") }
    }
    struct Scope { state:Arc<State>,family:String }
    impl RuntimeTelemetryScope for Scope {
        fn with_context(&self, callback:&mut dyn FnMut()) {
            self.state.active.lock().unwrap().push(self.family.clone());
            callback();
            self.state.active.lock().unwrap().pop();
        }
        fn success(&mut self, attrs:BTreeMap<String,RuntimeAttributeValue>) {
            let outcome=match attrs.get("teaql.cache.result") {Some(RuntimeAttributeValue::String(v))=>v.as_str(),_=>""};
            self.state.note(format!("success:{}:{outcome}",self.family));
        }
        fn failure(&mut self,error:&str) { self.state.note(format!("failure:{}:{error}",self.family)); }
    }
    struct Telemetry(Arc<State>);
    impl RuntimeTelemetry for Telemetry {
        fn start(&self, operation:RuntimeOperation)->Box<dyn RuntimeTelemetryScope> {
            self.0.note(format!("start:{}:{}",operation.family,self.0.path()));
            Box::new(Scope{state:self.0.clone(),family:operation.family})
        }
    }
    fn rows(label:&str)->Vec<CompactRow> { vec![CompactRow::from_map(BTreeMap::from([("source".into(),Value::from(label))]))] }
    struct Cache {state:Arc<State>,hit:bool}
    impl crate::AggregationCacheBackend for Cache {
        fn namespace(&self)->&str {"custom"}
        fn get(&self,_:&str,age:u64)->Option<Vec<CompactRow>> {
            self.state.note(format!("get:{age}:{}",self.state.path())); self.hit.then(Vec::new)
        }
        fn put(&self,_:String,rows:Vec<CompactRow>) {self.state.note(format!("put:{}:{}",rows.len(),self.state.path()));}
        fn invalidate_namespace(&self,_:&str) {}
    }
    struct Executor {state:Arc<State>,fail:bool}
    impl DataServiceExecutor for Executor {type Error=std::io::Error;fn capabilities(&self)->DataServiceCapabilities {DataServiceCapabilities::default()}}
    impl QueryExecutor for Executor {
        fn query_log_intent(&self,_:&SelectQuery)->SqlIntentRedactions {
            *self.state.classified.lock().unwrap()+=1;
            let mut value=SqlIntentRedactions::default();value.capture_target_id(&Value::from("classified"));value
        }
        async fn query(&self,_:QueryRequest)->Result<QueryResult,Self::Error> {
            self.state.note(format!("query:{}",self.state.path()));
            if self.fail {return Err(std::io::Error::other("provider failed"));}
            Ok(QueryResult{rows:rows("provider"),metadata:ExecutionMetadata::unrecorded_query(1)})
        }
    }
    impl MutationExecutor for Executor {async fn mutate(&self,_:MutationRequest)->Result<MutationResult,Self::Error>{panic!("unexpected mutation")}}
    struct Noop;impl Wake for Noop {fn wake(self:Arc<Self>) {}}
    fn ready<T>(future:impl Future<Output=T>)->T {
        let mut future=Box::pin(future);let waker=Waker::from(Arc::new(Noop));let mut context=Context::from_waker(&waker);
        match future.as_mut().poll(&mut context) {Poll::Ready(v)=>v,Poll::Pending=>panic!("fixture suspended")}
    }
    #[test]
    fn cache_execution() {
        let mut case=0;
        for entry in ["borrowed","owned","compact"] {for backend in ["none","concrete","custom","both"] {
        for enabled in [false,true] {for outcome in ["hit","miss","failure"] {for capture in [false,true] {for scoped in [false,true] {
            let state=Arc::new(State::default());
            let mut context=crate::RuntimeModule::new().into_context().with_runtime_telemetry(Arc::new(Telemetry(state.clone())));
            context.set_sql_log_options(crate::SqlLogOptions{select:capture,mutation:false});
            let mut query=SelectQuery::new("Item");
            query.comment=Some("inspect cache execution".into());query.purpose=Some("verify backend and telemetry contracts".into());
            if enabled {query=query.enable_aggregation_cache();query.aggregation_cache.as_mut().unwrap().cache_expired_millis=0;}
            if backend=="concrete" || backend=="both" {
                let cache=crate::InMemoryAggregationCache::with_namespace("concrete");
                if outcome=="hit" || backend=="both" {
                    let key=aggregation_cache_key("concrete",&aggregation_cache_namespace("Item"),&query);
                    cache.put(key,rows("concrete"));
                }
                context.insert_resource(cache);
            }
            if backend=="custom" || backend=="both" {
                context.insert_resource(Arc::new(Cache{state:state.clone(),hit:outcome=="hit"}) as Arc<dyn crate::AggregationCacheBackend>);
            }
            let executor=Executor{state:state.clone(),fail:outcome=="failure"};
            let base=EntityDataService::for_executor(&context,"Item",&executor);
            let scope=scoped.then(||base.query_scoped_service(&query).unwrap());
            let service=scope.as_ref().unwrap_or(&base);
            let result=match entry {"borrowed"=>ready(service.fetch_prepared_query(&query)),"owned"=>ready(service.fetch_prepared_query_owned(query)),_=>ready(service.fetch_prepared_compact_owned(query))};
            let values=result.as_ref().ok().map(|rows|rows.iter().map(|r|match r.get("source").unwrap() {Value::Text(value)=>value.as_str(),_=>panic!("unexpected source") }).collect::<Vec<_>>());
            println!("RESOLVED_CACHE_EXECUTION_OBSERVATION {}",json!({"case":case,"entry":entry,"backend":backend,"enabled":enabled,"outcome":outcome,"capture":capture,"scoped":scoped,"values":values,"error":result.as_ref().err().map(|e|e.to_string()),"events":*state.events.lock().unwrap(),"classified":*state.classified.lock().unwrap()}));
            case+=1;
        }}}}}}
        assert_eq!(case,288);
    }
}
