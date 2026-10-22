#[cfg(test)]
mod cpp_complete_enhancement_observer {
use std::{collections::BTreeMap,io::{self,BufRead},sync::{Arc,Mutex},future::Future,pin::Pin,task::{Context,Poll,Wake,Waker}};
use serde_json::{Value as J,json};use teaql_core::*;use teaql_data_service::*;
use crate::{RuntimeModule,DataServiceError,EntityDataService};
use crate::{fixture_value, fixture_sql};
struct Mock<'a> { state: &'a State, input: &'a J }

impl DataServiceExecutor for Mock<'_>{type Error=io::Error;fn capabilities(&self)->DataServiceCapabilities{*self.state.capabilities.lock().unwrap()+=1;DataServiceCapabilities::default()}}
impl QueryExecutor for Mock<'_> {
 async fn query(&self, request: QueryRequest) -> Result<QueryResult, Self::Error> {
  self.state.calls.lock().unwrap().push(json!({"query":query_out(&request.query),
      "trace":trace_out(&request.trace_chain),"comment":request.comment(),"purpose":request.purpose()}));
  let index=self.state.calls.lock().unwrap().len();
  let _drop=DropWork(self.state);
  Delay(self.input["pending"].as_u64().unwrap()).await;
  if index==self.input["fail_at"].as_u64().unwrap() as usize { return Err(io::Error::other("fetch failed")); }
  let rows=items(self.input,"responses").get(index-1).map(|r|rows(r.as_array().unwrap())).unwrap_or_default();
  let count=rows.len();
  Ok(QueryResult{rows,metadata:ExecutionMetadata::unrecorded_query(count)})
 }
}
impl MutationExecutor for Mock<'_>{async fn mutate(&self,_:MutationRequest)->Result<MutationResult,Self::Error>{panic!("unused")}}
#[derive(Default)]struct State{calls:Mutex<Vec<J>>,drops:Mutex<usize>,capabilities:Mutex<usize>}
struct Delay(u64);impl Future for Delay{type Output=();fn poll(mut self:Pin<&mut Self>,cx:&mut Context<'_>)->Poll<()>{if self.0>0{self.0-=1;cx.waker().wake_by_ref();Poll::Pending}else{Poll::Ready(())}}}
struct DropWork<'a>(&'a State);impl Drop for DropWork<'_>{fn drop(&mut self){*self.0.drops.lock().unwrap()+=1;}}
mod locked{use super::*;
 pub async fn run(repo:&EntityDataService<'_,Mock<'_>>,parents:&mut [CompactRow],j:&J)->Result<(),DataServiceError<io::Error>>{
  let trace=vec![TraceNode::typed(TraceKind::Request,"Parent",None,"parent trace")];
  match j["mode"].as_str().unwrap(){
   "aggregate"=>{let inputs=items(j,"enhancements").iter().map(|a|RelationAggregate{relation_name:a["relation"].as_str().unwrap().into(),alias:a["alias"].as_str().unwrap().into(),query:query_in(&a["query"]),single_result:a["single"]==true}).collect::<Vec<_>>();{let keys=repo.relation_aggregate_key_rows(parents,&inputs).map_err(DataServiceError::Runtime)?;repo.enhance_relation_aggregates_internal(parents,&keys,&inputs,cache_in(j),&trace).await}},
   "group"=>{let inputs=items(j,"enhancements").iter().map(|a|ObjectGroupBy{property_name:a["property"].as_str().unwrap().into(),storage_field:a["storage"].as_str().unwrap().into(),query:query_in(&a["query"])}).collect::<Vec<_>>();repo.enhance_object_group_bys_internal(parents,&inputs,&trace).await},
   _=>{let inputs=items(j,"enhancements").iter().map(|a|query_in(&a["query"])).collect::<Vec<_>>();repo.enhance_child_queries_internal(parents,&inputs,&trace).await}
  }
 }
}
fn items<'a>(j:&'a J,k:&str)->&'a [J]{j[k].as_array().map(Vec::as_slice).unwrap_or(&[])}
fn rows(j:&[J])->Vec<CompactRow>{j.iter().map(|r|CompactRow::from_map(r.as_object().unwrap().iter().map(|(k,v)|(k.clone(),fixture_value::value(v))).collect())).collect()}
fn rows_out(rows:&[CompactRow])->J{json!(rows.iter().map(|r|r.clone().into_map().iter().map(|(k,v)|(k.clone(),format!("{v:?}"))).collect::<BTreeMap<_,_>>()).collect::<Vec<_>>())}
fn cache_in(j:&J)->Option<AggregationCacheOptions>{j.get("cache").filter(|c|!c.is_null()).map(|c|AggregationCacheOptions{enabled:c["enabled"]==true,cache_expired_millis:c["ttl"].as_u64().unwrap(),propagate:c["propagate"]==true,propagate_cache_expired_millis:c["child_ttl"].as_u64().unwrap()})}
fn query_in(j:&J)->SelectQuery{let mut q=fixture_sql::query(j);q.entity=j["entity"].as_str().unwrap().into();q.purpose=j["purpose"].as_str().map(str::to_owned);q.aggregation_cache=cache_in(j);q.relations=items(j,"relations").iter().map(|l|RelationLoad{name:l["name"].as_str().unwrap().into(),query:l.get("query").map(|q|Box::new(query_in(q)))}).collect();q.trace_chain=vec![TraceNode::typed(TraceKind::Request,q.entity.clone(),None,"query trace")];q}
fn expr_out(e:&Expr)->J{let (kind,op,name,value,args):(u8,u8,String,Value,Vec<&Expr>)=match e{Expr::Column(s)=>(0,0,s.clone(),Value::Null,vec![]),Expr::Value(v)=>(1,0,String::new(),v.clone(),vec![]),Expr::Binary{left,op,right}=>(3,*op as u8,String::new(),Value::Null,vec![left,right]),Expr::And(a)=>(8,0,String::new(),Value::Null,a.iter().collect()),Expr::Or(a)=>(9,0,String::new(),Value::Null,a.iter().collect()),_=>panic!("unexpected fixture expression")};json!({"kind":kind,"op":op,"name":name,"value":format!("{value:?}"),"args":args.into_iter().map(expr_out).collect::<Vec<_>>()})}
fn trace_out(trace:&[TraceNode])->J{json!(trace.iter().map(|t|json!({"kind":t.kind as u8,"entity":t.entity_type,"comment":t.comment})).collect::<Vec<_>>())}
fn query_out(q:&SelectQuery)->J{
 json!({"entity":q.entity,"projection":q.projection,"group":q.group_by,"filter":q.filter.as_ref().map(expr_out),"having":q.having.as_ref().map(expr_out),"partition":q.partition_by,"comment":q.comment,"search":q.search_with_text,"raw_sql":q.raw_sql,"raw_criteria":q.raw_sql_search_criteria,"trace":trace_out(&q.trace_chain),"slice":q.slice.map(|s|json!({"offset":s.offset,"limit":s.limit})),"cache":q.aggregation_cache.map(|c|json!({"enabled":c.enabled,"ttl":c.cache_expired_millis,"propagate":c.propagate,"child_ttl":c.propagate_cache_expired_millis})),
 "aggregates":q.aggregates.iter().map(|a|json!({"function":a.function as u8,"field":a.field,"alias":a.alias})).collect::<Vec<_>>(),"expr_projection":q.expr_projection.iter().map(|p|json!({"alias":p.alias,"expr":expr_out(&p.expr)})).collect::<Vec<_>>(),"order":q.order_by.iter().map(|o|json!({"field":o.field,"direction":o.direction as u8})).collect::<Vec<_>>(),"relations":q.relations.iter().map(|l|json!({"name":l.name,"query":l.query.as_ref().map(|q|query_out(q))})).collect::<Vec<_>>(),"raw_projection":q.raw_projections.iter().map(|p|json!({"alias":p.property_name,"sql":p.raw_sql_segment})).collect::<Vec<_>>(),"dynamic":q.dynamic_properties.iter().map(|p|json!({"alias":p.property_name,"sql":p.raw_sql_segment})).collect::<Vec<_>>()})
}
struct Noop;impl Wake for Noop{fn wake(self:Arc<Self>){}}
fn drive(f:impl Future<Output=Result<(),DataServiceError<io::Error>>>,polls:u64)->J{let mut f=Box::pin(f);let waker=Waker::from(Arc::new(Noop));let mut cx=Context::from_waker(&waker);for _ in 0..polls{if let Poll::Ready(r)=f.as_mut().poll(&mut cx){return match r{Ok(())=>json!({}),Err(e)=>json!({"error":e.to_string()})}}}json!({"cancelled":true})}
fn evaluate(j:&J)->J{
 let mut module=RuntimeModule::new();for input in items(j,"entities"){let mut e=fixture_sql::entity(input);for rel in items(input,"relations"){let mut r=RelationDescriptor::new(rel["name"].as_str().unwrap(),rel["target"].as_str().unwrap());r.local_key=rel["local"].as_str().unwrap().into();r.foreign_key=rel["foreign"].as_str().unwrap().into();r.many=true;e.relations.push(r)}module=module.descriptor(e)}let context=module.into_context();let state=State::default();let executor=Mock{state:&state,input:j};
 let repo=EntityDataService::for_executor(&context,"Parent",&executor)
 .with_trace_context(vec![TraceNode::typed(TraceKind::Entity,"ignored repository",None,"repo trace")]);let mut parents=rows(items(j,"parents"));let mut out=drive(locked::run(&repo,&mut parents,j),j["polls"].as_u64().unwrap());out["parents"]=rows_out(&parents);out["calls"]=json!(*state.calls.lock().unwrap());out["drops"]=json!(*state.drops.lock().unwrap());out["capabilities"]=json!(*state.capabilities.lock().unwrap());out
}
#[test]
fn complete_enhancement_inputs() {
 let inputs: Vec<J> = serde_json::from_str(include_str!("__ENHANCEMENT_INPUTS__")).unwrap();
 for (index,input) in inputs.iter().enumerate() {
  println!("COMPLETE_ENHANCEMENT_OBSERVATION {}",json!({"case":index,"result":evaluate(input)}));
 }
 assert!(inputs.len() == 377 || inputs.len() == 754);
}
}
