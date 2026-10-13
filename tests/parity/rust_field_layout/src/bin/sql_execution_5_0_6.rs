use serde_json::{Value as J, json};
use std::{
    future::Future,
    io::{self, BufRead},
    sync::{Arc, Mutex},
    task::{Context, Poll, Waker},
};
use teaql_core::*;
use teaql_data_service::{
    ExecutionMetadata, GuardedMutationExecutor, GuardedMutationRequest, MutationCommand,
    MutationExecutor, MutationRequest, SchemaProvider, SqlIntentRedactions, TransactionExecutor,
};
use teaql_sql::*;
#[path = "../../../rust_values/src/fixture_sql.rs"]
mod fixture_sql;
#[path = "../../../rust_values/src/fixture_value.rs"]
mod fixture_value;
use fixture_value::value;
fn mutation(j: &J, fallback: &str) -> MutationRequest {
    let name = j["entity"].as_str().unwrap_or(fallback);
    let version = j["version"].as_str().map(|s| s.parse::<i64>().unwrap());
    let fields = || {
        j["values"]
            .as_object()
            .unwrap()
            .iter()
            .map(|(k, v)| (k.clone(), value(v)))
            .collect::<std::collections::BTreeMap<_, _>>()
            .into()
    };
    let trace = || {
        let mut trace = j["trace"]
            .as_str()
            .map(|s| {
                vec![TraceNode {
                    kind: TraceKind::AuditReason,
                    entity_type: name.into(),
                    entity_id: None,
                    comment: s.into(),
                }]
            })
            .unwrap_or_default();
        if let Some(nodes) = j["trace_nodes"].as_array() {
            let kinds = [
                TraceKind::Operation,
                TraceKind::Request,
                TraceKind::Relation,
                TraceKind::Entity,
                TraceKind::Provider,
                TraceKind::Sql,
                TraceKind::Comment,
                TraceKind::Purpose,
                TraceKind::AuditReason,
            ];
            trace.extend(nodes.iter().map(|node| {
                TraceNode::typed(
                    kinds[node["kind"].as_u64().unwrap() as usize],
                    node["entity"].as_str().unwrap(),
                    node["id"].as_u64(),
                    node["comment"].as_str().unwrap(),
                )
            }));
        }
        trace
    };
    let command = match j["mode"].as_str().unwrap() {
        "batch" => MutationCommand::Batch(
            j["children"]
                .as_array()
                .unwrap()
                .iter()
                .map(|c| mutation(c, fallback))
                .collect(),
        ),
        "insert" => {
            let mut c = InsertCommand::new(name);
            c.values = fields();
            c.trace_chain = trace();
            MutationCommand::Insert(c)
        }
        "update" => {
            let mut c = UpdateCommand::new(name, value(&j["id"]));
            c.values = fields();
            c.expected_version = version;
            c.trace_chain = trace();
            c.old_values = j["old_values"].as_object().map(|fields| {
                fields
                    .iter()
                    .map(|(key, input)| (key.clone(), value(input)))
                    .collect::<std::collections::BTreeMap<_, _>>()
                    .into()
            });
            MutationCommand::Update(c)
        }
        "delete" => {
            let mut c = DeleteCommand::new(name, value(&j["id"]));
            c.soft_delete = j["soft"].as_bool().unwrap();
            c.expected_version = version;
            c.trace_chain = trace();
            MutationCommand::Delete(c)
        }
        _ => {
            let mut c = RecoverCommand::new(name, value(&j["id"]), version.unwrap());
            c.trace_chain = trace();
            MutationCommand::Recover(c)
        }
    };
    let mut request = command.request(j["comment"].as_str().unwrap()).unwrap();
    if let Some(values) = j["trusted_redactions"].as_array() {
        let mut redactions = SqlIntentRedactions::default();
        for input in values {
            redactions.capture_target_id(&value(input));
        }
        request = request.with_diagnostic_redactions(Some(Arc::new(redactions)));
    }
    request
}
fn values(v: &[Value]) -> Vec<J> {
    v.iter().map(Value::to_json_value).collect()
}
fn metadata(m: ExecutionMetadata) -> J {
    let mut secrets = vec![];
    let mut debug = vec![];
    m.sql_log
        .intent_redactions
        .extend_secrets(false, &mut secrets);
    m.sql_log.intent_redactions.extend_secrets(true, &mut debug);
    json!({"backend":m.backend,"operation":m.operation as usize,"outcome":m.sql_log.execution_outcome.map(|v|v as usize),"affected":m.affected_rows,"count":m.result_count,"comment":m.comment,"sql":m.parameterized_query,"debug":m.debug_query,"params":values(&m.params),"trace":m.trace_chain.len(),"trace_nodes":m.trace_chain.iter().map(|node|json!({"kind":node.kind as usize,"entity":node.entity_type,"id":node.entity_id,"comment":node.comment})).collect::<Vec<_>>(),"generated":m.sql_log.generated_sql,"policies":m.sql_log.parameter_policies.iter().map(|p|*p as usize).collect::<Vec<_>>(),"statements":m.statements.into_iter().map(metadata).collect::<Vec<_>>(),"secrets":secrets,"secrets_debug":debug})
}
#[derive(Debug)]
struct Error;
impl std::fmt::Display for Error {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "provider failed")
    }
}
impl std::error::Error for Error {}
struct Schema(Arc<EntityDescriptor>);
impl SchemaProvider for Schema {
    fn get_entity(&self, name: &str) -> Option<Arc<EntityDescriptor>> {
        (name == self.0.name).then(|| self.0.clone())
    }
}
struct Transport {
    steps: Vec<J>,
    calls: Mutex<Vec<J>>,
}
impl Transport {
    fn start(&self, q: &CompiledQuery, kind: &str) -> (String, u64) {
        let mut calls = self.calls.lock().unwrap();
        let step = self.steps.get(calls.len());
        let mode = step
            .and_then(|v| v["mode"].as_str())
            .unwrap_or("success")
            .into();
        let count = step.and_then(|v| v["count"].as_u64()).unwrap_or(1);
        calls.push(json!({"kind":kind,"sql":q.sql,"params":values(&q.params),"comment":q.comment}));
        (mode, count)
    }
    async fn run(&self, q: &CompiledQuery, kind: &str) -> Result<u64, Error> {
        let (mode, count) = self.start(q, kind);
        match mode.as_str() {
            "pending" => std::future::pending().await,
            "panic" => panic!("provider panic"),
            "error" => Err(Error),
            _ => Ok(count),
        }
    }
}
impl SqlTransport for Transport {
    type Error = Error;
    async fn execute_sql(&self, q: &CompiledQuery) -> Result<u64, Error> {
        self.run(q, "execute").await
    }
    async fn fetch_all_compact_sql(&self, q: &CompiledQuery) -> Result<Vec<CompactRow>, Error> {
        Ok((0..self.run(q, "fetch").await?)
            .map(|_| {
                CompactRow::new(
                    vec!["id".into(), "saved".into()].into(),
                    vec![Value::U64(7), Value::Text("database default".into())],
                )
            })
            .collect())
    }
}
struct Tx<'a>(&'a Transport);
impl SqlTransport for Tx<'_> {
    type Error = Error;
    async fn execute_sql(&self, q: &CompiledQuery) -> Result<u64, Error> {
        self.0.execute_sql(q).await
    }
    async fn fetch_all_compact_sql(&self, q: &CompiledQuery) -> Result<Vec<CompactRow>, Error> {
        self.0.fetch_all_compact_sql(q).await
    }
}
impl SqlTransaction for Tx<'_> {
    type Error = Error;
    async fn commit_sql(self) -> Result<(), Error> {
        Ok(())
    }
    async fn rollback_sql(self) -> Result<(), Error> {
        Ok(())
    }
}
impl SqlTransactionTransport for Transport {
    type Tx<'a> = Tx<'a>;
    async fn begin_sql(&self) -> Result<Tx<'_>, Error> {
        Ok(Tx(self))
    }
}
fn ready<F: Future>(f: F) -> F::Output {
    let mut f = std::pin::pin!(f);
    match f.as_mut().poll(&mut Context::from_waker(Waker::noop())) {
        Poll::Ready(v) => v,
        Poll::Pending => panic!("ready fixture"),
    }
}
fn poll<F: Future<Output = Result<teaql_data_service::MutationResult, SqlExecutorError<Error>>>>(
    f: F,
) -> J {
    let mut f = std::pin::pin!(f);
    match f.as_mut().poll(&mut Context::from_waker(Waker::noop())) {
        Poll::Pending => json!({"pending":true}),
        Poll::Ready(Err(e)) => json!({"error":e.to_string()}),
        Poll::Ready(Ok(v)) => {
            json!({"affected":v.affected_rows,"metadata":metadata(v.metadata),"snapshot":v.persisted_snapshot.map(|s|s.iter().map(|(k,v)|(k.clone(),v.to_json_value())).collect::<serde_json::Map<_,_>>()),"generated":v.generated_values.len()})
        }
    }
}
struct KindCountingDialect {
    base: fixture_sql::FixtureDialect,
    calls: std::sync::atomic::AtomicUsize,
    fail_after: Option<usize>,
}
impl SqlDialect for KindCountingDialect {
    fn kind(&self) -> DatabaseKind {
        let count = self
            .calls
            .fetch_add(1, std::sync::atomic::Ordering::Relaxed)
            + 1;
        if self.fail_after.is_some_and(|limit| count > limit) {
            panic!("fixture kind failure");
        }
        self.base.kind()
    }
    fn quote_ident(&self, name: &str) -> String {
        self.base.quote_ident(name)
    }
    fn placeholder(&self, index: usize) -> String {
        self.base.placeholder(index)
    }
}
fn evaluate(j: J) -> J {
    let e = fixture_sql::entity(&j["entity"]);
    let request = mutation(&j["request"], &e.name);
    let transport = Transport {
        steps: j["steps"].as_array().cloned().unwrap_or_default(),
        calls: Mutex::new(vec![]),
    };
    let dialect = KindCountingDialect {
        base: fixture_sql::FixtureDialect { numbered: true },
        calls: Default::default(),
        fail_after: j["kind_fail_after"].as_u64().map(|n| n as usize),
    };
    let executor = SqlDataServiceExecutor::new(dialect, transport, Schema(Arc::new(e)));
    let events = Arc::new(Mutex::new(vec![]));
    let observer = if j["observe"].as_bool().unwrap_or(true) {
        let events = events.clone();
        Some(Arc::new(move |m| events.lock().unwrap().push(metadata(m)))
            as teaql_data_service::ExecutionObserver)
    } else {
        None
    };
    let mut out = match std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
        if j["transaction"] == true {
            let tx = ready(executor.begin()).unwrap();
            if let Some(g) = j.get("guard") {
                poll(tx.mutate_guarded_observed(
                    GuardedMutationRequest {
                        mutation: request,
                        guard: fixture_sql::expression(g),
                    },
                    observer,
                ))
            } else {
                poll(tx.mutate_observed(request, observer))
            }
        } else if let Some(g) = j.get("guard") {
            poll(executor.mutate_guarded_observed(
                GuardedMutationRequest {
                    mutation: request,
                    guard: fixture_sql::expression(g),
                },
                observer,
            ))
        } else {
            poll(executor.mutate_observed(request, observer))
        }
    })) {
        Ok(v) => v,
        Err(_) => json!({"panic":true}),
    };
    out["calls_before_poll"] = json!(0);
    out["calls"] = json!(*executor.transport.calls.lock().unwrap());
    out["events"] = json!(*events.lock().unwrap());
    if j["observe_kind"] == true {
        out["kind_calls"] = json!(
            executor
                .dialect
                .calls
                .load(std::sync::atomic::Ordering::Relaxed)
        );
    }
    out
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
