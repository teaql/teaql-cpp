//! Execute unchanged original event and private privacy modules in a test-only crate.
pub use teaql_runtime::{MutationGovernanceSnapshot, RuntimeError, SqlLogEntry, UserContext};
#[allow(dead_code)]
#[path = "../../../../../.reference/teaql-rs/teaql-runtime/src/event.rs"]
mod event;
pub use event::*;
#[allow(dead_code)]
#[path = "../../../../../.reference/teaql-rs/teaql-runtime/src/log_privacy.rs"]
mod log_privacy;
#[path = "../../../rust_values/src/fixture_value.rs"]
mod fixture_value;
use serde_json::{Value as Json, json};
use std::{collections::BTreeMap, io::{self, BufRead}, sync::Arc};
use teaql_core::{TraceKind, TraceNode, Value};
use teaql_data_service::{SqlIntentRedactions, SqlLogContext, SqlParameterLogPolicy};

fn observe(input: &Json) -> Json {
    let allow = input["allow"].as_bool().unwrap();
    unsafe {
        if allow { std::env::set_var(log_privacy::ENV, log_privacy::ACK); }
        else { std::env::remove_var(log_privacy::ENV); }
    }
    let name = input["field"].as_str().unwrap();
    let mut old = BTreeMap::from([("id".into(), Value::U64(7)), ("note".into(), Value::from("old PRIVATE-OLD PRIVATE-NEW PRIVATE-OPERAND"))]);
    let mut new = BTreeMap::from([("id".into(), Value::U64(7)), ("note".into(), Value::from("new PRIVATE-OLD PRIVATE-NEW PRIVATE-OPERAND"))]);
    for (record, value) in [(&mut old, &input["old"]), (&mut new, &input["new"])] {
        if value["kind"] != "absent" { record.insert(name.into(), fixture_value::value(value)); }
    }
    let mut event = RawAuditEvent::updated_with_old_values("Item", BTreeMap::from([("id".into(), Value::U64(7))]), Some(old), new, vec![name.into(), "note".into()]);
    event.trace_chain = vec![TraceNode::typed(TraceKind::AuditReason, "Item", Some(7), "PRIVATE-OLD PRIVATE-NEW PRIVATE-OPERAND id 7")];
    event.bootstrap_audit = Some(BootstrapAuditIdentity { actor: "PRIVATE-OLD actor".into(), category: "bootstrap".into(), reason: "PRIVATE-NEW PRIVATE-OPERAND reason".into(), resulting_version: Some(4), occurred_at_millis: 17 });
    if input["provenance"].as_bool().unwrap() {
        let log = SqlLogContext { parameter_policies: vec![SqlParameterLogPolicy::Masked], ..Default::default() };
        event.diagnostic_redactions = Some(Arc::new(SqlIntentRedactions::from_bindings(&log, &[Value::from("PRIVATE-OPERAND")], "")));
    }
    let masks = if input["masked"].as_bool().unwrap() { vec![name.into()] } else { vec![] };
    let safe = event.build_safe_event(&masks, input["limit"].as_u64().map(|v| v as usize));
    let projected = log_privacy::audit_event(&event, allow);
    json!({"safe":format!("{safe:?}"), "projected":format!("{projected:?}"), "safe_clone_equal":safe==safe.clone()})
}
fn main() {
    for line in io::stdin().lock().lines() { println!("{}", observe(&serde_json::from_str(&line.unwrap()).unwrap())); }
}
