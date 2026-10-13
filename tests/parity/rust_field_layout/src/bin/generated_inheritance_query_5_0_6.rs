use cpp_inheritance_core::Q;
use serde_json::{json, Value as Json};
use std::io::{self, BufRead};
use teaql_core::{request::QuerySelection, SelectQuery};
#[path = "../../../rust_values/src/fixture_value.rs"]
mod fixture_value;

fn describe(query: &SelectQuery) -> Json {
    json!({"entity":query.entity,"projection":query.projection,
        "filter":query.filter.as_ref().map(|value|format!("{:?}",value)),
        "slice":query.slice.as_ref().map(|value|format!("{:?}",value)),
        "hard_limit":query.hard_limit,
        "children":query.child_enhancements.iter().map(describe).collect::<Vec<_>>()})
}

fn relation_tree(query: &SelectQuery) -> Json {
    json!({"query":describe(query),"relations":query.relations.iter().map(|r|json!({"name":r.name,"query":r.query.as_ref().map(|q|relation_tree(q))})).collect::<Vec<_>>()})
}

fn observe(input: &Json) -> Json {
    macro_rules! apply {
        ($request:expr) => {{
            let mut request = $request;
            if input["types"].is_array() {
                request = request.match_types(input["types"].as_array().unwrap().iter().map(fixture_value::value));
            }
            if input["group"] == true { request = request.with_type_group(); }
            if input["limit"] == true { request = request.offset(3, 5); }
            if input["unlimited"] == true { request = request.unlimited(); }
            if input["enhance"] == "automatic" { request = request.enhance_children_if_needed(); }
            if input["enhance"] == "manual" {
                request = request.enhance_child(Q::research_academies_minimal().select_budget().with_rank_is(7));
            }
            if let Some(shortcut) = input["shortcut"].as_str() {
                let times = if input["repeat"] == true { 2 } else { 1 };
                for _ in 0..times {
                    request = match shortcut {
                        "self_without_parent" => request.select_self_without_parent(),
                        "all" => request.select_all(), "children" => request.select_children(),
                        _ => request.select_any(),
                    };
                }
            }
            let query: SelectQuery = QuerySelection::from(request).into_query();
            if input["shortcut"].is_string() { relation_tree(&query) } else { describe(&query) }
        }};
    }
    macro_rules! entry {
        ($normal:expr, $minimal:expr, $children:expr) => {
            match input["entry"].as_str().unwrap() {
                "minimal" => $minimal,
                "children" => $children,
                _ => $normal,
            }
        };
    }
    match input["entity"].as_str().unwrap() {
        "school" => {
            let request = entry!(Q::schools(), Q::schools_minimal(), Q::schools_with_children());
            apply!(if input["subtype"] == true {request.is_academy()} else {request})
        },
        "academy" => {
            let request = entry!(Q::academies(), Q::academies_minimal(), Q::academies_with_children());
            apply!(if input["subtype"] == true {request.is_research_academy()} else {request})
        },
        _ => apply!(entry!(Q::research_academies(), Q::research_academies_minimal(), Q::research_academies_with_children())),
    }
}
fn main() {
    for line in io::stdin().lock().lines() {
        let input: Json = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", observe(&input));
    }
}
