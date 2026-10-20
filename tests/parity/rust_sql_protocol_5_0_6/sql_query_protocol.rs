// Historical provider fixtures; only request construction follows the current protocol.
use serde_json::{Value as J, json};
use std::{
    future::Future,
    io::{self, BufRead},
    sync::{
        Arc, Mutex,
        atomic::{AtomicUsize, Ordering},
    },
    task::{Context, Poll, Waker},
};
use teaql_core::{CompactRow, EntityDescriptor, SelectQuery, TraceKind, TraceNode, Value};
use teaql_data_service::{ExecutionMetadata, QueryExecutor, QueryRequest, SchemaProvider};
use teaql_sql::*;
#[path = "../rust_values/src/fixture_sql.rs"]
mod fixture_sql;
#[path = "../rust_values/src/fixture_value.rs"]
mod fixture_value;
#[derive(Debug)]
struct Error;
impl std::fmt::Display for Error {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "fetch failed")
    }
}
impl std::error::Error for Error {}
fn values(v: &[Value]) -> Vec<J> {
    v.iter().map(Value::to_json_value).collect()
}
fn metadata(m: ExecutionMetadata) -> J {
    json!({"backend":m.backend,"operation":m.operation as usize,"outcome":m.sql_log.execution_outcome.map(|v|v as usize),"count":m.result_count,"comment":m.comment,"sql":m.parameterized_query,"debug":m.debug_query,"params":values(&m.params),"policies":m.sql_log.parameter_policies.iter().map(|p|*p as usize).collect::<Vec<_>>(),"generated":m.sql_log.generated_sql,"epoch":m.started_at==std::time::UNIX_EPOCH && m.ended_at==std::time::UNIX_EPOCH,"trace":m.trace_chain.len()})
}
#[derive(Clone)]
struct Dialect {
    compiles: Arc<AtomicUsize>,
    probes: bool,
}
impl SqlDialect for Dialect {
    fn kind(&self) -> DatabaseKind {
        DatabaseKind::Sqlite
    }
    fn quote_ident(&self, s: &str) -> String {
        quote_identifier_if_needed(s, '"')
    }
    fn placeholder(&self, _: usize) -> String {
        "?".into()
    }
    fn prefers_small_parent_relation_probes(&self) -> bool {
        self.probes
    }
    fn compile_select(
        &self,
        e: &EntityDescriptor,
        q: &SelectQuery,
    ) -> Result<CompiledQuery, SqlCompileError> {
        self.compiles.fetch_add(1, Ordering::SeqCst);
        fixture_sql::FixtureDialect { numbered: false }.compile_select(e, q)
    }
}
#[derive(Clone)]
struct Schema {
    entity: Arc<EntityDescriptor>,
    lookups: Arc<AtomicUsize>,
}
impl SchemaProvider for Schema {
    fn get_entity(&self, name: &str) -> Option<Arc<EntityDescriptor>> {
        self.lookups.fetch_add(1, Ordering::SeqCst);
        (name == self.entity.name).then(|| self.entity.clone())
    }
}
#[derive(Clone)]
struct Transport {
    mode: Arc<Mutex<String>>,
    calls: Arc<Mutex<Vec<J>>>,
}
impl Transport {
    async fn fetch(
        &self,
        q: &CompiledQuery,
        index: Option<usize>,
        v: &[Value],
    ) -> Result<Vec<CompactRow>, Error> {
        self.calls.lock().unwrap().push(json!({"sql":q.sql,"params":values(&q.params),"comment":q.comment,"index":index,"values":values(v)}));
        let mode = self.mode.lock().unwrap().clone();
        match mode.as_str() {
            "pending" => std::future::pending().await,
            "panic" => panic!("provider panic"),
            "error" => Err(Error),
            _ => Ok(vec![]),
        }
    }
}
impl SqlTransport for Transport {
    type Error = Error;
    async fn fetch_all_compact_sql(&self, q: &CompiledQuery) -> Result<Vec<CompactRow>, Error> {
        self.fetch(q, None, &[]).await
    }
    async fn fetch_repeated_compact_sql(
        &self,
        q: &CompiledQuery,
        i: usize,
        v: &[Value],
    ) -> Result<Vec<CompactRow>, Error> {
        self.fetch(q, Some(i), v).await
    }
    async fn execute_sql(&self, _: &CompiledQuery) -> Result<u64, Error> {
        panic!("unused fixture path")
    }
}
fn evaluate(j: J) -> J {
    let dialect = Dialect {
        compiles: Arc::new(AtomicUsize::new(0)),
        probes: j["probes"].as_bool().unwrap_or(true),
    };
    let schema = Schema {
        entity: Arc::new(fixture_sql::entity(&j["entity"])),
        lookups: Arc::new(AtomicUsize::new(0)),
    };
    let transport = Transport {
        mode: Arc::default(),
        calls: Arc::default(),
    };
    let executor = SqlDataServiceExecutor::new(dialect.clone(), transport.clone(), schema.clone());
    let clone = executor.clone();
    let events = Arc::new(Mutex::new(Vec::new()));
    let observer = {
        let events = events.clone();
        Arc::new(move |m| events.lock().unwrap().push(metadata(m)))
    };
    let mut results = Vec::new();
    for item in j["requests"].as_array().unwrap() {
        let mut q = fixture_sql::query(&item["query"]);
        q.entity = item["entity"]
            .as_str()
            .unwrap_or(&schema.entity.name)
            .into();
        q.top_n_probe_parent_threshold = item["threshold"].as_u64().map(|v| v as usize);
        if let Some(v) = item["hard_limit"].as_u64() {
            q.hard_limit = v;
        }
        *transport.mode.lock().unwrap() = item["mode"].as_str().unwrap_or("success").into();
        let active = if results.len() % 2 == 0 {
            &executor
        } else {
            &clone
        };
        if item["intent"] == true {
            let _ = active.query_log_intent(&q);
        }
        let intent = match teaql_core::QueryIntent::from_optional(
            item["comment"].as_str().or(q.comment.as_deref()),
            item["purpose"].as_str(),
        ) {
            Ok(value) => value,
            Err(error) => {
                results.push(json!({"intent_error":error.to_string(),"compiles":dialect.compiles.load(Ordering::SeqCst),"lookups":schema.lookups.load(Ordering::SeqCst)}));
                continue;
            }
        };
        let request = QueryRequest {
            query: q,
            trace_chain: if item["trace"] == true {
                vec![TraceNode {
                    kind: TraceKind::Request,
                    entity_type: "Fixture".into(),
                    entity_id: None,
                    comment: "request trace".into(),
                }]
            } else {
                vec![]
            },
            intent,
            capture_debug_query: item["debug"].as_bool().unwrap_or(false),
            capture_execution_metadata: item["metadata"].as_bool().unwrap_or(true),
        };
        let mut out = match std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
            let mut f = std::pin::pin!(active.query_observed(request, Some(observer.clone())));
            match f.as_mut().poll(&mut Context::from_waker(Waker::noop())) {
                Poll::Pending => json!({"pending":true}),
                Poll::Ready(Ok(result)) => json!({"metadata":metadata(result.metadata)}),
                Poll::Ready(Err(e)) => json!({"error":e.to_string()}),
            }
        })) {
            Ok(v) => v,
            Err(_) => json!({"panic":true}),
        };
        out["compiles"] = json!(dialect.compiles.load(Ordering::SeqCst));
        out["lookups"] = json!(schema.lookups.load(Ordering::SeqCst));
        results.push(out);
    }
    json!({"results":results,"calls":*transport.calls.lock().unwrap(),"events":*events.lock().unwrap()})
}
fn main() {
    std::panic::set_hook(Box::new(|_| {}));
    for line in io::stdin().lock().lines() {
        println!(
            "{}",
            evaluate(serde_json::from_str(&line.unwrap()).unwrap())
        );
    }
}
