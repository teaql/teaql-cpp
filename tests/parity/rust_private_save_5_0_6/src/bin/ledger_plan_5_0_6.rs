//! Fixtures construct keys and metadata; the original method performs all sorting.
use std::{collections::BTreeMap, io::{self, BufRead}};
use serde_json::{json, Value as J};
use teaql_core::*;
use teaql_data_service::*;
use teaql_runtime::{EntityKey, RuntimeModule, __cpp_order_ledger};
#[path = "../../../rust_values/src/fixture_value.rs"]
mod fixture_value;

struct Unused;
impl DataServiceExecutor for Unused {
    type Error = io::Error;
    fn capabilities(&self) -> DataServiceCapabilities { DataServiceCapabilities::default() }
}
impl QueryExecutor for Unused {
    async fn query(&self, _: QueryRequest) -> Result<QueryResult, io::Error> {
        panic!("dependency sorting must not query")
    }
}
impl MutationExecutor for Unused {
    async fn mutate(&self, _: MutationRequest) -> Result<MutationResult, io::Error> {
        panic!("dependency sorting must not mutate")
    }
}
fn key(j: &J) -> EntityKey {
    EntityKey::new(j["entity"].as_str().unwrap(), fixture_value::value(&j["id"]))
}
fn evaluate(j: J) -> J {
    let mut module = RuntimeModule::new();
    for info in j["descriptors"].as_array().unwrap() {
        let mut descriptor = EntityDescriptor::new(info["entity"].as_str().unwrap());
        for input in info["relations"].as_array().unwrap() {
            let mut relation = RelationDescriptor::new(input["name"].as_str().unwrap(),
                                                       input["target"].as_str().unwrap());
            relation.local_key = input["local"].as_str().unwrap().into();
            relation.foreign_key = input["foreign"].as_str().unwrap().into();
            relation.many = input["many"].as_bool().unwrap();
            descriptor = descriptor.relation(relation);
        }
        module = module.descriptor(descriptor);
    }
    let context = module.into_context();
    let keys = j["keys"].as_array().unwrap().iter().map(key).collect();
    let mut changes = BTreeMap::new();
    for change in j["changes"].as_array().unwrap() {
        let values: BTreeMap<String, Value> = change["values"].as_object().unwrap().iter()
            .map(|(name, value)| (name.clone(), fixture_value::value(value))).collect();
        changes.insert(key(change), values.into());
    }
    match __cpp_order_ledger(&context, &Unused, keys, &changes) {
        Ok(keys) => json!({"keys": keys.iter().map(|k| format!("{k:?}")).collect::<Vec<_>>()}),
        Err(error) => json!({"error": error.to_string()}),
    }
}
fn main() {
    unsafe { std::env::set_var("TEAQL_LOG_ENDPOINT", "off"); }
    for line in io::stdin().lock().lines() {
        println!("{}", evaluate(serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
