use serde_json::{Value as Json, json};
use std::{collections::HashMap, io::{self, BufRead}};
use teaql_core::dynamic_fields::{DynamicFieldDefinitions, DynamicFieldValues};
use teaql_core::{DataType, Value};
use teaql_runtime::EntityRuntimeState;

fn install(state: &mut EntityRuntimeState, bits: u64) {
    let definitions = DynamicFieldDefinitions::new("Item", "v1",
        vec![("number".to_owned(), DataType::F64)]).unwrap();
    let values = HashMap::from([("number".to_owned(), Value::F64(f64::from_bits(bits)))]);
    state.install_loaded_dynamic_fields(DynamicFieldValues::from_values(definitions, values).unwrap());
}

fn evaluate(input: Json) -> Json {
    let mut state = EntityRuntimeState::default();
    if let Some(bits) = input["bits"].as_str() {
        install(&mut state, u64::from_str_radix(bits, 16).unwrap());
    }
    if input["active"] == true { state.set_comment("receiving ledger"); }
    let before_clone = state == state;
    let copied = state.clone();
    let mut other = EntityRuntimeState::fresh_with_shared_graph(&state);
    if let Some(bits) = input["other_bits"].as_str() {
        install(&mut other, u64::from_str_radix(bits, 16).unwrap());
    }
    let rebound = other.with_shared_graph(&state);
    let output = json!({"self_before_clone":before_clone,"self_after_clone":state==state,
        "clone_self":copied==copied,"clone_forward":state==copied,"clone_reverse":copied==state,
        "independent_self":other==other,"independent_forward":state==other,
        "independent_reverse":other==state,"rebound_self":rebound==rebound,
        "rebound_forward":state==rebound,
        "dynamic_self":state.loaded_dynamic_fields().map(|v|v==v),
        "copied_dynamic_self":copied.loaded_dynamic_fields().map(|v|v==v)});
    state.clear_committed();
    json!({"before_clear":output,"self_after_clear":state==state,"clone_after_clear":state==copied})
}

fn main() {
    for line in io::stdin().lock().lines() {
        println!("{}", evaluate(serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
