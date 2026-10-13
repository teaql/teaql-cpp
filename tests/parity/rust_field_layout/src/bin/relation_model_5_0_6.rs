//! Actual original relation plan Clone/PartialEq/Debug, including query/children.
use serde_json::{json,Value as J};
use std::io::{self,BufRead};
use teaql_runtime::RelationLoadPlan;
use teaql_core::{SelectQuery,Expr,OrderBy};
#[path="../../../rust_values/src/fixture_value.rs"]mod fixture_value;
fn formatted<T:std::fmt::Debug>(v:&T)->J {json!({"compact":format!("{v:?}"),"pretty":format!("{v:#?}"),"lower_hex":format!("{v:x?}"),"upper_hex":format!("{v:X?}"),"pretty_lower_hex":format!("{v:#x?}"),"pretty_upper_hex":format!("{v:#X?}"),"width_precision":format!("{v:>40.2?}"),"fancy":format!("{v:🧊^+18.3?}"),"zero_pad":format!("{v:+012?}")})}
fn plan(j:&J)->RelationLoadPlan {
 let query=j["query"].as_object().map(|_|{let q=&j["query"];let mut query=SelectQuery::new(q["entity"].as_str().unwrap()).and_filter(Expr::eq("name",fixture_value::value(&q["value"]))).comment(q["comment"].as_str().unwrap());query.order_by.push(OrderBy::asc("id"));if let Some(n)=q["limit"].as_str(){query=query.limit(n.parse().unwrap())}if let Some(n)=q["offset"].as_str(){query=query.offset(n.parse().unwrap())}if let Some(n)=q["threshold"].as_u64(){query=query.top_n_probe_parent_threshold(n as usize)}query});
 RelationLoadPlan{parent_entity:j["parent"].as_str().unwrap().into(),relation_name:j["name"].as_str().unwrap().into(),path:j["path"].as_str().unwrap().into(),target_entity:j["target"].as_str().unwrap().into(),local_key:j["local"].as_str().unwrap().into(),foreign_key:j["foreign"].as_str().unwrap().into(),many:j["many"].as_bool().unwrap(),query,children:j["children"].as_array().unwrap().iter().map(plan).collect()}
}
fn evaluate(j:J)->J {let value=plan(&j);let clone=value.clone();let mut other=value.clone();match j["changed"].as_str().unwrap(){"parent"=>other.parent_entity.push('!'),"name"=>other.relation_name.push('!'),"path"=>other.path.push('!'),"target"=>other.target_entity.push('!'),"local"=>other.local_key.push('!'),"foreign"=>other.foreign_key.push('!'),"many"=>other.many=!other.many,"query"=>other.query=if other.query.is_some(){None}else{Some(SelectQuery::new("Other"))},"children"=>other.children.push(RelationLoadPlan{parent_entity:"Child".into(),relation_name:"empty".into(),path:"empty".into(),target_entity:"Other".into(),local_key:"id".into(),foreign_key:"owner_id".into(),many:false,query:None,children:vec![]}),_=>panic!("unknown field")};json!({"debug":formatted(&value),"clone":formatted(&clone),"clone_equal":value==clone,"changed_equal":value==other})}
fn main(){for line in io::stdin().lock().lines(){println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap()));}}
