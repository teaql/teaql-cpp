// Appended source observer; original runtime bodies remain unchanged.
#[cfg(test)]
mod cpp_resolved_flat_predecode_observer {
use super::*;
use teaql_core::*;
use teaql_data_service::*;
use crate::{EntityRuntimeState,RuntimeModule};
use serde_json::json;
use std::{sync::{Arc,Mutex},future::Future,task::{Context,Poll,Wake,Waker}};
#[derive(Clone)]
struct Model<const K: usize> {
    row: CompactRow,
    state: Option<EntityRuntimeState>,
}
impl<const K: usize> TeaqlEntity for Model<K> {
    const ENTITY_NAME: &'static str = if K == 0 {
        "Parent"
    } else if K == 1 {
        "Child"
    } else {
        "Leaf"
    };
    fn entity_descriptor() -> EntityDescriptor {
        EntityDescriptor::new(Self::ENTITY_NAME)
    }
}
impl<const K: usize> Entity for Model<K> {
    fn from_compact_row(row: CompactRow) -> Result<Self, EntityError> {
        if row.get("fail").and_then(Value::try_bool).unwrap_or(false) {
            return Err(EntityError::new(Self::ENTITY_NAME, "model decode failed"));
        }
        Ok(Self { row, state: None })
    }
    fn into_values(self) -> MutationValues {
        self.row.into_map().into()
    }
    fn on_loaded(&mut self, context: &dyn std::any::Any) {
        if let Some(root) = context.downcast_ref::<EntityRuntimeState>() {
            let mut state = root.clone();
            state.set_original_compact_row(Self::ENTITY_NAME, self.row.clone());
            self.state = Some(state);
        }
    }
}
impl<const K: usize> IdentifiableEntity for Model<K> {
    fn id_value(&self) -> Value {
        self.row.get("id").cloned().unwrap_or(Value::Null)
    }
}

struct Executor { mode:&'static str, calls:Mutex<usize> }
impl DataServiceExecutor for Executor {type Error=std::io::Error;fn capabilities(&self)->DataServiceCapabilities {DataServiceCapabilities::default()}}
impl QueryExecutor for Executor {
 async fn query(&self,_:QueryRequest)->Result<QueryResult,Self::Error> {
  *self.calls.lock().unwrap()+=1;
  let mut row=CompactRow::from_map(BTreeMap::from([("id".into(),Value::U64(1))]));
  match self.mode {
   "null"=>{row.insert("children".into(),Value::Null);},
   "empty"=>{row.insert("children".into(),Value::List(vec![]));},
   "one"|"bad_child"=>{row.insert("children".into(),Value::List(vec![Value::Object(BTreeMap::from([("id".into(),Value::U64(10)),("fail".into(),Value::Bool(self.mode=="bad_child"))]))]));},
   _=>{}
  }
  Ok(QueryResult{rows:vec![row],metadata:ExecutionMetadata::unrecorded_query(1)})
 }
}
impl MutationExecutor for Executor {async fn mutate(&self,_:MutationRequest)->Result<MutationResult,Self::Error>{panic!("unexpected mutation")}}
struct Noop;impl Wake for Noop {fn wake(self:Arc<Self>) {}}
fn ready<T>(future:impl Future<Output=T>)->T {let mut future=Box::pin(future);let waker=Waker::from(Arc::new(Noop));let mut context=Context::from_waker(&waker);match future.as_mut().poll(&mut context) {Poll::Ready(value)=>value,Poll::Pending=>panic!("fixture suspended")}}
#[test]
fn flat_predecode() {
 let mut case=0;
 for continuous in [false,true] {for decoder in [false,true] {for mode in ["absent","null","empty","one","bad_child"] {
  let mut descriptor=EntityDescriptor::new("Parent");
  let mut id=PropertyDescriptor::new("id",DataType::U64);id.is_id=true;descriptor.properties.push(id.clone());
  descriptor.relations.push(RelationDescriptor::new("children","Child").local_key("id").foreign_key("parent_id").many());
  let mut child=EntityDescriptor::new("Child");child.properties.push(id);
  let mut module=RuntimeModule::new().entity::<Model<0>>();if decoder {module=module.entity::<Model<1>>();}
  let mut context=module.descriptor(descriptor).descriptor(child).into_context();context.disable_sql_log();
  let executor=Executor{mode,calls:Mutex::new(0)};
  let service=EntityDataService::for_executor(&context,"Parent",&executor);
  let mut query=SelectQuery::new("Parent").order_asc("id").page(0,1);
  query.comment=Some("read graph branch".into());query.purpose=Some("verify flat decoding order".into());
  if continuous {query=query.optimize_for_continuous_page_fetch();}
  let result=ready(service.fetch_enhanced_entities_internal::<Model<0>>(&query));
  let values=result.as_ref().ok().map(|list|list.data.iter().map(|value|json!({"embedded":value.row.get("children").is_some(),"relation_view":value.state.as_ref().unwrap().has_relation_view("Parent",1,"children"),"child_resolved":value.state.as_ref().unwrap().resolve_entity::<Model<1>>(10).is_some()})).collect::<Vec<_>>());
  println!("RESOLVED_FLAT_PREDECODE_OBSERVATION {}",json!({"case":case,"continuous":continuous,"decoder":decoder,"mode":mode,"values":values,"error":result.as_ref().err().map(|error|error.to_string()),"queries":*executor.calls.lock().unwrap()}));case+=1;
 }}}
 assert_eq!(case,20);
}
}
