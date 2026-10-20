#[cfg(test)]
mod cpp_mutation_intent_privacy_observer {
    use super::*;
    use crate::UserContext;
    use serde_json::{Value as Json, json};
    use std::{future::Future, io, sync::{Arc,Mutex}, task::{Context,Poll,Wake,Waker}};
    use teaql_core::*;
    use teaql_data_service::*;

    struct Executor(Mutex<Vec<Json>>);
    impl DataServiceExecutor for Executor {
        type Error=io::Error;
        fn capabilities(&self)->DataServiceCapabilities { DataServiceCapabilities::default() }
    }
    impl QueryExecutor for Executor {
        async fn query(&self,_:QueryRequest)->Result<QueryResult,Self::Error> { panic!("unused query") }
    }
    impl MutationExecutor for Executor {
        async fn mutate(&self,request:MutationRequest)->Result<MutationResult,Self::Error> {
            self.0.lock().unwrap().push(json!({"comment":request.comment(),
                "privacy":request.diagnostic_redactions().map(SqlIntentRedactions::is_empty)}));
            Ok(MutationResult{affected_rows:1,generated_values:GeneratedValues::new(),
                persisted_snapshot:None,metadata:ExecutionMetadata::unrecorded_query(0)})
        }
    }
    struct NoWake;
    impl Wake for NoWake { fn wake(self:Arc<Self>) {} }
    fn wait<F:Future>(future:F)->F::Output {
        let mut future=std::pin::pin!(future);
        let waker=Waker::from(Arc::new(NoWake));let mut context=Context::from_waker(&waker);
        match future.as_mut().poll(&mut context) { Poll::Ready(value)=>value,Poll::Pending=>panic!("unexpected pending") }
    }
    #[test]
    fn replacing_intent_preserves_existing_privacy() {
        let inputs:Vec<Json>=serde_json::from_str(include_str!("__TEAQL_INTENT_PRIVACY_INPUTS__")).unwrap();
        for input in inputs {
            let mut user_context=UserContext::new();user_context.disable_sql_log();
            let executor=Executor(Mutex::new(vec![]));
            let first=MutationIntent::new("first reason").unwrap();
            let next=MutationIntent::new(if input["replacement"]=="different" {"next reason"} else {"first reason"}).unwrap();
            let privacy=if input["privacy"]=="none" {None} else {
                let mut value=SqlIntentRedactions::default();
                if input["privacy"]=="nonempty" {value.capture_target_id(&Value::U64(7));}
                Some(Arc::new(value))
            };
            let mut command=InsertCommand::new("Payment");command.values.insert("id".into(),Value::U64(7));
            let rows=if input["layer"]=="base" {
                let metadata=UserContextMetadata{context:&user_context};
                let mut service=RuntimeDataService::new(&metadata,&executor);
                service.mutation_privacy=privacy;
                service=service.with_mutation_intent(first);
                if input["replacement"]!="none" {service=service.with_mutation_intent(next);}
                wait(service.insert(&command)).unwrap()
            } else {
                let mut service=ContextDataService{metadata:UserContextMetadata{context:&user_context},
                    executor:&executor,mutation_intent:None,mutation_privacy:privacy};
                service=service.with_mutation_intent(first);
                if input["replacement"]!="none" {service=service.with_mutation_intent(next);}
                wait(service.insert(&command)).unwrap()
            };
            assert_eq!(rows,1);let requests=executor.0.lock().unwrap();assert_eq!(requests.len(),1);
            println!("TEAQL_INTENT_PRIVACY_JSON {}",requests[0]);
        }
    }
}
