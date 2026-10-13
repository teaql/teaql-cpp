//! Original graph execution/save/read bodies; fixtures implement provider and sink SPIs only.
use std::{collections::BTreeMap,io::{self,BufRead},sync::{Arc,Mutex,atomic::{AtomicU64,Ordering}},future::Future,task::{Context,Poll,Wake,Waker}};
use serde_json::{json,Value as J};
use teaql_core::*;use teaql_data_service::*;
use teaql_runtime::{UserContext,RuntimeModule,RuntimeError,RawAuditEventSink,RawAuditEvent,InternalIdGenerator,GraphNode,GraphOperation,GraphMutationPlan,GraphMutationKind,TraceScopeToken,__cpp_execute_graph,__cpp_save_graph,__cpp_current_row,__cpp_save_entity_adapter,__cpp_save_core_graph,__cpp_reference_node,__cpp_remove_node,__cpp_update_node,__cpp_delete_node,__cpp_recursive_graph};
#[path = "../../../rust_values/src/fixture_value.rs"]
mod fixture_value;
#[derive(Default)]struct State{notes:Mutex<Vec<String>>,requests:Mutex<Vec<J>>,events:Mutex<Vec<J>>,queries:Mutex<Vec<J>>}
impl State{fn note(&self,v:&str){self.notes.lock().unwrap().push(v.into())}}
fn record(j:&J)->BTreeMap<String,Value>{j.as_object().unwrap().iter().map(|(k,v)|(k.clone(),fixture_value::value(v))).collect()}
fn record_out(v:&BTreeMap<String,Value>)->J{json!(v.iter().map(|(k,v)|(k.clone(),format!("{v:?}"))).collect::<BTreeMap<_,_>>())}
fn trace(j:&J)->Vec<TraceNode>{j.as_array().unwrap().iter().map(|n|TraceNode{kind:[TraceKind::AuditReason,TraceKind::Entity,TraceKind::Comment][n["kind"].as_u64().unwrap()as usize],entity_type:n["entity"].as_str().unwrap().into(),entity_id:None,comment:n["comment"].as_str().unwrap().into()}).collect()}
fn trace_out(v:&[TraceNode])->J{json!(v.iter().map(|n|json!({"kind":n.kind as u8,"entity":n.entity_type,"comment":n.comment})).collect::<Vec<_>>())}
fn request_out(r:&MutationCommand)->J{match r{
 MutationCommand::Insert(c)=>json!({"kind":"insert","entity":c.entity,"values":record_out(&c.values),"trace":trace_out(&c.trace_chain)}),
 MutationCommand::Update(c)=>json!({"kind":"update","entity":c.entity,"id":format!("{:?}",c.id),"version":c.expected_version,"values":record_out(&c.values),"old":c.old_values.as_ref().map(|s|record_out(s)),"trace":trace_out(&c.trace_chain)}),
 MutationCommand::Delete(c)=>json!({"kind":"delete","entity":c.entity,"id":format!("{:?}",c.id),"version":c.expected_version,"trace":trace_out(&c.trace_chain)}),
 MutationCommand::Recover(c)=>json!({"kind":"recover","entity":c.entity,"id":format!("{:?}",c.id),"version":c.expected_version,"trace":trace_out(&c.trace_chain)}),_=>panic!("unexpected batch request")}}
