//! Exercise the original unconstrained request marker; no substitute query mechanics.
#![allow(unexpected_cfgs)]
use cpp_dynamic_json_core::Q;
use serde_json::{Value as Json, json};
use std::io::{self, BufRead};
use teaql_core::{SelectQuery, request::QuerySelection};

struct NotAnEntity;

// Compile this observer with --cfg teaql_marker_negative to verify every public
// execution/creation refusal against the actual original generated implementation.
#[cfg(teaql_marker_negative)]
fn refused(user_context: &teaql_runtime::UserContext) {
    macro_rules! request {
        () => { Q::people().return_type::<NotAnEntity>().comment("what").purpose("why") };
    }
    let _ = request!().execute_for_list(user_context);
    let _ = request!().execute_for_rows(user_context);
    let _ = request!().execute_for_stream(user_context);
    let _ = request!().execute_for_first(user_context);
    let _ = request!().execute_for_one(user_context);
    let _ = request!().execute_for_page(user_context, 0, 1);
    let _ = request!().execute_for_count(user_context);
    let _ = request!().execute_for_exists(user_context);
    let _ = request!().new_entity(user_context);
}

fn observe(input: &Json) -> Json {
    macro_rules! inspect {
        ($request:expr, $child:expr, $relation:ident) => {{
            let text = input["text"].as_str().unwrap();
            let mut request = $request;
            let shape = input["shape"].as_str().unwrap();
            if shape == "options" || shape == "combined" {
                request = request.comment(text).raw_sql(text).raw_sql_filter(text)
                    .create_property_as("_computed", text);
            }
            if shape == "child" || shape == "combined" {
                let child = $child.return_type::<NotAnEntity>().comment(text);
                request = request.enhance_child(child.clone())
                    .matching_any_of(child.clone()).$relation(child);
            }
            let bare = format!("{:?}", request.query());
            let options = format!("{:?}", request.query_options());
            let relations = format!("{:?}", request.relation_selections());
            let filters = format!("{:?}", request.relation_filters());
            let children = format!("{:?}", request.child_enhancements());
            let raw = request.clone().into_query();
            let complete: SelectQuery = request.clone().into();
            let selection: QuerySelection = request.into();
            json!({"bare":bare,"options":options,"relations":relations,"filters":filters,
                "children":children,"raw":format!("{raw:?}"),"complete":format!("{complete:?}"),
                "selection":format!("{selection:?}")})
        }};
    }
    macro_rules! marker {
        ($type:ty) => {{
            if input["entity"] == "person" {
                inspect!((if input["minimal"] == true { Q::people_minimal() } else { Q::people() })
                    .return_type::<$type>(), Q::orders(), select_orders_with)
            } else {
                inspect!((if input["minimal"] == true { Q::orders_minimal() } else { Q::orders() })
                    .return_type::<$type>(), Q::people(), select_customer_with)
            }
        }};
    }
    match input["marker"].as_str().unwrap() {
        "integer" => marker!(u64),
        "tuple" => marker!((i64, String)),
        "custom" => marker!(NotAnEntity),
        _ => unreachable!(),
    }
}

fn main() {
    for line in io::stdin().lock().lines() {
        let input: Json = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", observe(&input));
    }
}
