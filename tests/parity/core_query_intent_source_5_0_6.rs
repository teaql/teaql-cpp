//! Observe the distinct original core setter and runtime authorization boundary.
use serde_json::{Value as Json, json};
use std::io::{self, BufRead};
use teaql_core::SelectQuery;
use teaql_runtime::PurposedSelectQuery;

fn formatted<T: std::fmt::Debug>(value: &T) -> Json {
    json!({"compact":format!("{value:?}"),"pretty":format!("{value:#?}"),
        "lower_hex":format!("{value:x?}"),"upper_hex":format!("{value:X?}"),
        "pretty_lower_hex":format!("{value:#x?}"),"pretty_upper_hex":format!("{value:#X?}"),
        "width_precision":format!("{value:>40.2?}"),"fancy":format!("{value:🧊^+18.3?}"),
        "zero_pad":format!("{value:+012?}")})
}

fn observe(input: &Json) -> Json {
    let mut query = SelectQuery::new(input["entity"].as_str().unwrap()).project("id");
    if let Some(comment) = input["comment"].as_str() {
        query = query.comment(comment);
    }
    let mut stages = Vec::new();
    for value in input["purposes"].as_array().unwrap() {
        let purpose = value.as_str().unwrap();
        let before = formatted(&query);
        let mut assigned = query.clone();
        assigned.purpose = Some(purpose.to_owned());
        query = query.purpose(purpose);
        let wrapped = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
            PurposedSelectQuery::new(query.clone(), purpose)
        }));
        let runtime = match wrapped {
            Ok(wrapped) => {
                let twice = PurposedSelectQuery::new(wrapped.clone().into_query(), "second boundary");
                json!({"debug":formatted(&wrapped), "clone":formatted(&wrapped.clone()),
                       "twice":formatted(&twice), "trace_count":wrapped.as_query().trace_chain.len()})
            }
            Err(error) => json!({"error":error.downcast_ref::<String>().cloned()
                .or_else(||error.downcast_ref::<&str>().map(|v|v.to_string())).unwrap()}),
        };
        stages.push(json!({"before":before,"core":formatted(&query),
            "same_as_assignment":query==assigned,"trace_count":query.trace_chain.len(),
            "prepared":formatted(&query.clone().prepare_for_list().unwrap()),"runtime":runtime}));
    }
    json!(stages)
}

fn main() {
    std::panic::set_hook(Box::new(|_| {}));
    for line in io::stdin().lock().lines() {
        println!("{}", observe(&serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
