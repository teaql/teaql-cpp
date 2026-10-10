//! Original private policy-plan projections; fixtures construct source models only.
use serde_json::{json,Value as J};
use std::{collections::BTreeMap,io::{self,BufRead}};
use teaql_core::{Value,CompactRow};
use teaql_runtime::{UserContext,EntityKey,EntityRuntimeState,GraphNode,GraphMutationPlan,GraphMutationKind,MutationPlan,__teaql_cpp_graph_policy_plan,__teaql_cpp_ledger_policy_plan};
mod fixture_value;
fn record(j:&J)->BTreeMap<String,Value>{j.as_object().unwrap().iter().map(|(k,v)|(k.clone(),fixture_value::value(v))).collect()}
fn key(j:&J)->EntityKey{EntityKey::new(j["entity"].as_str().unwrap(),fixture_value::value(&j["id"]))}
fn output(plan:&MutationPlan)->J{json!({"execution":plan.execution_id,"request":plan.request_key,"root":plan.root_entity_type,"reason":plan.audit_reason,"operations":plan.operations.iter().map(|o|json!({"kind":format!("{:?}",o.kind),"key":format!("{:?}",o.entity),"version":o.original_version,"values":o.changed_values.iter().map(|(k,v)|(k.clone(),format!("{v:?}"))).collect::<BTreeMap<_,_>>()})).collect::<Vec<_>>()})}
fn evaluate(j:J)->J{
 let c=UserContext::default().with_trace_id(j["trace"].as_str().unwrap());
 let mut node=GraphNode::new(j["entity"].as_str().unwrap());if let Some(s)=j["comment"].as_str(){node.set_comment(s)}
 if j["mode"]=="graph"{
  let mut graph=GraphMutationPlan::default();if j["root"]==true{graph.planned_root=Some(node)}
  for item in j["items"].as_array().unwrap(){graph.push(item["entity"].as_str().unwrap(),[GraphMutationKind::Create,GraphMutationKind::Update,GraphMutationKind::Delete,GraphMutationKind::Reference][item["kind"].as_u64().unwrap() as usize],record(&item["values"]).into(),item["fields"].as_array().unwrap().iter().map(|v|v.as_str().unwrap().into()).collect(),None,item["old"].as_object().map(|_|record(&item["old"]).into()));}
  match __teaql_cpp_graph_policy_plan(&c,&graph){Ok(p)=>output(&p),Err(e)=>json!({"error":e.to_string()})}
 }else{
  let mut state=EntityRuntimeState::default();if j["loaded"].is_object(){let row=CompactRow::from_map(record(&j["loaded"]));state.set_original_compact_row(j["entity"].as_str().unwrap(),row)}
  for seed in j["seed"].as_array().unwrap(){let k=key(seed);if seed["new"]==true{state.mark_as_new(k.clone())}if seed["delete"]==true{state.mark_as_delete(k.clone())}if let Some(v)=seed["version"].as_i64(){state.set_original_version(k.clone(),v)}for(kf,v)in record(&seed["values"]){state.set(k.clone(),kf,v)}}
  if let Some(s)=j["ledger_comment"].as_str(){state.set_comment(s)}
  let p=__teaql_cpp_ledger_policy_plan(&c,&node,&state);output(&p)
 }
}
fn main(){unsafe{std::env::set_var("TEAQL_LOG_ENDPOINT","off");}for line in io::stdin().lock().lines(){println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap()));}}
