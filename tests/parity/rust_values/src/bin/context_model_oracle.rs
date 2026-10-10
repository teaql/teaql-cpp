//! Actual pinned Context model constructors, derives and Error methods.
use serde_json::{json, Value};
use std::io::{self, BufRead};
use teaql_runtime::{ContextEntityRef, ContextRootError, FixEvidence, FixEvidenceSource};
fn formatted<T: std::fmt::Debug>(value: &T) -> Value {
    json!({"compact":format!("{value:?}"),"pretty":format!("{value:#?}"),
        "lower_hex":format!("{value:x?}"),"upper_hex":format!("{value:X?}"),
        "pretty_lower_hex":format!("{value:#x?}"),"pretty_upper_hex":format!("{value:#X?}"),
        "width_precision":format!("{value:>40.2?}"),"fancy":format!("{value:🧊^+18.3?}"),
        "zero_pad":format!("{value:+012?}")})
}
fn evaluate(input: Value) -> Value {
    if input["mode"] == "root" {
        let entity = ContextEntityRef {entity_type:input["entity"].as_str().unwrap().into(),id:input["id"].as_str().unwrap().parse().unwrap()};
        let error = ContextRootError {expected_entity_type:input["expected"].as_str().unwrap().into(),actual_root:input["present"].as_bool().unwrap().then(||entity.clone())};
        let mut changed = entity.clone(); changed.id = if entity.id == u64::MAX {0} else {entity.id+1};
        let mut changed_error=error.clone();changed_error.expected_entity_type.push('!');
        return json!({"entity":formatted(&entity),"entity_clone":formatted(&entity.clone()),
            "entity_clone_equal":entity==entity.clone(),"entity_changed_equal":entity==changed,
            "error":formatted(&error),"error_clone":formatted(&error.clone()),
            "error_clone_equal":error==error.clone(),"error_changed_equal":error==changed_error,
            "display":error.to_string(),"source_none":std::error::Error::source(&error).is_none()});
    }
    let source = if input["source"].as_u64().unwrap()==0 {FixEvidenceSource::Clock} else {FixEvidenceSource::Context};
    let built=std::panic::catch_unwind(||FixEvidence::new(input["entity"].as_str().unwrap(),input["path"].as_str().unwrap(),source,input["label"].as_str().unwrap()));
    match built {
        Ok(mut evidence) => {
            if input["mutate"]==true {evidence.entity_type.clear();evidence.model_path.clear();evidence.source_label="token=raw".into();}
            let mut changed=evidence.clone();changed.source_label.push('!');
            json!({"evidence":formatted(&evidence),"clone":formatted(&evidence.clone()),
                "clone_equal":evidence==evidence.clone(),"changed_equal":evidence==changed,
                "source":formatted(&evidence.source)})
        },
        Err(payload) => json!({"panic":payload.downcast_ref::<String>().map(String::as_str).or_else(||payload.downcast_ref::<&str>().copied()).unwrap()}),
    }
}
fn main(){std::panic::set_hook(Box::new(|_|{}));for line in io::stdin().lock().lines(){println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap()));}}
