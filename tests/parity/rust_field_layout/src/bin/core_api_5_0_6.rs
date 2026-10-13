//! Direct original public Timestamp traits and EntityDescriptor builders.
use serde_json::{Value, json};
use std::hash::{Hash, Hasher};
use std::io::{self, BufRead};
use teaql_core::{EntityDescriptor, time::Timestamp};

#[derive(Default)]
struct HashInput(Vec<i64>);
impl Hasher for HashInput {
    fn finish(&self) -> u64 {
        0
    }
    fn write(&mut self, _: &[u8]) {
        panic!("unexpected Timestamp Hash representation");
    }
    fn write_i64(&mut self, value: i64) {
        self.0.push(value);
    }
}

fn names(value: &Value) -> Vec<String> {
    value
        .as_array()
        .unwrap()
        .iter()
        .map(|v| v.as_str().unwrap().to_owned())
        .collect()
}

fn observe(input: Value) -> Value {
    if input["kind"] == "timestamp" {
        let values: Vec<_> = input["values"]
            .as_array()
            .unwrap()
            .iter()
            .map(|v| Timestamp::from(v.as_str().unwrap().parse::<i64>().unwrap()))
            .collect();
        let mut sorted = values.clone();
        sorted.sort();
        let comparisons: Vec<_> = values
            .iter()
            .flat_map(|a| {
                values.iter().map(move |b| match a.cmp(b) {
                    std::cmp::Ordering::Less => -1,
                    std::cmp::Ordering::Equal => 0,
                    std::cmp::Ordering::Greater => 1,
                })
            })
            .collect();
        let hash_inputs: Vec<_> = values
            .iter()
            .map(|value| {
                let mut input = HashInput::default();
                value.hash(&mut input);
                assert_eq!(input.0.len(), 1);
                input.0[0].to_string()
            })
            .collect();
        json!({"default": Timestamp::default().as_millis().to_string(),
               "sorted": sorted.iter().map(|v| v.as_millis().to_string()).collect::<Vec<_>>(),
               "comparisons": comparisons, "hash_inputs": hash_inputs})
    } else {
        let mut descriptor = EntityDescriptor::new(input["name"].as_str().unwrap());
        if !input["initial"].is_null() {
            descriptor = descriptor.audit_mask_fields(names(&input["initial"]));
        }
        let initial_declared = descriptor.audit_mask_fields_declared;
        descriptor = descriptor.audit_mask_fields(names(&input["fields"]));
        if input["alias"].as_bool().unwrap() {
            descriptor = descriptor
                .clone()
                .audit_mask_fields(descriptor.audit_mask_fields.clone());
        }
        json!({"name": descriptor.name, "table": descriptor.table_name,
               "initial_declared": initial_declared, "declared": descriptor.audit_mask_fields_declared,
               "fields": descriptor.audit_mask_fields, "clone_equal": descriptor.clone() == descriptor})
    }
}

fn main() {
    for line in io::stdin().lock().lines() {
        let input: Value = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", observe(input));
    }
}
