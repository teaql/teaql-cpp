//! Generated dual-forward relationships: ambiguous lookup and inferred-key conflicts.
use cpp_sample_runtime_core::*;
use serde_json::{json, Value as J};
use std::io::{self, BufRead};
use teaql_core::Entity;
use teaql_runtime::LedgerEntity;
fn run(kind: &str) -> J {
    let context = module().into_context();
    if kind == "ambiguous" {
        let input = json!({"id":1,"first_record_id":7,"first_record":{"id":7,"label":"shared"},
            "second_record_id":7,"second_record":{"id":7}});
        let row = context.decode_json_entity::<SampleLink>(&input).unwrap();
        let state = row.entity_runtime_state().unwrap();
        let first = state.resolve_relation_option::<SampleRecord>("SampleLink",1,"first_record").unwrap().as_ref().unwrap();
        let second = state.resolve_relation_option::<SampleRecord>("SampleLink",1,"second_record").unwrap().as_ref().unwrap();
        json!({"first_loaded":first.is_field_loaded("label"),"second_loaded":second.is_field_loaded("label"),
            "ambiguous":state.resolve_entity::<SampleRecord>(7).is_none(),"json":row.clone().into_json(),
            "first_json":first.clone().into_json(),"second_json":second.clone().into_json()})
    } else if kind == "inferred_conflict" {
        let input = json!({"id":1,"first_links":[
            {"id":2,"first_record_id":1,"second_record":{"id":7}},
            {"id":2,"first_record_id":1,"second_record":{"id":8}}]});
        let error = match context.decode_json_entity::<SampleRecord>(&input) {
            Ok(_) => panic!("inferred keys must conflict"), Err(error) => error,
        };
        json!({"error":error.to_string()})
    } else { panic!("unknown fixture"); }
}
fn main(){for line in io::stdin().lock().lines(){let input:J=serde_json::from_str(&line.unwrap()).unwrap();println!("{}",run(input["case"].as_str().unwrap()));}}
