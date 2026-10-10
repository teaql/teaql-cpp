//! Direct source-derived Debug formatting; fixtures only construct the models.
use serde_json::{Value as Json, json};
use std::io::{self, BufRead};
use teaql_core::{CompactRow, SmartList, Value};
#[path = "../fixture_value.rs"]
mod fixture_value;

fn row(input: &Json) -> CompactRow {
    let names: Vec<String> = input["names"]
        .as_array()
        .unwrap()
        .iter()
        .map(|v| v.as_str().unwrap().to_owned())
        .collect();
    let values: Vec<Value> = input["values"]
        .as_array()
        .unwrap()
        .iter()
        .map(fixture_value::value)
        .collect();
    CompactRow::new(names.into(), values)
}

fn formatted<T: std::fmt::Debug>(v: &T) -> Json {
    json!({"compact":format!("{v:?}"),"pretty":format!("{v:#?}"),
           "lower_hex":format!("{v:x?}"),"upper_hex":format!("{v:X?}"),
           "pretty_lower_hex":format!("{v:#x?}"),"pretty_upper_hex":format!("{v:#X?}"),
           "width_precision":format!("{v:>40.2?}"),
           "fancy":format!("{v:🧊^+18.3?}"),"zero_pad":format!("{v:+012?}")})
}

fn observe(input: Json) -> Json {
    let current = row(&input["row"]);
    let mut list = SmartList::new(
        input["values"]
            .as_array()
            .unwrap()
            .iter()
            .map(fixture_value::value)
            .collect(),
    );
    list.is_loaded = input["loaded"].as_bool().unwrap();
    list.total_count = input["total"].as_str().map(|v| v.parse().unwrap());
    for (k, v) in input["summary"].as_object().unwrap() {
        list.summary.insert(k.clone(), fixture_value::value(v));
    }
    for (k, v) in input["aggregations"].as_object().unwrap() {
        list.aggregations.insert(k.clone(), fixture_value::value(v));
    }
    let mut facet = SmartList::new(vec![current.clone()]);
    facet.is_loaded = false;
    facet.total_count = Some(31);
    list.add_facet("rows", facet);
    json!({"row":formatted(&current),"list":formatted(&list)})
}

fn main() {
    for line in io::stdin().lock().lines() {
        println!("{}", observe(serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
