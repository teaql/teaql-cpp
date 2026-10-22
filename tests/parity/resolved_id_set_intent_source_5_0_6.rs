// Appended observer; original runtime bodies remain unchanged.
#[cfg(test)]
mod cpp_resolved_id_set_intent_observer {
use serde_json::{Value as J, json};
use std::{
    future::Future,
    io::{self, BufRead},
    sync::{Arc, Mutex},
    task::{Context, Poll, Wake, Waker},
};
use teaql_core::*;
use teaql_data_service::*;
use crate::{DataServiceError, EntityDataService, IdSetStore, RetainedIdSet, UserContext};

struct Store {
    value: Mutex<Option<RetainedIdSet>>,
    notes: Mutex<Vec<String>>,
    reads: Mutex<usize>,
    fail_read: usize,
    fail_put: bool,
}
#[async_trait::async_trait]
impl IdSetStore for Store {
    async fn get(&self, key: &str) -> Result<Option<RetainedIdSet>, String> {
        self.notes.lock().unwrap().push(format!("get:{key}"));
        let mut count = self.reads.lock().unwrap();
        *count += 1;
        if *count == self.fail_read {
            return Err("store read failed".into());
        }
        Ok(self
            .value
            .lock()
            .unwrap()
            .as_ref()
            .filter(|v| v.query_key == key)
            .cloned())
    }
    async fn put(&self, value: RetainedIdSet) -> Result<(), String> {
        self.notes
            .lock()
            .unwrap()
            .push(format!("put:{}", value.query_key));
        if self.fail_put {
            return Err("store write failed".into());
        }
        *self.value.lock().unwrap() = Some(value);
        Ok(())
    }
    async fn invalidate(&self, _: &str) -> Result<(), String> {
        Ok(())
    }
}
struct Mock {
    ids: Vec<J>,
    requests: Mutex<Vec<String>>,
    fail: bool,
}
impl DataServiceExecutor for Mock {
    type Error = io::Error;
    fn capabilities(&self) -> DataServiceCapabilities {
        DataServiceCapabilities::default()
    }
}
impl QueryExecutor for Mock {
    async fn query(&self, request: QueryRequest) -> Result<QueryResult, Self::Error> {
        self.requests
            .lock()
            .unwrap()
            .push(format!("{:?}", request.query));
        if self.fail {
            return Err(io::Error::other("ID lookup failed"));
        }
        let rows = self
            .ids
            .iter()
            .map(|id| {
                let mut record = Record::new();
                if let Some(value) = id.as_u64() {
                    record.insert("id".into(), Value::U64(value));
                } else if let Some(text) = id.as_str() {
                    record.insert("id".into(), Value::Text(text.into()));
                }
                CompactRow::from_map(record)
            })
            .collect();
        Ok(QueryResult {
            rows,
            metadata: ExecutionMetadata::unrecorded_query(self.ids.len()),
        })
    }
}
impl MutationExecutor for Mock {
    async fn mutate(&self, _: MutationRequest) -> Result<MutationResult, Self::Error> {
        panic!("unexpected mutation")
    }
}
struct NoWake;
impl Wake for NoWake {
    fn wake(self: Arc<Self>) {}
}
fn wait<F: Future>(future: F) -> F::Output {
    let mut future = std::pin::pin!(future);
    let waker = Waker::from(Arc::new(NoWake));
    let mut cx = Context::from_waker(&waker);
    loop {
        if let Poll::Ready(value) = future.as_mut().poll(&mut cx) {
            return value;
        }
    }
}
fn evaluate(input: &J) -> J {
    let store = Arc::new(Store {
        value: Mutex::new(None),
        notes: Mutex::new(vec![]),
        reads: Mutex::new(0),
        fail_read: input["fail_read"].as_u64().unwrap() as usize,
        fail_put: input["fail_put"].as_bool().unwrap(),
    });
    let mut context = UserContext::new().with_user_identifier("reader");
    if !input["capture"].as_bool().unwrap() {
        context.disable_sql_log();
    }
    if let Some(root) = input["root"].as_array() {
        context = context.with_active_root(root[0].as_str().unwrap(), root[1].as_u64().unwrap());
    }
    context.set_id_set_store(store.clone());
    if let Some(controls) = input["root_controls"].as_array() {
        return json!(
            controls
                .iter()
                .map(|parts| {
                    let entity = parts[1].as_str().unwrap();
                    if parts[0] == "bootstrap" {
                        match context
                            .set_generated_bootstrap_active_root(entity, parts[2].as_u64().unwrap())
                        {
                            Ok(_) => json!({"ok": true}),
                            Err(error) => json!({"error": error.to_string()}),
                        }
                    } else {
                        match context.require_active_root(entity) {
                            Ok(root) => json!({"entity": root.entity_type, "id": root.id}),
                            Err(error) => json!({"error": error.to_string()}),
                        }
                    }
                })
                .collect::<Vec<_>>()
        );
    }
    let executor = Mock {
        ids: input["ids"].as_array().unwrap().clone(),
        requests: Mutex::new(vec![]),
        fail: input["failure"].as_bool().unwrap(),
    };
    let repo = EntityDataService::for_executor(&context, "Item", &executor);
    let mut query = repo.select().project("label").comment("purpose");
    if let Some(purpose) = input["purpose"].as_str() { query.purpose=Some(purpose.into()); }
    if input["enabled"].as_bool().unwrap() {
        query = query.optimize_pagination_with_id_set_config(
            "fixture",
            600,
            input["max_ids"].as_u64().unwrap(),
        );
    }
    if let Some(limit) = input["limit"].as_u64() {
        query = query.page(input["offset"].as_u64().unwrap(), limit);
    }
    match input["shape"].as_str().unwrap() {
        "aggregate" => query = query.count("count"),
        "grouped" => query = query.group_by("id"),
        "continuous" => query = query.optimize_for_continuous_page_fetch(),
        "raw" => query = query.raw_sql("SELECT id FROM items"),
        "partition" => query = query.partition_by("owner"),
        "having" => query = query.having(Expr::eq("id", 1u64)),
        "expr_order" => query = query.order_expr_asc(Expr::value(1u64)),
        "desc" => query = query.order_desc("id"),
        "tie" => query = query.order_desc("label"),
        _ => {}
    }
    let mut results = vec![];
    for _ in 0..input["calls"].as_u64().unwrap() {
        let result: Result<_, DataServiceError<io::Error>> =
            wait(repo.prepare_id_set_page(query.clone()));
        results.push(match result {
            Ok((q, total)) => json!({"query": format!("{q:?}"), "total": total}),
            Err(error) => json!({"error": error.to_string()}),
        });
    }
    json!({"results": results, "requests": *executor.requests.lock().unwrap(),
        "store": *store.notes.lock().unwrap(), "plan": context.id_set_plan(), "count": context.id_set_count()})
}
#[test]
fn id_set_intent() {
    let path=std::env::var("TEAQL_ID_SET_OBSERVER_INPUT").unwrap();
    let inputs:J=serde_json::from_str(&std::fs::read_to_string(path).unwrap()).unwrap();
    let cases=inputs.as_array().unwrap();
    assert_eq!(cases.len(),466);
    for (case,input) in cases.iter().enumerate() {
        println!("RESOLVED_ID_SET_INTENT_OBSERVATION {}",json!({"case":case,"input":input,"expected":evaluate(input)}));
    }
}
}