fn node(j:&J)->GraphNode{
 let mut out=GraphNode::new(j["entity"].as_str().unwrap()).operation([GraphOperation::Upsert,GraphOperation::Create,GraphOperation::Reference,GraphOperation::Remove][j["operation"].as_u64().unwrap() as usize]);out.values=record(&j["values"]).into();if let Some(s)=j["comment"].as_str(){out.set_comment(s)}out.dirty_fields=j["dirty"].as_array().map(|v|v.iter().map(|s|s.as_str().unwrap().to_owned()).collect());out.original_values=if j["old"].is_object(){Some(record(&j["old"]).into())}else{None};for(k,v)in j["relations"].as_object().unwrap(){out.relations.insert(k.clone(),v.as_array().unwrap().iter().map(node).collect());}out
}
fn node_out(n:&GraphNode)->J{json!({"entity":n.entity,"values":record_out(&n.values),"operation":n.operation as u8,"comment":n.comment,"relations":n.relations.iter().map(|(k,v)|(k.clone(),v.iter().map(node_out).collect::<Vec<_>>())).collect::<BTreeMap<_,_>>()})}
fn plan(j:&J)->GraphMutationPlan{
 let mut p=GraphMutationPlan::default();if j["root"].is_object(){p.planned_root=Some(node(&j["root"]));}
 for item in j["items"].as_array().unwrap(){let mut token=None;for frame in item["token"].as_array().unwrap(){token=Some(Arc::new(TraceScopeToken{parent:token,track:TraceNode::typed(TraceKind::AuditReason,frame["entity"].as_str().unwrap(),None,frame["comment"].as_str().unwrap()),node_index:frame["index"].as_u64().unwrap()}));}
  p.push(item["entity"].as_str().unwrap(),[GraphMutationKind::Create,GraphMutationKind::Update,GraphMutationKind::Delete,GraphMutationKind::Reference][item["kind"].as_u64().unwrap()as usize],record(&item["values"]).into(),item["fields"].as_array().unwrap().iter().map(|s|s.as_str().unwrap().into()).collect(),token,item["old"].as_object().map(|_|record(&item["old"]).into()));}
 p.rebuild_batches();p
}
struct Mock{state:Arc<State>,input:J}
impl DataServiceExecutor for Mock{type Error=io::Error;fn capabilities(&self)->DataServiceCapabilities{DataServiceCapabilities::default()}}
impl QueryExecutor for Mock{async fn query(&self,q:QueryRequest)->Result<QueryResult,Self::Error>{let query_index=self.state.queries.lock().unwrap().len();self.state.note("query");self.state.queries.lock().unwrap().push(json!({"entity":q.query.entity,"filter":q.query.filter.as_ref().map(|e|format!("{e:?}")),"trace":trace_out(&q.query.trace_chain),"comment":q.query.comment}));if self.input["read_fail"]==true{return Err(io::Error::other("read failed"))}let rows=self.input["read_batches"].as_array().and_then(|b|b.get(query_index)).unwrap_or(&self.input["rows"]);Ok(QueryResult{rows:rows.as_array().unwrap().iter().map(|j|CompactRow::from_map(record(j))).collect(),metadata:ExecutionMetadata::unrecorded_query(rows.as_array().unwrap().len())})}}
impl MutationExecutor for Mock{async fn mutate(&self,q:MutationRequest)->Result<MutationResult,Self::Error>{let index=self.state.requests.lock().unwrap().len();self.state.note("provider");self.state.requests.lock().unwrap().push(request_out(&q.command));if self.input["fail"].as_i64().unwrap()==index as i64{return Err(io::Error::other("provider failed"))}let count=self.input["counts"].as_array().unwrap().get(index).and_then(J::as_u64).unwrap_or(1);let mut metadata=ExecutionMetadata::unrecorded_query(0);metadata.operation=DataServiceOperation::Update;metadata.affected_rows=Some(count);Ok(MutationResult{affected_rows:count,generated_values:GeneratedValues::new(),persisted_snapshot:None,metadata})}}
struct Sink{state:Arc<State>,fail:i64}
impl RawAuditEventSink for Sink{fn on_event(&self,_:&UserContext,e:&RawAuditEvent)->Result<(),RuntimeError>{let index=self.state.events.lock().unwrap().len();self.state.note("event");self.state.events.lock().unwrap().push(json!({"kind":format!("{:?}",e.kind),"entity":e.entity,"values":record_out(&e.values),"old":e.old_values.as_ref().map(record_out),"new":e.new_values.as_ref().map(record_out),"fields":e.updated_fields,"trace":trace_out(&e.trace_chain),"governance":e.mutation_governance.as_ref().map(|s|json!({"source":format!("{:?}",s.source),"request":s.request_key,"warnings":s.warning_codes}))}));if index as i64==self.fail{Err(RuntimeError::Event("event failed".into()))}else{Ok(())}}}
struct Id{next:AtomicU64}impl InternalIdGenerator for Id{fn generate_id(&self,_:&str)->Result<u64,RuntimeError>{Ok(self.next.fetch_add(1,Ordering::Relaxed))}}
struct NoWake;impl Wake for NoWake{fn wake(self:Arc<Self>){}}fn wait<F:Future>(f:F)->F::Output{let mut f=std::pin::pin!(f);let w=Waker::from(Arc::new(NoWake));let mut cx=Context::from_waker(&w);loop{if let Poll::Ready(v)=f.as_mut().poll(&mut cx){return v}}}
struct AdapterModel {
 values:MutationValues, dirty:Option<std::collections::BTreeSet<String>>,
 old:Option<EntitySnapshot>, comment:Option<String>, deleted:bool, state:Arc<State>,
}
impl TeaqlEntity for AdapterModel {
 const ENTITY_NAME:&'static str="Order";
 fn entity_descriptor()->EntityDescriptor{EntityDescriptor::new("Order")}
}
impl Entity for AdapterModel {
 fn from_compact_row(_:CompactRow)->Result<Self,EntityError>{panic!("save adapter attempted typed hydration")}
 fn dirty_fields(&self)->Option<std::collections::BTreeSet<String>>{self.state.note("dirty");self.dirty.clone()}
 fn original_values(&self)->Option<EntitySnapshot>{self.state.note("original");self.old.clone()}
 fn is_marked_as_delete(&self)->bool{self.state.note("deleted");self.deleted}
 fn get_comment(&self)->Option<String>{self.state.note("comment");self.comment.clone()}
 fn is_new(&self)->bool{panic!("save adapter called is_new")}
 fn is_field_loaded(&self,_:&str)->bool{panic!("save adapter called is_field_loaded")}
 fn into_values(self)->MutationValues{self.state.note("values");self.values}
}
fn core_node(j:&J)->EntityGraphNode {
 EntityGraphNode{entity_type:j["entity"].as_str().unwrap().into(),values:record(&j["values"]).into(),comment:j["comment"].as_str().map(str::to_owned),operation:if j["operation"]==0{EntityGraphOperation::Save}else{EntityGraphOperation::Delete},children:j["children"].as_array().unwrap().iter().map(|c|(c["name"].as_str().unwrap().into(),core_node(&c["node"]))).collect()}
}
fn adapter_work(c:&UserContext,provider:&Mock,state:Arc<State>,j:&J)->Result<GraphNode,teaql_runtime::DataServiceError<io::Error>> {
 let service=j["service"].as_str().unwrap();let mode=j["mode"].as_str().unwrap();
 if mode=="adapter_core" {return wait(__cpp_save_core_graph(c,provider,service,EntityGraph{root:core_node(&j["core"])}));}
 let model=AdapterModel{values:record(&j["root"]["values"]).into(),dirty:j["dirty"].as_array().map(|v|v.iter().map(|s|s.as_str().unwrap().into()).collect()),old:if j["old"].is_null(){None}else{Some(record(&j["old"]).into())},comment:j["root"]["comment"].as_str().map(str::to_owned),deleted:j["deleted"].as_bool().unwrap(),state};
 let status=[teaql_runtime::EntityStatus::New,teaql_runtime::EntityStatus::Persisted,teaql_runtime::EntityStatus::PersistedDeleted,teaql_runtime::EntityStatus::Updated,teaql_runtime::EntityStatus::UpdatedDeleted,teaql_runtime::EntityStatus::UpdatedRecover,teaql_runtime::EntityStatus::Refer][j["status"].as_u64().unwrap()as usize];
 wait(__cpp_save_entity_adapter(c,provider,service,model,status,j["annotation"].as_str().unwrap().into(),mode))
}
fn node_command(c:&UserContext,provider:&Mock,j:&J)->J {
 let service=j["service"].as_str().unwrap();let mut graph=node(&j["root"]);let lineage=trace(&j["trace"]);
 if j["mode"].as_str().unwrap().starts_with("recursive_") {
 fn run(c:&UserContext,p:&Mock,service:&str,node:GraphNode,frames:&[TraceNode],parent:Option<&teaql_runtime::ScopedCommentNode<'_>>,insert:bool)->J {
  if let Some((head,tail))=frames.split_first(){let scope=teaql_runtime::ScopedCommentNode{parent,track:head.clone()};return run(c,p,service,node,tail,Some(&scope),insert);}
  match wait(__cpp_recursive_graph(c,p,service,node,parent,insert)){Ok(n)=>json!({"raw":format!("{n:?}")}),Err(e)=>json!({"error":e.to_string()})}
 }
 return run(c,provider,service,graph,&lineage,None,j["mode"]=="recursive_insert");
 }
 match j["mode"].as_str().unwrap(){
 "node_reference"=>match wait(__cpp_reference_node(c,provider,service,graph,lineage)){Ok(g)=>json!({"raw":format!("{g:?}")}),Err(e)=>json!({"error":e.to_string()})},
 "node_remove"=>match wait(__cpp_remove_node(c,provider,service,&graph,lineage)){Ok(())=>json!({"ok":true}),Err(e)=>json!({"error":e.to_string()})},
 "node_update"=>{let result=__cpp_update_node(c,provider,service,&mut graph,&fixture_value::value(&j["id"]));let mut out=match result{Ok(command)=>json!({"command":request_out(&MutationCommand::Update(command))}),Err(e)=>json!({"error":e.to_string()})};out["node"]=json!(format!("{graph:?}"));out},
 "node_delete"=>{
 fn run(c:&UserContext,p:&Mock,service:&str,node:&GraphNode,frames:&[TraceNode],parent:Option<&teaql_runtime::ScopedCommentNode<'_>>)->J {
  if let Some((head,tail))=frames.split_first(){let scope=teaql_runtime::ScopedCommentNode{parent,track:head.clone()};return run(c,p,service,node,tail,Some(&scope));}
  match wait(__cpp_delete_node(c,p,service,node,parent)){Ok(n)=>json!({"affected":n}),Err(e)=>json!({"error":e.to_string()})}
 }
 run(c,provider,service,&graph,&lineage,None)
 },_=>panic!("unknown node command")}
}
struct NodeChecker{state:Arc<State>,action:String}
impl teaql_runtime::Checker for NodeChecker {
 fn entity(&self)->&str{"Order"}
 fn check_and_fix(&self,_:&UserContext,values:&mut teaql_runtime::EntityValues,location:&teaql_runtime::ObjectLocation,results:&mut teaql_runtime::CheckResults){
  self.state.note(&format!("checker.{}",teaql_runtime::CheckObjectStatus::from_values(values).as_str()));
  if self.action=="fill"||self.action=="error"{values.insert("name".into(),Value::Text("fixed allocated checker payload".into()));}
  if self.action=="erase"{values.remove("name");}
  if self.action=="error"{results.push(teaql_runtime::CheckResult::required(location.clone().member("name")));}
 }
}
fn evaluate(j:J)->J{
 let state=Arc::new(State::default());let mut module=RuntimeModule::new().event_sink(Sink{state:state.clone(),fail:j["event_fail"].as_i64().unwrap()});for info in j["descriptors"].as_array().unwrap(){let mut d=EntityDescriptor::new(info["entity"].as_str().unwrap());let property=info["id"].as_str().unwrap();if !property.is_empty(){d=d.property(PropertyDescriptor::new(property,DataType::U64).id())}d=d.property(PropertyDescriptor::new("version",DataType::I64).version()).property(PropertyDescriptor::new("name",DataType::Text));for r in info["relations"].as_array().unwrap(){let mut relation=RelationDescriptor::new(r["name"].as_str().unwrap(),r["target"].as_str().unwrap());relation.many=r["many"].as_bool().unwrap_or(true);relation.attach=r["attach"].as_bool().unwrap_or(true);if let Some(local)=r["local"].as_str(){relation.local_key=local.into();}if let Some(foreign)=r["foreign"].as_str(){relation.foreign_key=foreign.into();}d=d.relation(relation)}module=module.descriptor(d);}let mut c=module.into_context();if let Some(action)=j["checker"].as_str().filter(|a|*a!="none"){c=c.with_checker_registry(teaql_runtime::InMemoryCheckerRegistry::new().with_checker(NodeChecker{state:state.clone(),action:action.into()}));}c.set_user_identifier("fixture");c.set_trace_id("fixture");c.disable_sql_log();c.set_internal_id_generator(Id{next:AtomicU64::new(100)});let provider=Mock{state:state.clone(),input:j.clone()};let service=j["service"].as_str().unwrap();let outcome=if j["mode"].as_str().unwrap().starts_with("node_")||j["mode"].as_str().unwrap().starts_with("recursive_"){node_command(&c,&provider,&j)}else if j["mode"]=="fetch"{match wait(__cpp_current_row(&c,&provider,service,j["entity"].as_str().unwrap(),j["property"].as_str().unwrap(),&fixture_value::value(&j["id"]),trace(&j["trace"]))){Ok(row)=>json!({"row":row.as_ref().map(|r|record_out(&r.clone().into_map()))}),Err(e)=>json!({"error":e.to_string()})}}else{let result=if j["mode"].as_str().unwrap().starts_with("adapter_"){adapter_work(&c,&provider,state.clone(),&j)}else if j["mode"]=="save"{wait(__cpp_save_graph(&c,&provider,service,node(&j["root"])))}else{wait(__cpp_execute_graph(&c,&provider,service,plan(&j)))};match result{Ok(n)=>{let mut out=json!({"root":node_out(&n)});if j["mode"].as_str().unwrap().starts_with("adapter_"){out["raw"]=json!(format!("{n:?}"));}out},Err(e)=>json!({"error":e.to_string()})}};
 let out=json!({"outcome":outcome,"notes":*state.notes.lock().unwrap(),"requests":*state.requests.lock().unwrap(),"queries":*state.queries.lock().unwrap(),"events":*state.events.lock().unwrap()});out
}
fn main(){unsafe{std::env::set_var("TEAQL_LOG_ENDPOINT","off");}for line in io::stdin().lock().lines(){println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap()));}}
