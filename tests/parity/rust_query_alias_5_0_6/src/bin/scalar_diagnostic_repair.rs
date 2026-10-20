use cpp_query_aliases_core::{Q, E};
use serde_json::{json, Value as Json};
use std::{io::{self, BufRead}, sync::{Arc, Mutex}};
#[path = "../../../rust_field_layout/src/bin/original_like_privacy_5_0_6.rs"]
mod driver;
fn panic_fields(panic: Box<dyn std::any::Any + Send>) -> Json {
    let text = panic.downcast_ref::<String>().unwrap();
    let mut fields = serde_json::Map::new();
    for line in text.lines() {
        if let Some((name, value)) = line.split_once(": ") {
            fields.insert(name.into(), if name == "root" {json!(value)}
                else if name == "human_message" {json!(&value[1..value.len()-1])}
                else {serde_json::from_str(value).unwrap()});
        }
    }
    Json::Object(fields)
}
fn observe(input: &Json) -> Json {
    let pool = Arc::new(Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));
    pool.lock().unwrap().execute_batch(include_str!("../../../../support/query_alias_sql.sql")).unwrap();
    let user_context = driver::block_on(cpp_query_aliases_core::service_runtime_from_pool(pool))
        .unwrap().with_active_root("Platform", 1);
    let mut request = if input["minimal"] == true {Q::records_minimal()} else {Q::records()};
    if input["deleted"] == true { request = request.with_deleted_rows(); }
    macro_rules! run {($select:ident, $remove:ident, $get:ident) => {{
        match input["selection"].as_str().unwrap() {
            "selected" => request = request.$select(),
            "removed" => request = request.$select().$remove(),
            "replaced" => request = request.$select().$remove().$select(),
            "self" => request = request.select_self(),
            _ => {},
        }
        let core: teaql_core::SelectQuery = request.clone().into();
        let mut result = json!({"query":format!("{core:?}"), "present":false, "value":null,
                               "diagnostic":null, "error":null});
        let loaded = driver::block_on(request.with_id_is(input["id"].as_u64().unwrap())
            .comment("Inspect a keyword scalar").purpose("Apply the actual missing-load repair")
            .execute_for_one(&user_context));
        match loaded {
            Ok(Some(entity)) => {
                result["present"] = json!(true);
                match std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
                    let expression = E::record(&entity).$get();
                    if input["action"] == "eval" {format!("{:?}", expression.eval())}
                    else {format!("{:?}", expression.unwrap())}
                })) {
                    Ok(value) => result["value"] = json!(value),
                    Err(panic) => result["diagnostic"] = panic_fields(panic),
                }
            },
            Ok(None) => {},
            Err(error) => result["error"] = json!(error.to_string()),
        }
        result["logs"] = json!(user_context.sql_logs().iter().map(|entry|json!({
            "sql":entry.sql,"params":entry.params.iter().map(|value|format!("{value:?}")).collect::<Vec<_>>(),
            "comment":entry.comment,"purpose":entry.purpose,"trace":entry.trace_path.iter().map(|value|format!("{value:?}")).collect::<Vec<_>>()
        })).collect::<Vec<_>>());
        result
    }};}
    match input["field"].as_str().unwrap() {
        "class" => run!(select_class, unselect_class, get_class),
        "operator" => run!(select_operator, unselect_operator, get_operator),
        "template" => run!(select_template, unselect_template, get_template),
        _ => panic!("unknown scalar keyword"),
    }
}
fn main() {
    std::panic::set_hook(Box::new(|_| {}));
    for line in io::stdin().lock().lines() {
        println!("{}", observe(&serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
