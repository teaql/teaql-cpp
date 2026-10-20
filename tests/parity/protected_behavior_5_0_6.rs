use cpp_checker_fixture_core::{Note, NoteBehavior, Tenant};
use serde_json::{json, Value as Json};
use std::{future::Future, io::{self, BufRead}, sync::{Arc, Mutex, atomic::{AtomicBool, Ordering}}, task::{Context, Poll, Wake, Waker}};
use teaql_core::{DeleteCommand, RecoverCommand, TraceNode};
use teaql_provider_sqlite::SqliteProviderExt;
use teaql_runtime::{EntityDataService, EntityDataServiceBehavior, RawAuditEvent, RawAuditEventSink,
    RequestPolicy, RuntimeError, RuntimeModule, UserContext};

struct NoWake;
impl Wake for NoWake { fn wake(self: Arc<Self>) {} }
fn wait<T>(future: impl Future<Output = T>) -> T {
    let mut future = Box::pin(future);
    let waker = Waker::from(Arc::new(NoWake));
    let mut context = Context::from_waker(&waker);
    loop { if let Poll::Ready(result) = future.as_mut().poll(&mut context) { return result; } }
}
struct State { calls: Mutex<Vec<Json>>, events: Mutex<Vec<Json>>, version: i64, sink_fail: bool, ready: AtomicBool }
struct Behavior { base: NoteBehavior, state: Arc<State>, mode: String }
impl Behavior {
    fn call(&self, kind: &str, version: Option<i64>) -> Result<(), RuntimeError> {
        self.state.calls.lock().unwrap().push(json!({"stage":"behavior","kind":kind,"version":version}));
        if self.mode == "deny" { Err(RuntimeError::Behavior("business behavior denied".into())) } else { Ok(()) }
    }
}
impl EntityDataServiceBehavior for Behavior {
    fn before_delete(&self, context: &UserContext, command: &mut DeleteCommand) -> Result<(), RuntimeError> {
        self.base.before_delete(context, command)?;
        self.call("delete", command.expected_version)?;
        if self.mode == "rewrite" { command.expected_version = Some(self.state.version); }
        Ok(())
    }
    fn before_recover(&self, context: &UserContext, command: &mut RecoverCommand) -> Result<(), RuntimeError> {
        self.base.before_recover(context, command)?;
        self.call("recover", Some(command.expected_version))?;
        if self.mode == "rewrite" { command.expected_version = self.state.version; }
        Ok(())
    }
}
struct Policy { state: Arc<State>, mode: String }
impl Policy {
    fn call(&self, kind: &str, version: Option<i64>) -> Result<(), RuntimeError> {
        self.state.calls.lock().unwrap().push(json!({"stage":"policy","kind":kind,"version":version}));
        if self.mode == "deny" { Err(RuntimeError::Policy("business policy denied".into())) } else { Ok(()) }
    }
}
impl RequestPolicy for Policy {
    fn enforce_delete(&self, _: &UserContext, command: &mut DeleteCommand) -> Result<(), RuntimeError> {
        self.call("delete", command.expected_version)?;
        if self.mode == "rewrite" { command.expected_version = Some(self.state.version); }
        Ok(())
    }
    fn enforce_recover(&self, _: &UserContext, command: &mut RecoverCommand) -> Result<(), RuntimeError> {
        self.call("recover", Some(command.expected_version))?;
        if self.mode == "rewrite" { command.expected_version = self.state.version; }
        Ok(())
    }
}
struct Sink(Arc<State>);
impl RawAuditEventSink for Sink {
    fn on_event(&self, _: &UserContext, event: &RawAuditEvent) -> Result<(), RuntimeError> {
        if !self.0.ready.load(Ordering::SeqCst) { return Ok(()); }
        self.0.calls.lock().unwrap().push(json!({"stage":"event","kind":format!("{:?}",event.kind)}));
        self.0.events.lock().unwrap().push(json!({"kind":format!("{:?}",event.kind),"entity":event.entity,
            "values":event.values.iter().map(|(key,value)|(key.clone(),value.to_json_value())).collect::<serde_json::Map<_,_>>(),
            "old":event.old_values.is_some(),"new":event.new_values.is_some(),
            "trace":event.trace_chain.iter().map(|node|node.comment.clone()).collect::<Vec<_>>()}));
        if self.0.sink_fail { Err(RuntimeError::Event("business event delivery failed".into())) } else { Ok(()) }
    }
}
fn observe(input: &Json) -> Json {
    let recover = input["operation"] == "recover";
    let version = if recover { -3 } else { 3 };
    let state = Arc::new(State { calls: Mutex::new(Vec::new()), events: Mutex::new(Vec::new()),
        version, sink_fail: input["sink_fail"] == true, ready: AtomicBool::new(false) });
    let module = RuntimeModule::new().entity::<Tenant>().entity_with_behavior::<Note,_>(Behavior {
        base: NoteBehavior, state: state.clone(), mode: input["behavior"].as_str().unwrap().into() })
        .event_sink(Sink(state.clone()));
    let mut user_context = module.into_context();
    user_context.disable_sql_log();
    let transport = teaql_provider_sqlite::SqliteMutationExecutor::from_connection(rusqlite::Connection::open_in_memory().unwrap());
    user_context.use_sqlite_provider(transport.clone());
    user_context.register_executor(cpp_checker_fixture_core::ServiceRuntimeExecutor::new(transport.clone()));
    wait(user_context.ensure_schema()).unwrap();
    let connection = transport.connection();
    {
        let connection = connection.lock().unwrap();
        connection.execute_batch("INSERT INTO tenant_data(id,name,version) VALUES(42,'trusted',3);").unwrap();
        if input["state"] != "missing" {
            connection.execute(&format!("INSERT INTO note_data(id,name,tenant,created_at,updated_at,version) VALUES(7,'stored',42,111,222,{version})"), []).unwrap();
        }
        if input["state"] == "trigger" {
            let operation = if input["operation"] == "hard_delete" { "DELETE" } else { "UPDATE" };
            connection.execute_batch(&format!("CREATE TRIGGER fixture_failure BEFORE {operation} ON note_data BEGIN SELECT RAISE(ABORT,'fixture write failed'); END;")).unwrap();
        }
    }
    user_context.set_request_policy(Policy { state: state.clone(), mode: input["policy"].as_str().unwrap().into() });
    state.ready.store(true, Ordering::SeqCst);
    user_context.enable_mutation_sql_log();
    let executor = user_context.require_resource::<cpp_checker_fixture_core::ServiceRuntimeExecutor>().unwrap();
    let mut service = EntityDataService::for_executor(&user_context, "Note", executor);
    if input["trace"] == true { service = service.with_trace_context(vec![TraceNode::new("Note",Some(7),"scope trace")]); }
    let expected = if input["state"] == "stale" { version + if recover { -1 } else { 1 } } else { version };
    let reason = match input["intent"].as_str().unwrap() { "missing" => None, "blank" => Some("  "), _ => Some("Review protected business deletion/recovery") };
    let result = if recover {
        let mut command = RecoverCommand::new("Note",7_u64,expected);
        if input["trace"] == true { command.trace_chain.push(TraceNode::new("Note",Some(7),"command trace")); }
        wait(service.__cpp_protected_recover(&command, reason))
    } else {
        let mut command = DeleteCommand::new("Note",7_u64).expected_version(expected);
        if input["operation"] == "hard_delete" { command = command.hard_delete(); }
        if input["trace"] == true { command.trace_chain.push(TraceNode::new("Note",Some(7),"command trace")); }
        wait(service.__cpp_protected_delete(&command, reason))
    };
    let outcome = match result { Ok(rows)=>json!({"rows":rows}),Err(error)=>json!({"error":error.to_string()}) };
    let database = {
        let connection = connection.lock().unwrap();
        let mut statement = connection.prepare("SELECT id,name,version FROM note_data ORDER BY id").unwrap();
        statement.query_map([],|row|Ok(json!([row.get::<_,i64>(0)?,row.get::<_,String>(1)?,row.get::<_,i64>(2)?])))
            .unwrap().map(Result::unwrap).collect::<Vec<_>>()
    };
    let logs = user_context.sql_logs().iter().map(|log|json!({"sql":log.sql,"comment":log.comment,
        "purpose":log.purpose,"audit":log.audit_reason,"operation":format!("{:?}",log.operation),
        "summary":log.result_summary,"rows":log.affected_rows})).collect::<Vec<_>>();
    json!({"outcome":outcome,"calls":state.calls.lock().unwrap().clone(),"events":state.events.lock().unwrap().clone(),"database":database,"logs":logs})
}
fn main() {
    for line in io::stdin().lock().lines() {
        let input:Json=serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}",observe(&input));
    }
}
