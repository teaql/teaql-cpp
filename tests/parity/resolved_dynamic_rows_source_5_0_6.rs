#[cfg(test)]
mod cpp_resolved_dynamic_rows_observer {
    use super::*;
    use crate::{UserContext,dynamic_fields::{DynamicFieldBatch,DynamicFieldsProvider}};
    use serde_json::{Value as Json,json};
    use teaql_core::*;
    use teaql_core::dynamic_fields::*;
    use teaql_data_service::*;
    use std::{collections::HashMap,io,sync::{Arc,Mutex},future::Future};

    struct State { input:Json,events:Mutex<Vec<Json>>,selection:DynamicFieldSelection }
    fn definitions(owner:&str,revision:&str)->Arc<DynamicFieldDefinitions> {
        DynamicFieldDefinitions::new(owner,revision,[("a".into(),DataType::Text),("b".into(),DataType::I64)]).unwrap()
    }
    fn selection(mode:&str)->DynamicFieldSelection {
        if mode=="all" {return DynamicFieldSelection::All;}
        let fields=match mode {
            "a"=>vec![("a".into(),DataType::Text)],"b"=>vec![("b".into(),DataType::I64)],
            "empty"=>vec![],"wrong_type"=>vec![("a".into(),DataType::I64)],
            "unknown"=>vec![("missing".into(),DataType::Text)],_=>panic!("fixture selection")
        };
        DynamicFieldSelection::fields(fields).unwrap()
    }
    fn identity(input:&Json)->Option<Value> {
        Some(match input["kind"].as_str().unwrap() {
            "u64"=>Value::U64(input["value"].as_u64().unwrap()),"missing"=>return None,
            "null"=>Value::Null,"negative"=>Value::I64(-1),"text"=>Value::Text("1".into()),
            "bool"=>Value::Bool(true),"typed_null"=>Value::TypedNull(DataType::U64),
            "f64"=>Value::F64(1.0),"i64_one"=>Value::I64(1),_=>panic!("fixture id")
        })
    }
    fn payload(input:&Json,id:u64)->HashMap<String,Value> {
        if input["batch"]=="empty_payload" || input["payload"]=="empty" {return HashMap::new();}
        if input["batch"]=="wrong_value" {return HashMap::from([("a".into(),Value::I64(7))]);}
        if input["batch"]=="unselected" || (input["payload"]=="split" && id==2) {
            return HashMap::from([("b".into(),Value::I64(4))]);
        }
        HashMap::from([("a".into(),if input["payload"]=="null" && id==1 {Value::Null}else{Value::Text(format!("new{id}"))})])
    }
    struct Provider {state:Arc<State>,base:Arc<DynamicFieldDefinitions>}
    #[async_trait::async_trait]
    impl DynamicFieldsProvider for Provider {
        async fn load_values(&self,_:&UserContext,owner:&str,ids:&[u64],selection:&DynamicFieldSelection,intent:&QueryIntent)->Result<DynamicFieldBatch,DynamicFieldError> {
            self.state.events.lock().unwrap().push(json!({"owner":owner,"ids":ids,"selection_matches":selection==&self.state.selection,"comment":intent.comment(),"purpose":intent.purpose()}));
            if self.state.input["batch"]=="provider_error" {return Err(DynamicFieldError{code:"FIXTURE_DYNAMIC_FAILED",field:owner.into()});}
            let mut rows=ids.iter().map(|id|(*id,payload(&self.state.input,*id))).collect::<HashMap<_,_>>();
            if self.state.input["batch"]=="omitted" {if let Some(id)=ids.last() {rows.remove(id);}}
            if self.state.input["batch"]=="unrequested" {rows.insert(999,payload(&self.state.input,999));}
            let definitions=match self.state.input["batch"].as_str().unwrap() {
                "wrong_owner"=>definitions("Other","v1"),"wrong_revision"=>definitions("Owner","v2"),_=>self.base.clone()
            };
            Ok(DynamicFieldBatch{definitions,rows})
        }
    }
    struct Executor;
    impl DataServiceExecutor for Executor {
        type Error=io::Error;
        fn capabilities(&self)->DataServiceCapabilities {DataServiceCapabilities::default()}
    }
    impl QueryExecutor for Executor {
        async fn query(&self,_:QueryRequest)->Result<QueryResult,Self::Error>{panic!("row helper cannot fetch root")}
    }
    impl MutationExecutor for Executor {
        async fn mutate(&self,_:MutationRequest)->Result<MutationResult,Self::Error>{panic!("row helper cannot mutate")}
    }
    fn record(fields:impl IntoIterator<Item=(String,Value)>)->Json {
        json!(fields.into_iter().map(|(name,value)|(name,format!("{value:?}"))).collect::<BTreeMap<_,_>>())
    }
    fn snapshot(rows:&[CompactRow])->Json {
        let mut rows=rows.to_vec();
        let dynamic=rows.iter_mut().map(|row|row.take_loaded_dynamic_fields()).collect::<Vec<_>>();
        let views=rows.into_iter().zip(&dynamic).map(|(row,dynamic)| {
            let fields=dynamic.as_ref().map(|values| {
                let mut codes=values.selected_codes().iter().cloned().collect::<Vec<_>>();codes.sort();
                json!({"owner":values.definitions().owner_type(),"revision":values.definitions().revision(),
                    "selected":codes,"values":record(values.values().clone())})
            });
            json!({"native":record(row.into_map()),"dynamic":fields})
        }).collect::<Vec<_>>();
        let sharing=dynamic.iter().map(|left|dynamic.iter().map(|right|match (left,right){
            (Some(left),Some(right))=>Some(Arc::ptr_eq(left.selected_codes(),right.selected_codes())),_=>None
        }).collect::<Vec<_>>()).collect::<Vec<_>>();
        json!({"rows":views,"sharing":sharing})
    }
    fn evaluate(input:&Json)->Json {
        let selected=selection(input["selection"].as_str().unwrap());
        let state=Arc::new(State{input:input.clone(),events:Mutex::new(vec![]),selection:selected.clone()});
        let base=definitions("Owner","v1");let mode=input["existing"].as_str().unwrap();
        let equal=definitions("Owner","v1");let old=definitions("Owner","v0");let other=definitions("Other","v1");
        let mut rows=input["ids"].as_array().unwrap().iter().enumerate().map(|(index,id)| {
            let mut fields=Record::from([("tag".into(),Value::U64(index as u64))]);if let Some(id)=identity(id) {fields.insert("id".into(),id);}
            let mut row=CompactRow::from_map(fields);
            if mode!="none" {
                let definitions=match mode {"equal"|"sparse"=>equal.clone(),"old_revision"=>old.clone(),"other_owner"=>other.clone(),"mixed_second_bad" if index==1=>old.clone(),_=>base.clone()};
                let values=if mode=="equal"||mode=="sparse" {HashMap::from([("b".into(),Value::I64(3))])}else{HashMap::from([("a".into(),Value::Text("old".into()))])};
                row.set_loaded_dynamic_fields(DynamicFieldValues::from_values(definitions,values).unwrap());
            }
            row
        }).collect::<Vec<_>>();
        let before=snapshot(&rows);let mut context=UserContext::new();context.disable_sql_log();
        if input["batch"]!="missing_provider" {context.set_dynamic_fields_provider(Arc::new(Provider{state:state.clone(),base}));}
        let repository=EntityDataService::for_executor(&context,"Repository",&Executor);
        let intent=QueryIntent::new("inspect dynamic rows","verify batch order").unwrap();
        let result={
            let mut future=std::pin::pin!(repository.load_dynamic_fields_into_rows(&mut rows,"Owner",&selected,&intent));
            let mut cx=std::task::Context::from_waker(std::task::Waker::noop());
            match future.as_mut().poll(&mut cx){std::task::Poll::Ready(value)=>value,std::task::Poll::Pending=>panic!("ready fixture suspended")}
        };
        let outcome=match result {Ok(())=>json!({"ok":true}),Err(error)=>json!({"error":error.to_string()})};
        json!({"before":before,"after":snapshot(&rows),"outcome":outcome,"events":*state.events.lock().unwrap()})
    }
    #[test]
    fn replay_batch_validation_merge_and_shapes() {
        let inputs:Vec<Json>=serde_json::from_str(include_str!("__TEAQL_RESOLVED_DYNAMIC_INPUTS__")).unwrap();
        for input in inputs {println!("TEAQL_RESOLVED_DYNAMIC_JSON {}",evaluate(&input));}
    }
}
