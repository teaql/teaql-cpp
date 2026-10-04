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
fn main(){for line in io::stdin().lock().lines(){let j:J=serde_json::from_str(&line.unwrap()).unwrap();let q=fixture_query_keys::query(&j["query"]);println!("{}",locked::output(&j,&q));}}
