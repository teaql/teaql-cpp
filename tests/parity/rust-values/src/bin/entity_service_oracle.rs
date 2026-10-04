//! Public EntityDataService planning plus verbatim locked private plan helpers.
use std::{collections::BTreeMap,io::{self,BufRead}};
use serde_json::{Value as J,json};
use teaql_core::*;
use teaql_runtime::{UserContext,RuntimeModule,RuntimeError,RelationLoadPlan,EntityDataService,EntityDataServiceBehavior};
use teaql_data_service::{DataServiceExecutor,DataServiceCapabilities,QueryExecutor,QueryRequest,QueryResult,MutationExecutor,MutationRequest,MutationResult};
#[path="../fixture_value.rs"]mod fixture_value;
#[allow(dead_code)]#[path="../fixture_sql.rs"]mod fixture_sql;
struct UnusedExecutor;
impl DataServiceExecutor for UnusedExecutor{type Error=std::io::Error;fn capabilities(&self)->DataServiceCapabilities{DataServiceCapabilities::default()}}
impl QueryExecutor for UnusedExecutor{async fn query(&self,_:QueryRequest)->Result<QueryResult,Self::Error>{panic!("planning performed I/O")}}
impl MutationExecutor for UnusedExecutor{async fn mutate(&self,_:MutationRequest)->Result<MutationResult,Self::Error>{panic!("planning performed I/O")}}
struct Loads(Vec<String>);impl EntityDataServiceBehavior for Loads{fn relation_loads(&self,_:&UserContext)->Vec<String>{self.0.clone()}}
mod locked {
 use super::*;
 struct Metadata<'a>{context:&'a UserContext}
 impl Metadata<'_>{fn entity(&self,name:&str)->Option<&EntityDescriptor>{self.context.entity(name)}}
 struct ContextService<'a>{metadata:Metadata<'a>}
 struct PlanningRepo<'a>{data_service:ContextService<'a>}
 impl<'a> PlanningRepo<'a>{fn scoped_data_service_internal(&self,_:String)->Self{Self{data_service:ContextService{metadata:Metadata{context:self.data_service.metadata.context}}}}}
 include!(concat!(env!("OUT_DIR"),"/relation_planning.rs"));
 pub fn plans(context:&UserContext,loads:&[String],query:Option<&SelectQuery>)->Result<Vec<RelationLoadPlan>,RuntimeError>{let repo=PlanningRepo{data_service:ContextService{metadata:Metadata{context}}};if let Some(q)=query{repo.build_relation_plans_from_loads(&q.entity,&q.relations)}else{repo.build_relation_plans("Root",loads)}}
 pub fn query(context:&UserContext,plan:&RelationLoadPlan,rows:&[CompactRow],compact:bool)->SelectQuery{let repo=PlanningRepo{data_service:ContextService{metadata:Metadata{context}}};if compact{repo.query_for_compact_plan(plan,rows)}else{repo.query_for_plan(plan,rows)}}
}
fn items<'a>(j:&'a J,key:&str)->&'a [J]{j[key].as_array().map(Vec::as_slice).unwrap_or(&[])}
fn query_in(j:&J)->SelectQuery{let mut q=fixture_sql::query(j);q.entity=j["entity"].as_str().unwrap_or("Root").into();q.top_n_probe_parent_threshold=j["threshold"].as_u64().map(|v|v as usize);q.relations=items(j,"relations").iter().map(|l|RelationLoad{name:l["name"].as_str().unwrap().into(),query:l.get("query").map(|q|Box::new(query_in(q)))}).collect();q}
fn expr_out(e:&Expr)->J{
 let (kind,op,name,value,args):(u8,u8,String,Value,Vec<&Expr>)=match e{
  Expr::Column(s)=>(0,0,s.clone(),Value::Null,vec![]),Expr::Value(v)=>(1,0,String::new(),v.clone(),vec![]),
  Expr::Binary{left,op,right}=>(3,*op as u8,String::new(),Value::Null,vec![left,right]),
  Expr::And(args)=>(8,0,String::new(),Value::Null,args.iter().collect()),Expr::Or(args)=>(9,0,String::new(),Value::Null,args.iter().collect()),
  _=>panic!("unused expression snapshot")
 };json!({"kind":kind,"op":op,"name":name,"value":format!("{value:?}"),"args":args.into_iter().map(expr_out).collect::<Vec<_>>()})
}
fn query_out(q:&SelectQuery)->J{json!({"entity":q.entity,"projection":q.projection,"filter":q.filter.as_ref().map(expr_out),"order":q.order_by.iter().map(|o|json!({"field":o.field,"direction":o.direction as u8,"expr":o.expr.as_ref().map(expr_out)})).collect::<Vec<_>>(),"relations":q.relations.iter().map(|l|json!({"name":l.name,"query":l.query.as_ref().map(|q|query_out(q))})).collect::<Vec<_>>(),"partition":q.partition_by,"slice":q.slice.map(|s|json!({"offset":s.offset,"limit":s.limit})),"comment":q.comment,"threshold":q.top_n_probe_parent_threshold})}
fn plan_out(p:&RelationLoadPlan)->J{json!({"parent":p.parent_entity,"name":p.relation_name,"path":p.path,"target":p.target_entity,"local":p.local_key,"foreign":p.foreign_key,"many":p.many,"query":p.query.as_ref().map(query_out),"children":p.children.iter().map(plan_out).collect::<Vec<_>>()})}
fn evaluate(j:&J)->J{
 let mut module=RuntimeModule::new();for item in items(j,"entities"){let mut e=fixture_sql::entity(item);for rel in items(item,"relations"){let mut p=RelationDescriptor::new(rel["name"].as_str().unwrap(),rel["target"].as_str().unwrap());p.local_key=rel["local"].as_str().unwrap().into();p.foreign_key=rel["foreign"].as_str().unwrap().into();p.many=rel["many"].as_bool().unwrap();e.relations.push(p);}module=module.descriptor(e);}
 let loads=items(j,"loads").iter().map(|v|v.as_str().unwrap().to_owned()).collect::<Vec<_>>();module=module.behavior("Root",Loads(loads.clone()));let context=module.into_context();let executor=UnusedExecutor;let service=EntityDataService::for_executor(&context,"Root",&executor);
 let rows=items(j,"rows").iter().map(|r|CompactRow::from_map(r.as_object().unwrap().iter().map(|(k,v)|(k.clone(),fixture_value::value(v))).collect::<BTreeMap<_,_>>())).collect::<Vec<_>>();let query=j.get("query").map(query_in);let plans=locked::plans(&context,&loads,query.as_ref());
 if query.is_none(){assert_eq!(plans.as_ref().map(|p|p.iter().map(plan_out).collect::<Vec<_>>()).map_err(|e|e.to_string()),service.relation_plans().as_ref().map(|p|p.iter().map(plan_out).collect::<Vec<_>>()).map_err(|e|e.to_string()));}
 let plans=match plans{Ok(plans)=>plans,Err(e)=>return json!({"error":e.to_string()})};let mut out=json!({"plans":plans.iter().map(plan_out).collect::<Vec<_>>(),"queries":plans.iter().map(|p|query_out(&locked::query(&context,p,&rows,false))).collect::<Vec<_>>(),"compact":plans.iter().map(|p|query_out(&locked::query(&context,p,&rows,true))).collect::<Vec<_>>()});
 if query.is_none(){out["lookup"]=match service.relation_query(j["find"].as_str().unwrap_or("children"),&rows){Ok(q)=>query_out(&q),Err(e)=>json!(e.to_string())};}out
}
fn main(){for line in io::stdin().lock().lines(){let j:J=serde_json::from_str(&line.unwrap()).unwrap();println!("{}",evaluate(&j));}}
