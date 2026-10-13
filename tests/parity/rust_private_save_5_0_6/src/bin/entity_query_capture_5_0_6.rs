//! Exercise the pinned public EntityDataService entry points, without copied orchestration.
use serde_json::{Value as J, json};
use std::{
    future::Future,
    io::{self, BufRead},
    sync::{Arc, Mutex},
    task::{Context, Poll, Wake, Waker},
};
use teaql_core::*;
use teaql_data_service::*;
use teaql_runtime::{
    EntityDataService, InMemoryAggregationCache, PurposedSelectQuery, RuntimeModule,
};

#[derive(Default)]
struct State {
    requests: Mutex<Vec<J>>,
}
struct Mock {
    state: Arc<State>,
    fail: bool,
}
impl DataServiceExecutor for Mock {
    type Error = io::Error;
    fn capabilities(&self) -> DataServiceCapabilities {
        DataServiceCapabilities::default()
    }
}
fn trace(nodes: &[TraceNode]) -> J {
    json!(
        nodes
            .iter()
            .map(|n| json!({"kind":n.kind as u8,"entity":n.entity_type,"comment":n.comment}))
            .collect::<Vec<_>>()
    )
}
impl QueryExecutor for Mock {
    async fn query(&self, q: QueryRequest) -> Result<QueryResult, Self::Error> {
        self.state.requests.lock().unwrap().push(json!({"entity":q.query.entity,"projection":q.query.projection,
            "slice":q.query.slice.as_ref().map(|s|json!([s.offset,s.limit])),"request_trace":trace(&q.trace_chain),
            "query_trace":trace(&q.query.trace_chain),"comment":q.comment(),"query_comment":q.query.comment,
            "capture":q.capture_execution_metadata,"debug":q.capture_debug_query,"filter":q.query.filter.as_ref().map(|f|format!("{f:?}"))}));
        if self.fail {
            return Err(io::Error::other("executor failed"));
        }
        Ok(QueryResult {
            rows: vec![CompactRow::from_map([("id".into(), Value::U64(1))].into())],
            metadata: ExecutionMetadata::unrecorded_query(1),
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
    let state = Arc::new(State::default());
    let mut descriptor = EntityDescriptor::new("Item");
    descriptor
        .properties
        .push(PropertyDescriptor::new("id", DataType::U64));
    descriptor.properties[0].is_id = true;
    let mut context = RuntimeModule::new()
        .descriptor(descriptor)
        .into_context()
        .with_user_identifier("fixture");
    if !input["capture"].as_bool().unwrap() {
        context.disable_sql_log();
    }
    context.set_sql_log_options(teaql_runtime::SqlLogOptions {
        select: input["select_capture"].as_bool().unwrap(),
        mutation: input["mutation_capture"].as_bool().unwrap(),
    });
    if input["backend"].as_bool().unwrap() {
        context.insert_resource(InMemoryAggregationCache::with_namespace("integration"));
    }
    let executor = Mock {
        state: state.clone(),
        fail: input["failure"].as_bool().unwrap(),
    };
    let repo = EntityDataService::for_executor(&context, "Item", &executor);
    let mut query = repo.select();
    if input["cache"].as_bool().unwrap() {
        query = query.enable_aggregation_cache();
    }
    if input["aggregate"].as_bool().unwrap() {
        query = query.count("count");
    }
    if let Some(limit) = input["limit"].as_u64() {
        query = query.limit(limit);
    }
    if let Some(text) = input["comment"].as_str() {
        query = query.comment(text);
    }
    if input["continuous"].as_bool().unwrap() {
        query = query
            .order_asc("id")
            .limit(1)
            .optimize_for_continuous_page_fetch();
    }
    let mut outcomes = vec![];
    for index in 0..input["calls"].as_u64().unwrap() {
        let mut page = query.clone();
        if input["continuous"].as_bool().unwrap() {
            let offset = input.get("offsets").map(|values| values[index as usize].as_u64().unwrap()).unwrap_or(index);
            page = page.offset(offset);
        }
        let wrapped = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
            PurposedSelectQuery::new(page, input["purpose"].as_str().unwrap())
        }));
        let wrapped = match wrapped {
            Ok(v) => v,
            Err(_) => {
                outcomes.push(json!({"purpose_panic":true}));
                continue;
            }
        };
        let result = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| match input["mode"].as_str().unwrap() {
            "list" => wait(repo.fetch_smart_list(&wrapped)).map(|v| v.data.len()),
            "aggregates" => wait(repo.fetch_smart_list_with_relation_aggregates(&wrapped, &[]))
                .map(|v| v.data.len()),
            "owned" => wait(repo.fetch_all_owned(wrapped)).map(|v| v.len()),
            _ => wait(repo.fetch_all(&wrapped)).map(|v| v.len()),
        }));
        outcomes.push(match result {
            Ok(Ok(n)) => json!({"rows":n}),
            Ok(Err(e)) => json!({"error":e.to_string()}),
            Err(_) => json!({"execution_panic":true}),
        });
    }
    json!({"outcomes":outcomes,"requests":state.requests.lock().unwrap().clone(),"continuous":context.continuous_page_plan()})
}
fn main() {
    std::panic::set_hook(Box::new(|_| {}));
    for line in io::stdin().lock().lines() {
        let input: J = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", evaluate(&input));
    }
}
