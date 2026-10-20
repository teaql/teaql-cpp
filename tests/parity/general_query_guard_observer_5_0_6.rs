// Shared observation code; all query/selection/execution mechanics are original APIs.
use serde_json::{Value as Json, json};
use teaql_core::{Entity, TeaqlEntity, SelectQuery, request::QuerySelection};
use std::io::{self, BufRead};
struct NotAnEntity;

fn snapshot<T: Entity + TeaqlEntity>(entity: T) -> Json {
    let mut loaded = serde_json::Map::new();
    let descriptor = T::entity_descriptor();
    for field in &descriptor.properties {
        loaded.insert(field.name.clone(), json!(entity.is_field_loaded(&field.name)));
    }
    for relation in &descriptor.relations {
        loaded.insert(relation.name.clone(), json!(entity.is_field_loaded(&relation.name)));
    }
    json!({"loaded": loaded, "json": entity.into_json()})
}

macro_rules! select {
    ($request:expr, $input:expr) => {{
        match $input["selection"].as_str().unwrap() {
            "none" => $request,
            "self" => $request.select_self(),
            "self_fields" => $request.select_self_fields(),
            "without_parent" => $request.select_self_without_parent(),
            "all" => $request.select_all(),
            "children" => $request.select_children(),
            "any" => $request.select_any(),
            _ => unreachable!(),
        }
    }};
}
macro_rules! state {
    ($query:expr) => {{
        let query = &$query;
        let raw = query.clone().into_query();
        let complete: SelectQuery = query.clone().into();
        let selection: QuerySelection = query.clone().into();
        json!({"bare":format!("{:?}",query.query()), "options":format!("{:?}",query.query_options()),
            "relations":format!("{:?}",query.relation_selections()),
            "filters":format!("{:?}",query.relation_filters()),
            "children":format!("{:?}",query.child_enhancements()),
            "raw":format!("{raw:?}"), "complete":format!("{complete:?}"),
            "selection":format!("{selection:?}")})
    }};
}
macro_rules! observe_request {
    ($input:expr, $context:expr, $model:ident, $request:ident, $ordinary:ident, $minimal:ident, $children:ident) => {{
        macro_rules! construct {
            ($marker:ty) => {{
                let request = match $input["entry"].as_str().unwrap() {
                    "bare" => domain::$request::<$marker>::default(),
                    "default" => domain::Q::$ordinary().return_type::<$marker>(),
                    "minimal" => domain::Q::$minimal().return_type::<$marker>(),
                    "with_children" => domain::Q::$children().return_type::<$marker>(),
                    _ => unreachable!(),
                };
                state!(select!(request, $input))
            }};
        }
        if $input["mode"] == "construct" {
            match $input["marker"].as_str().unwrap() {
                "integer" => construct!(u64),
                "custom" => construct!(NotAnEntity),
                _ => construct!(domain::$model),
            }
        } else {
            let request = match $input["entry"].as_str().unwrap() {
                "bare" => domain::$request::<domain::$model>::default(),
                "default" => domain::Q::$ordinary(),
                "minimal" => domain::Q::$minimal(),
                "with_children" => domain::Q::$children(),
                _ => unreachable!(),
            };
            let request = select!(request, $input);
            let mut output = state!(request);
            output["rows"] = json!([]); output["error"] = Json::Null;
            let executable = request.comment("Inspect general request guards")
                .purpose("Qualify constructor and selection responsibilities");
            if $input["mode"] == "list" {
                match driver::block_on(executable.execute_for_list($context)) {
                    Ok(rows) => output["rows"] = json!(rows.into_iter().map(snapshot).collect::<Vec<_>>()),
                    Err(error) => output["error"] = json!(error.to_string()),
                }
            } else {
                match driver::block_on(executable.execute_for_rows($context)) {
                    Ok(rows) => output["rows"] = json!(rows.iter().map(|row|row.iter()
                        .map(|(key,value)|(key.clone(),format!("{value:?}")))
                        .collect::<std::collections::BTreeMap<_,_>>()).collect::<Vec<_>>()),
                    Err(error) => output["error"] = json!(error.to_string()),
                }
            }
            output["logs"] = json!($context.sql_logs().iter().map(|entry|json!({
                "sql":entry.sql,"params":entry.params.iter().map(|v|format!("{v:?}")).collect::<Vec<_>>(),
                "comment":entry.comment,"purpose":entry.purpose,"debug_sql":entry.debug_sql,
                "pretty_sql":entry.pretty_sql,"context":format!("{:?}",entry.log_context),
                "trace":entry.trace_path.iter().map(|v|format!("{v:?}")).collect::<Vec<_>>(),
                "result_count":entry.result_count,"result_type":entry.result_type,
                "affected_rows":entry.affected_rows,"summary":entry.result_summary})).collect::<Vec<_>>());
            output
        }
    }};
}

fn main() {
    for line in io::stdin().lock().lines() {
        let input: Json = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", observe(&input));
    }
}
