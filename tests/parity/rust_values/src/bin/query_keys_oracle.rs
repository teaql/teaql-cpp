//! Verbatim pinned key/canonicalization methods; wrappers only provide identity fields.
use std::{io::{self,BufRead},hash::{Hash,Hasher}};
use teaql_core::*;
use serde_json::{Value as J,json};
#[path="../fixture_value.rs"]mod fixture_value;
#[allow(dead_code)]#[path="../fixture_sql.rs"]mod fixture_sql;
#[path="../fixture_query_keys.rs"]mod fixture_query_keys;
mod locked {
    use super::*;
    struct Root {entity_type:String,id:u64}
    struct Context {user:Option<String>,root:Option<Root>}
    impl Context {fn user_identifier(&self)->Option<&str>{self.user.as_deref()}fn active_root_ref(&self)->Option<&Root>{self.root.as_ref()}}
    struct Metadata {context:Context}
    struct Service {metadata:Metadata}
    struct KeyRepo {data_service:Service}
    include!(concat!(env!("OUT_DIR"),"/query_keys.rs"));
    pub fn output(j:&J,q:&SelectQuery)->J{
        let repo=KeyRepo{data_service:Service{metadata:Metadata{context:Context{user:j["user"].as_str().map(str::to_owned),root:j.get("root").map(|r|Root{entity_type:r["entity"].as_str().unwrap().into(),id:r["id"].as_u64().unwrap()})}}}};
        let ns=j["namespace"].as_str().unwrap();json!({"debug":format!("{q:?}"),"canonical":format!("{:?}",canonical_aggregation_query(q)),"aggregation":aggregation_cache_key(ns,j["query_namespace"].as_str().unwrap(),q),"continuous":repo.continuous_page_query_key(q,ns),"ids":repo.id_set_query_key(q,ns),"entity_namespace":aggregation_cache_namespace(&q.entity)})
    }
}
// The JSON fixture stores u64 inputs; usize casts follow the observed target ABI.
// This prepares input values only. The included pinned key algorithms are unchanged.
fn target_expression(expr: &mut Expr) {
    match expr {
        Expr::Function {args,..} | Expr::And(args) | Expr::Or(args) => {
            for item in args { target_expression(item); }
        },
        Expr::Binary {left,right,..} => { target_expression(left); target_expression(right); },
        Expr::SubQuery {left,entity,query,..} => {
            target_expression(left);
            entity.audit_value_max_len = entity.audit_value_max_len.map(|n|n as u32 as usize);
            target_query(query);
        },
        Expr::Between {expr,lower,upper} => {
            target_expression(expr); target_expression(lower); target_expression(upper);
        },
        Expr::IsNull(expr) | Expr::IsNotNull(expr) | Expr::Not(expr) => target_expression(expr),
        Expr::Column(_) | Expr::Value(_) => {},
    }
}

fn target_query(query: &mut SelectQuery) {
    query.top_n_probe_parent_threshold = query.top_n_probe_parent_threshold.map(|n|n as u32 as usize);
    if let Some(config) = &mut query.stream_config { config.chunk_size = config.chunk_size as u32 as usize; }
    for named in &mut query.expr_projection { target_expression(&mut named.expr); }
    for expr in [&mut query.filter, &mut query.having].into_iter().flatten() { target_expression(expr); }
    for order in &mut query.order_by { if let Some(expr) = &mut order.expr { target_expression(expr); } }
    for relation in &mut query.relations { if let Some(child) = &mut relation.query { target_query(child); } }
    for group in &mut query.object_group_bys { target_query(&mut group.query); }
    for child in &mut query.child_enhancements { target_query(child); }
}

pub fn observe_target(input: &J, bits: u32) -> J {
    let mut query = fixture_query_keys::query(&input["query"]);
    match bits {
        32 => target_query(&mut query),
        64 => {},
        _ => panic!("Unsupported fixture usize width"),
    }
    locked::output(input, &query)
}

fn main(){for line in io::stdin().lock().lines(){let j:J=serde_json::from_str(&line.unwrap()).unwrap();println!("{}",observe_target(&j,usize::BITS));}}
