use domain::{Child, E, Item, Q, ServiceRuntimeExecutor};
use serde_json::{Value as Json, json};
use std::{future::Future, io::{self, BufRead}, pin::pin, sync::{Arc, Mutex}, task::{Context, Poll, Wake, Waker}};
use teaql_core::{Entity, Expr, SelectQuery, TeaqlEntity, Value, eval::EvalResult};
use teaql_runtime::{LedgerEntity, PurposedSelectQuery};

struct Notify;
impl Wake for Notify { fn wake(self: Arc<Self>) {} }
fn block_on<T>(future: impl Future<Output = T>) -> T {
    let mut future = pin!(future);
    let waker = Waker::from(Arc::new(Notify));
    let mut context = Context::from_waker(&waker);
    loop { if let Poll::Ready(value) = future.as_mut().poll(&mut context) { return value; } }
}

fn child_value(child: &Child) -> Json {
    let secret = match child.eval_secret() {
        EvalResult::Value(_) => json!({"value": E::child(child).get_secret().unwrap()}),
        EvalResult::Null => json!({"null": true}),
        EvalResult::NotLoaded { .. } => json!({"not_loaded": true}),
    };
    json!({"id": child.id(), "secret": secret})
}

fn observe(input: &Json) -> Json {
    let pool = Arc::new(Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));
    pool.lock().unwrap().execute_batch(include_str!("ap02_reverse_relation_sql_5_0_6.sql")).unwrap();
    let user_context = block_on(domain::service_runtime_from_pool(pool)).unwrap()
        .with_active_root("Platform", 1);
    let mut query: SelectQuery = Q::items().into();
    query = query.and_filter(Expr::eq("id", Value::U64(input["parent"].as_u64().unwrap())))
        .comment("Inspect injected reverse relation");
    if input["selected"] == true {
        let child: SelectQuery = if input["minimal"] == true {
            Q::children_minimal().into()
        } else { Q::children().into() };
        query = query.relation_query("links", child.order_asc("id"));
    }
    let query = PurposedSelectQuery::new(query, "Qualify typed relation loading");
    let service = user_context.entity_data_service::<ServiceRuntimeExecutor>("Item").unwrap();
    let result = block_on(service.fetch_enhanced_entities::<Item>(&query));
    let mut rows = Vec::new();
    let mut error = None;
    match result {
        Ok(list) => for item in &list.data {
            let state = item.entity_runtime_state().unwrap();
            let loaded = item.__load_state.is_loaded("links");
            let mut children = Vec::new();
            let mut graph_present = false;
            if input["many"] == true {
                if let Some(list) = state.resolve_relation_list::<Child>(
                    <Item as TeaqlEntity>::ENTITY_NAME, item.id(), "links") {
                    graph_present = true;
                    children.extend(list.data.iter().map(child_value));
                }
            } else if let Some(edge) = state.resolve_relation_option::<Child>(
                <Item as TeaqlEntity>::ENTITY_NAME, item.id(), "links") {
                graph_present = true;
                if let Some(child) = edge { children.push(child_value(child)); }
            }
            rows.push(json!({"id": item.id(), "name": E::item(item).get_name().unwrap(),
                "loaded": loaded, "graph_present": graph_present, "children": children,
                "json": item.clone().into_json()}));
        },
        Err(value) => error = Some(value.to_string()),
    }
    let sql = user_context.sql_logs().iter().map(|entry| json!({
        "sql": entry.sql, "comment": entry.comment, "purpose": entry.purpose,
        "count": entry.result_count,
    })).collect::<Vec<_>>();
    json!({"rows": rows, "error": error, "sql": sql})
}

fn main() {
    for line in io::stdin().lock().lines() {
        let input: Json = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", observe(&input));
    }
}
