#[cfg(test)]
mod cpp_resolved_mutation_preparation_observer {
    use super::*;
    use crate::{RuntimeModule, UserContext, RequestPolicy, SchemaProvider, SchemaInvocation,
        GeneratedSchemaBootstrapFuture, InternalIdGenerator, Checker, EntityValues,
        ObjectLocation, CheckResults, CheckResult};
    use serde_json::{Value as Json, json};
    use std::{future::Future, pin::Pin, sync::{Arc, Mutex, atomic::{AtomicU64, Ordering}}, io};
    use teaql_core::*;
    use teaql_data_service::*;

    struct State { input:Json, events:Mutex<Vec<Json>>, output:Mutex<Option<Json>>, next:AtomicU64 }
    fn values(values:&Record)->Json {
        json!(values.iter().map(|(name,value)|(name.clone(),format!("{value:?}"))).collect::<BTreeMap<_,_>>())
    }
    fn trace(nodes:&[TraceNode])->Json {
        json!(nodes.iter().map(|node|json!({"kind":node.kind as u8,"entity":node.entity_type,"comment":node.comment})).collect::<Vec<_>>())
    }
    fn frame(name:&str)->TraceNode { TraceNode::typed(TraceKind::Request,"Parent",None,name) }
    fn insert_snapshot(command:&InsertCommand)->Json {
        json!({"entity":command.entity,"values":values(&command.values),"trace":trace(&command.trace_chain)})
    }
    fn update_snapshot(command:&UpdateCommand)->Json {
        json!({"entity":command.entity,"values":values(&command.values),"trace":trace(&command.trace_chain),
            "id":format!("{:?}",command.id),"version":command.expected_version,
            "old":command.old_values.as_ref().map(|old|values(old))})
    }
    fn value(kind:&str)->Value {
        match kind {
            "null"=>Value::Null,"typed_null"=>Value::TypedNull(DataType::Text),
            "i64_zero"=>Value::I64(0),"u64_zero"=>Value::U64(0),"positive"=>Value::U64(7),
            "negative"=>Value::I64(-1),"text"=>Value::Text("provided".into()),
            "empty_text"=>Value::Text("".into()),"f64_zero"=>Value::F64(0.0),"bool"=>Value::Bool(false),
            _=>panic!("unexpected fixture value")
        }
    }
    fn fields(shape:&str,entity:&str)->(String,String) {
        match entity {
            "Target"=>("target_id".into(),"target_version".into()),
            "Policy"=>("policy_id".into(),"policy_version".into()),
            _ if shape=="custom"=>("pk".into(),"revision".into()),
            _=>("id".into(),"version".into())
        }
    }
    fn apply(state:&State,entity:&mut String,values:&mut MutationValues,trace:&mut Vec<TraceNode>,snapshot:Json,stage:&str,mode:&str)->Result<(),RuntimeError> {
        state.events.lock().unwrap().push(json!({"stage":stage,"command":snapshot}));
        if mode=="deny" {return Err(if stage=="policy" {RuntimeError::Policy("preparation denied".into())}else{RuntimeError::Behavior("preparation denied".into())});}
        if mode=="retarget" {
            *entity=if stage=="policy" {"Policy"}else{"Target"}.into();
            values.insert("name".into(),Value::Text(stage.into()));trace.push(frame(stage));
        }
        Ok(())
    }
    struct Hook(Arc<State>,String);
    impl EntityDataServiceBehavior for Hook {
        fn before_insert(&self,_:&UserContext,command:&mut InsertCommand)->Result<(),RuntimeError> {
            let snapshot=insert_snapshot(command);
            apply(&self.0,&mut command.entity,&mut command.values,&mut command.trace_chain,snapshot,&self.1,self.0.input["behavior"].as_str().unwrap())
        }
        fn before_update(&self,_:&UserContext,command:&mut UpdateCommand)->Result<(),RuntimeError> {
            let snapshot=update_snapshot(command);let mode=self.0.input["behavior"].as_str().unwrap();
            apply(&self.0,&mut command.entity,&mut command.values,&mut command.trace_chain,snapshot,&self.1,mode)?;
            if mode=="retarget" {command.id=Value::U64(9);command.expected_version=Some(77);}Ok(())
        }
    }
    struct Policy(Arc<State>);
    impl RequestPolicy for Policy {
        fn enforce_insert(&self,_:&UserContext,command:&mut InsertCommand)->Result<(),RuntimeError> {
            let snapshot=insert_snapshot(command);
            apply(&self.0,&mut command.entity,&mut command.values,&mut command.trace_chain,snapshot,"policy",self.0.input["policy"].as_str().unwrap())
        }
        fn enforce_update(&self,_:&UserContext,command:&mut UpdateCommand)->Result<(),RuntimeError> {
            let snapshot=update_snapshot(command);let mode=self.0.input["policy"].as_str().unwrap();
            apply(&self.0,&mut command.entity,&mut command.values,&mut command.trace_chain,snapshot,"policy",mode)?;
            if mode=="retarget" {command.id=Value::U64(10);command.expected_version=Some(88);}Ok(())
        }
    }
    struct Id(Arc<State>);
    impl InternalIdGenerator for Id {
        fn generate_id(&self,entity:&str)->Result<u64,RuntimeError> {
            self.0.events.lock().unwrap().push(json!({"stage":"id","entity":entity}));
            if self.0.input["id_failure"]==true {Err(RuntimeError::IdGeneration("allocation failed".into()))}
            else {Ok(self.0.next.fetch_add(1,Ordering::Relaxed))}
        }
    }
    struct Check(Arc<State>,String);
    impl Checker for Check {
        fn entity(&self)->&str {&self.1}
        fn check_and_fix(&self,_:&UserContext,values_:&mut EntityValues,location:&ObjectLocation,results:&mut CheckResults) {
            self.0.events.lock().unwrap().push(json!({"stage":"checker","entity":self.1,"values":values(values_),
                "status":CheckObjectStatus::from_values(values_).as_str(),"location":location.model_path()}));
            match self.0.input["checker"].as_str().unwrap() {
                "fix"=>{values_.insert("name".into(),Value::Text("fixed".into()));},
                "deny"=>results.push(CheckResult::required(location.clone().member("blocked"))),_=>{}
            }
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
        async fn mutate(&self,_:MutationRequest)->Result<MutationResult,Self::Error>{panic!("preparation cannot persist")}
    }
    fn run(context:&UserContext,state:&State)->Json {
        let input=&state.input;let entity=input["entity"].as_str().unwrap();
        let repository=EntityDataService::for_executor(context,"Parent",&Executor).with_trace_context(vec![frame("service")]);
        let (id,version)=fields(input["shape"].as_str().unwrap(),entity);
        let mut initial=Record::from([("__teaql_object_status".into(),Value::Text("update".into()))]);
        if input["name"]==true {initial.insert("name".into(),Value::Text("original".into()));}
        if input["id_kind"]!="missing" {initial.insert(id,value(input["id_kind"].as_str().unwrap()));}
        if input["version_kind"]!="missing" {initial.insert(version,value(input["version_kind"].as_str().unwrap()));}
        let mut output=if input["entry"]=="insert" {
            let mut command=InsertCommand::new(entity);command.values=initial.into();command.trace_chain=vec![frame("command")];
            let before=insert_snapshot(&command);
            let result=match repository.prepare_insert_command(&command) {Ok(command)=>json!({"ok":insert_snapshot(&command)}),Err(error)=>json!({"error":error.to_string()})};
            assert_eq!(before,insert_snapshot(&command));json!({"result":result,"source":before})
        }else{
            let mut command=UpdateCommand::new(entity,Value::U64(7));command.values=initial.into();command.trace_chain=vec![frame("command")];command.expected_version=input["expected_version"].as_i64();
            if input["old"]==true {command.old_values=Some(Record::from([("name".into(),Value::Text("old".into()))]).into());}
            let before=update_snapshot(&command);
            let result=match repository.prepare_update_command(&command) {Ok(command)=>json!({"ok":update_snapshot(&command)}),Err(error)=>json!({"error":error.to_string()})};
            assert_eq!(before,update_snapshot(&command));json!({"result":result,"source":before})
        };
        output["events"]=json!(*state.events.lock().unwrap());output["next_id"]=json!(state.next.load(Ordering::Relaxed));output
    }
    struct Schema;
    impl SchemaProvider for Schema {
        fn ensure_schema<'a>(&'a self,_:&'a UserContext,_:&'a SchemaInvocation)->Pin<Box<dyn Future<Output=Result<(),RuntimeError>>+Send+'a>>{Box::pin(async{Ok(())})}
    }
    fn bootstrap(context:&UserContext)->GeneratedSchemaBootstrapFuture<'_>{Box::pin(async move {
        let state=context.get_resource::<Arc<State>>().unwrap();*state.output.lock().unwrap()=Some(run(context,state));Ok(())
    })}
    fn evaluate(input:&Json)->Json {
        let state=Arc::new(State{input:input.clone(),events:Mutex::new(vec![]),output:Mutex::new(None),next:AtomicU64::new(100)});
        let mut module=RuntimeModule::new();let shape=input["shape"].as_str().unwrap();
        if shape!="missing" {
            for name in ["Parent","Child","Target","Policy"] {
                let (id,version)=fields(shape,name);let mut descriptor=EntityDescriptor::new(name);
                if shape!="no_id" {descriptor=descriptor.property(PropertyDescriptor::new(id,DataType::U64).id());}
                if shape!="no_version" {descriptor=descriptor.property(PropertyDescriptor::new(version,DataType::I64).version());}
                descriptor=descriptor.property(PropertyDescriptor::new("name",DataType::Text).not_null())
                    .property(PropertyDescriptor::new("create_time",DataType::Timestamp))
                    .property(PropertyDescriptor::new("update_time",DataType::Timestamp));
                module=module.descriptor(descriptor);
            }
        }
        if input["behavior"]!="none" {for name in ["Parent","Child"] {module=module.behavior(name,Hook(state.clone(),format!("behavior:{name}")));}}
        if input["checker"]!="none" {for name in ["Parent","Child","Target","Policy"] {module=module.checker(Check(state.clone(),name.into()));}}
        if input["bootstrap"]==true {module=module.generated_schema_bootstrap(bootstrap);}
        let mut context=module.into_context();context.disable_sql_log();context.insert_resource(state.clone());context.set_internal_id_generator(Id(state.clone()));
        if input["policy"]!="none" {context.set_request_policy(Policy(state.clone()));}
        if input["bootstrap"]==true {
            context.set_schema_provider(Schema);let mut cx=std::task::Context::from_waker(std::task::Waker::noop());
            let mut future=std::pin::pin!(context.ensure_schema());assert!(matches!(future.as_mut().poll(&mut cx),std::task::Poll::Ready(Ok(()))));
            state.output.lock().unwrap().take().unwrap()
        }else{run(&context,&state)}
    }
    #[test]
    fn replay_insert_update_preparation(){
        let inputs:Vec<Json>=serde_json::from_str(include_str!("__TEAQL_RESOLVED_MUTATION_INPUTS__")).unwrap();
        for input in inputs {println!("TEAQL_RESOLVED_MUTATION_JSON {}",evaluate(&input));}
    }
}
