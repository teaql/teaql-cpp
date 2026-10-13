use std::{collections::BTreeMap,io::{self,BufRead},sync::{Arc,Mutex},future::Future,pin::Pin,task::{Context,Poll,Wake,Waker}};
use serde_json::{Value as J,json};use teaql_core::*;
pub use teaql_runtime::{UserContext,RuntimeModule,RuntimeOperation,RuntimeAttributeValue,DataServiceError,RelationLoadPlan};
use teaql_data_service::*;
struct Mock{probes:bool,calls:Mutex<usize>}
impl DataServiceExecutor for Mock{type Error=io::Error;fn capabilities(&self)->DataServiceCapabilities{*self.calls.lock().unwrap()+=1;DataServiceCapabilities{small_parent_relation_probes:self.probes,..Default::default()}}}
impl QueryExecutor for Mock{async fn query(&self,_:QueryRequest)->Result<QueryResult,Self::Error>{panic!("relation planner bypassed child query entry point")}}
impl MutationExecutor for Mock{async fn mutate(&self,_:MutationRequest)->Result<MutationResult,Self::Error>{panic!("unused")}}
struct Metadata<'a>{context:&'a UserContext,capture:bool}
impl Metadata<'_>{fn entity(&self,name:&str)->Option<&EntityDescriptor>{self.context.entity(name)}fn capture_execution_metadata(&self)->bool{self.capture}}
struct ContextDataService<'a,E>{metadata:Metadata<'a>,executor:&'a E}
#[derive(Default)]struct State{queries:Mutex<Vec<SelectQuery>>,drops:Mutex<usize>}
struct EntityDataService<'a,E:DataServiceExecutor>{data_service:ContextDataService<'a,E>,state:&'a State,pending:u64,fail_at:u64,error:fn(&str)->E::Error}
struct Delay(u64);impl Future for Delay{type Output=();fn poll(mut self:Pin<&mut Self>,cx:&mut Context<'_>)->Poll<()>{if self.0>0{self.0-=1;cx.waker().wake_by_ref();Poll::Pending}else{Poll::Ready(())}}}
struct DropWork<'a>(&'a State);impl Drop for DropWork<'_>{fn drop(&mut self){*self.0.drops.lock().unwrap()+=1;}}
impl<E:DataServiceExecutor> EntityDataService<'_,E>{async fn fetch_compact_all_internal(&self,query:SelectQuery)->Result<Vec<CompactRow>,DataServiceError<E::Error>>{
 self.state.queries.lock().unwrap().push(query);let index=self.state.queries.lock().unwrap().len() as u64;let _drop=DropWork(self.state);Delay(self.pending).await;
 if index==self.fail_at{return Err(DataServiceError::Executor((self.error)("child failed")))}Ok(vec![CompactRow::from_map([("probe".into(),Value::U64(index))].into())])
}}
mod locked{
 use super::*;
 // Original methods retain their generic associated error. The fixture bridge
 // below supplies that same error without changing their control flow.
 include!(concat!(env!("OUT_DIR"),"/relation_execution.rs"));
 pub async fn fetch(repo:&EntityDataService<'_,Mock>,plan:&RelationLoadPlan,parents:&[CompactRow],compact:bool)->Result<Vec<CompactRow>,DataServiceError<io::Error>>{{assert!(plan.query.as_ref().and_then(|q|q.dynamic_field_selection.as_ref()).is_none());repo.fetch_native_relation_rows(repo,plan,parents,compact).await}}
 pub fn telemetry(plan:&RelationLoadPlan,probes:bool,parents:usize)->RuntimeOperation{let caps=DataServiceCapabilities{small_parent_relation_probes:probes,..Default::default()};relation_top_n_operation(plan,relation_top_n_execution_plan(&caps,plan,parents),parents)}
}
fn expr_out(e:&Expr)->J{let (kind,op,name,value,args):(u8,u8,String,Value,Vec<&Expr>)=match e{Expr::Column(s)=>(0,0,s.clone(),Value::Null,vec![]),Expr::Value(v)=>(1,0,String::new(),v.clone(),vec![]),Expr::Binary{left,op,right}=>(3,*op as u8,String::new(),Value::Null,vec![left,right]),Expr::And(a)=>(8,0,String::new(),Value::Null,a.iter().collect()),Expr::Or(a)=>(9,0,String::new(),Value::Null,a.iter().collect()),_=>panic!("unexpected fixture expression")};json!({"kind":kind,"op":op,"name":name,"value":format!("{value:?}"),"args":args.into_iter().map(expr_out).collect::<Vec<_>>()})}
fn query_out(q:&SelectQuery)->J{json!({"entity":q.entity,"filter":q.filter.as_ref().map(expr_out),"partition":q.partition_by,"projection":q.projection,"relations":q.relations.iter().map(|l|l.name.clone()).collect::<Vec<_>>(),"order":q.order_by.iter().map(|o|json!({"field":o.field,"direction":o.direction as u8})).collect::<Vec<_>>()})}
struct Noop;impl Wake for Noop{fn wake(self:Arc<Self>){}}
fn drive(f:impl Future<Output=Result<Vec<CompactRow>,DataServiceError<io::Error>>>,polls:u64)->J{let mut f=Box::pin(f);let waker=Waker::from(Arc::new(Noop));let mut cx=Context::from_waker(&waker);for _ in 0..polls{if let Poll::Ready(r)=f.as_mut().poll(&mut cx){return match r{Ok(rows)=>json!({"rows":rows.iter().map(|r|r.get("probe").unwrap().try_u64().unwrap()).collect::<Vec<_>>()}),Err(e)=>json!({"error":e.to_string()})}}}json!({"cancelled":true})}
fn evaluate(j:&J)->J{
 let mut target=EntityDescriptor::new("Child");let mut id=PropertyDescriptor::new("child_id",DataType::U64);id.is_id=true;target.properties.push(id);let context=RuntimeModule::new().descriptor(target).into_context();let state=State::default();let mock=Mock{probes:j["probes"]==true,calls:Mutex::new(0)};let repo=EntityDataService{data_service:ContextDataService{metadata:Metadata{context:&context,capture:j["capture"]==true},executor:&mock},state:&state,pending:j["pending"].as_u64().unwrap(),fail_at:j["fail_at"].as_u64().unwrap(),error:|s|io::Error::other(s.to_owned())};
 let query=if j["query"]==true{let mut q=SelectQuery::new("IgnoredTarget").project("name").order_desc("name");q.partition_by=Some("existing".into());q.relations.push(RelationLoad::new("nested"));match j["slice"].as_str().unwrap(){"limit"=>q=q.limit(2),"offset"=>q=q.offset(3),"zero"=>q=q.limit(0),_=>{}}q.top_n_probe_parent_threshold=j["threshold"].as_u64().map(|n|n as usize);Some(q)}else{None};
 let mut plan=RelationLoadPlan{parent_entity:"Root".into(),relation_name:"children".into(),path:"nested.children".into(),target_entity:"Child".into(),local_key:"id".into(),foreign_key:"owner_id".into(),many:j["many"]==true,query,children:vec![]};plan.children.push(RelationLoadPlan{parent_entity:"Child".into(),relation_name:"nested".into(),path:"nested".into(),target_entity:"Leaf".into(),local_key:"ref_id".into(),foreign_key:"id".into(),many:false,query:None,children:vec![]});
 let count=j["parents"].as_u64().unwrap();let mut parents=vec![];for i in 0..count{let row=CompactRow::from_map([("id".into(),Value::U64(i))].into());parents.push(row.clone());parents.push(row);}parents.push(CompactRow::from_map(BTreeMap::new()));
 let telemetry=locked::telemetry(&plan,j["probes"]==true,count as usize);let mut out=drive(locked::fetch(&repo,&plan,&parents,j["compact"]==true),j["polls"].as_u64().unwrap());out["queries"]=json!(state.queries.lock().unwrap().iter().map(query_out).collect::<Vec<_>>());out["capabilities_calls"]=json!(*mock.calls.lock().unwrap());out["drops"]=json!(*state.drops.lock().unwrap());out["telemetry"]=json!(telemetry.attributes.iter().map(|(k,v)|(k.clone(),match v{RuntimeAttributeValue::String(s)=>json!(s),RuntimeAttributeValue::Integer(n)=>json!(n),_=>panic!("unexpected attribute")})).collect::<BTreeMap<_,_>>());out
}
fn main(){for line in io::stdin().lock().lines(){println!("{}",evaluate(&serde_json::from_str::<J>(&line.unwrap()).unwrap()));}}
