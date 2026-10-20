#[cfg(test)]
mod cpp_resolved_select_preparation_observer {
    use super::*;
    use crate::{RuntimeModule,UserContext,RequestPolicy,SchemaProvider,SchemaInvocation,GeneratedSchemaBootstrapFuture};
    use serde_json::{Value as Json,json};
    use std::{future::Future,pin::Pin,sync::{Arc,Mutex},io};
    use teaql_core::*;
    use teaql_data_service::*;

    struct State { input:Json, events:Mutex<Vec<Json>>, output:Mutex<Option<Json>> }
    fn snapshot(query:&SelectQuery)->Json {
        json!({"entity":query.entity,"projection":query.projection,
            "relations":query.relations.iter().map(|load|&load.name).collect::<Vec<_>>(),
            "comment":query.comment,"purpose":query.purpose,
            "trace":query.trace_chain.iter().map(|node|&node.comment).collect::<Vec<_>>()})
    }
    fn frame(name:&str)->TraceNode { TraceNode::typed(TraceKind::Request,"Parent",None,name) }
    fn apply(state:&State,query:&mut SelectQuery,mode:&str,stage:&str)->Result<(),RuntimeError> {
        state.events.lock().unwrap().push(json!({"stage":stage,"query":snapshot(query)}));
        if mode=="deny" {
            return Err(if stage=="policy" {RuntimeError::Policy("select denied".into())}
                else {RuntimeError::Behavior("select denied".into())});
        }
        if mode=="rewrite" {
            query.entity=if stage=="policy" {"Policy"} else {"Rewritten"}.into();
            query.projection=vec![stage.into()];
            query.relations.push(RelationLoad::new("children"));
            query.trace_chain.push(frame(stage));
        }
        Ok(())
    }
    struct Hook(Arc<State>,String);
    impl EntityDataServiceBehavior for Hook {
        fn before_select(&self,_:&UserContext,query:&mut SelectQuery)->Result<(),RuntimeError> {
            apply(&self.0,query,self.0.input["behavior"].as_str().unwrap(),&self.1)
        }
    }
    struct Policy(Arc<State>);
    impl RequestPolicy for Policy {
        fn enforce_select(&self,_:&UserContext,query:&mut SelectQuery)->Result<(),RuntimeError> {
            apply(&self.0,query,self.0.input["policy"].as_str().unwrap(),"policy")
        }
    }
    struct Executor;
    impl DataServiceExecutor for Executor {
        type Error=io::Error;
        fn capabilities(&self)->DataServiceCapabilities {DataServiceCapabilities::default()}
    }
    impl QueryExecutor for Executor {
        async fn query(&self,_:QueryRequest)->Result<QueryResult,Self::Error>{panic!("preparation cannot query")}
    }
    impl MutationExecutor for Executor {
        async fn mutate(&self,_:MutationRequest)->Result<MutationResult,Self::Error>{panic!("preparation cannot mutate")}
    }
    fn run(context:&UserContext,state:&State)->Json {
        let input=&state.input;
        let mut repository=EntityDataService::for_executor(context,"Parent",&Executor);
        if input["trace"]==true {repository=repository.with_trace_context(vec![frame("service")]);}
        if input["intent"]=="bound" {repository.request_intent=Some(QueryIntent::new("bound comment","bound purpose").unwrap());}
        let mut query=SelectQuery::new(input["entity"].as_str().unwrap()).projects(["name"]);
        query.comment=match input["intent"].as_str().unwrap(){"valid"=>Some("  inspect parents  ".into()),"blank"=>Some("  ".into()),_=>None};
        query.purpose=Some("  verify preparation order  ".into());
        if input["trace"]==true {query.trace_chain.push(frame("query"));}
        match input["relations"].as_str().unwrap(){
            "one"=>query.relations.push(RelationLoad::new("children")),
            "duplicate"=>{query.relations.push(RelationLoad::new("children"));query.relations.push(RelationLoad::new("children"));},
            "missing"=>query.relations.push(RelationLoad::new("unknown")),_=>{}
        }
        let before=snapshot(&query);
        let result=repository.prepare_select_query(&query);
        let owned=repository.prepare_select_query_owned(query.clone());
        // Record only one call's events, but require the owned and borrowed forms agree.
        let events=state.events.lock().unwrap().clone();
        assert_eq!(events.len()%2,0);
        assert_eq!(&events[..events.len()/2],&events[events.len()/2..]);
        let encode=|result:Result<SelectQuery,RuntimeError>|match result {Ok(query)=>json!({"ok":snapshot(&query)}),Err(error)=>json!({"error":error.to_string()})};
        let output=encode(result);assert_eq!(output,encode(owned));assert_eq!(before,snapshot(&query));
        json!({"result":output,"source":snapshot(&query),"events":&events[..events.len()/2]})
    }
    struct Schema;
    impl SchemaProvider for Schema {
        fn ensure_schema<'a>(&'a self,_:&'a UserContext,_:&'a SchemaInvocation)->Pin<Box<dyn Future<Output=Result<(),RuntimeError>>+Send+'a>>{Box::pin(async{Ok(())})}
    }
    fn bootstrap(context:&UserContext)->GeneratedSchemaBootstrapFuture<'_>{Box::pin(async move {
        let state=context.get_resource::<Arc<State>>().unwrap();
        *state.output.lock().unwrap()=Some(run(context,state));Ok(())
    })}
    fn evaluate(input:&Json)->Json {
        let state=Arc::new(State{input:input.clone(),events:Mutex::new(vec![]),output:Mutex::new(None)});
        let mut module=RuntimeModule::new();
        if input["metadata"]==true {
            for name in ["Parent","Other","Rewritten","Policy"] {
                let mut descriptor=EntityDescriptor::new(name);
                let mut relation=RelationDescriptor::new("children","Child");
                relation.local_key=format!("{}_key",name.to_lowercase());
                descriptor.relations.push(relation);module=module.descriptor(descriptor);
            }
        }
        if input["behavior"]!="none" {
            for name in ["Parent","Other"] {module=module.behavior(name,Hook(state.clone(),format!("behavior:{name}")));}
        }
        if input["bootstrap"]==true {module=module.generated_schema_bootstrap(bootstrap);}
        let mut context=module.into_context();context.disable_sql_log();context.insert_resource(state.clone());
        if input["policy"]!="none" {context.set_request_policy(Policy(state.clone()));}
        if input["bootstrap"]==true {
            context.set_schema_provider(Schema);
            let waker=std::task::Waker::noop();let mut cx=std::task::Context::from_waker(waker);
            let mut future=std::pin::pin!(context.ensure_schema());
            assert!(matches!(future.as_mut().poll(&mut cx),std::task::Poll::Ready(Ok(()))));
            state.output.lock().unwrap().take().unwrap()
        }else{run(&context,&state)}
    }
    #[test]
    fn replay_callback_intent_and_projection_order(){
        let inputs:Vec<Json>=serde_json::from_str(include_str!("__TEAQL_RESOLVED_SELECT_INPUTS__")).unwrap();
        for input in inputs {println!("TEAQL_RESOLVED_SELECT_JSON {}",evaluate(&input));}
    }
}
