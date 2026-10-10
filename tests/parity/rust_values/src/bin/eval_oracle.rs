use serde_json::{json, Value as Json};
use std::io::{self, BufRead};
use teaql_core::eval::{EvalResult, LoadState};

fn formatted<T: std::fmt::Debug>(value: &T) -> Json {
    json!([format!("{value:?}"), format!("{value:#?}"), format!("{value:x?}"), format!("{value:#X?}")])
}
fn canonical(mut value: Json) -> Json {
    if let Some(fields) = value.get_mut("Partial").and_then(Json::as_array_mut) {
        fields.sort_by(|a,b| a.as_str().cmp(&b.as_str()));
    }
    value
}
fn evaluate(input: Json) -> Json {
    let state = serde_json::from_value::<LoadState>(input["state"].clone());
    let other = serde_json::from_value::<LoadState>(input["other_state"].clone());
    let state_equal = match (&state, &other) {
        (Ok(left), Ok(right)) => json!(left == right),
        _ => Json::Null,
    };
    let state = match state {
        Ok(state) => {
            let debug = if matches!(&state, LoadState::Partial(fields) if fields.len() > 1) {Json::Null} else {formatted(&state)};
            json!({"json":canonical(serde_json::to_value(&state).unwrap()), "loaded":input["fields"].as_array().unwrap().iter().map(|f|state.is_loaded(f.as_str().unwrap())).collect::<Vec<_>>(), "clone_equal":state==state.clone(), "debug":debug})
        },
        Err(_) => Json::Null,
    };
    let result = serde_json::from_value::<EvalResult<String>>(input["result"].clone());
    let result = match result {
        Ok(result) => {
            let mut map_calls=0;
            let mapped = result.clone().map(|value| {map_calls+=1; value.chars().count() as u64});
            let mut chain_calls=0;
            let chained=result.clone().and_then(input["field"].as_str().unwrap(), |value| {
                chain_calls+=1;
                match input["next"].as_str().unwrap() {
                    "value"=>EvalResult::Value(value),
                    "null"=>EvalResult::Null,
                    _=>EvalResult::NotLoaded {failed_node: input["node"].as_str().unwrap().into(), attempted_path:input["path"].as_str().unwrap().into()},
                }
            });
            json!({"json":result,"mapped":mapped,"map_calls":map_calls,"chained":chained,"chain_calls":chain_calls,"debug":formatted(&result),"chained_debug":formatted(&chained)})
        },
        Err(_) => Json::Null,
    };
    json!({"default_state":LoadState::default(),"state_equal":state_equal,"state":state,"result":result})
}
fn main() {
    for line in io::stdin().lock().lines() {
        println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
