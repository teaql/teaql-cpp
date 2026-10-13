use cpp_checker_fixture_core::{IntoU64, SampleDataPlan, SampleDataScale, SampleDataState};
use serde_json::{Value, json};
use std::{
    collections::HashSet,
    io::{self, BufRead},
};
#[path = "../../../rust_values/src/fixture_value.rs"]
mod fixture_value;
fn name(input: &Value) -> &'static str {
    match input.as_str().unwrap() {
        "person" => "person",
        "Person" => "Person",
        "orders" => "orders",
        "platform" => "platform",
        "" => "",
        _ => panic!("unknown fixture key"),
    }
}
fn main() {
    for line in io::stdin().lock().lines() {
        let input: Value = serde_json::from_str(&line.unwrap()).unwrap();
        let scale = match input["scale"].as_str().unwrap() {
            "Tiny" => SampleDataScale::Tiny,
            "Small" => SampleDataScale::Small,
            _ => SampleDataScale::Medium,
        };
        let mut state = SampleDataState::new(SampleDataPlan {
            scale,
            seed: input["seed"].as_u64().unwrap(),
        });
        for item in input["references"].as_array().unwrap() {
            state.add_reference(name(&item[0]), item[1].as_u64().unwrap());
        }
        for item in input["generated"].as_array().unwrap() {
            state.record_generated(name(item));
        }
        for item in input["skipped"].as_array().unwrap() {
            state.record_skipped(name(&item[0]), item[1].as_str().unwrap().into());
        }
        let mut selections = Vec::new();
        for item in input["picks"].as_array().unwrap() {
            let entity = name(&item["entity"]);
            let salt = item["salt"].as_u64().unwrap() as usize;
            let used: HashSet<u64> = item["used"]
                .as_array()
                .unwrap()
                .iter()
                .map(|v| v.as_u64().unwrap())
                .collect();
            selections.push(json!({"ids":state.ids(entity),"pick":state.pick_id(entity,salt),"unused":state.pick_unused_id(entity,salt,&used)}));
        }
        let refs = state.references.clone();
        let seed = state.plan.seed;
        let scale = format!("{:?}", state.plan.scale);
        let report = state.into_report();
        let conversions: Vec<u64> = input["values"]
            .as_array()
            .unwrap()
            .iter()
            .map(|v| {
                if v.is_null() {
                    None::<&teaql_core::Value>.into_u64()
                } else {
                    Some(&fixture_value::value(v)).into_u64()
                }
            })
            .collect();
        println!(
            "{}",
            json!({"scale":scale,"seed":seed,"references":refs,"picks":selections,"generated":report.generated,
            "skipped":report.skipped.iter().map(|v|json!({"entity":v.entity,"reason":v.reason})).collect::<Vec<_>>(),"converted":conversions})
        );
    }
}
