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
        if allow {
            std::env::set_var(log_privacy::ENV, log_privacy::ACK);
        } else {
            std::env::remove_var(log_privacy::ENV);
        }
    }
    let mut fields = BTreeMap::from([
        ("note".into(), Value::from("note PRIVATE-OPERAND PRIVATE-TOKEN")),
        ("#extension".into(), Value::from("PRIVATE-DYNAMIC")),
        ("version".into(), Value::I64(3)),
    ]);
    if input["id"]["kind"] != "absent" {
        fields.insert("id".into(), fixture_value::value(&input["id"]));
    }
    let mut event = RawAuditEvent::created("Item", fields);
    event.old_values = Some(BTreeMap::from([("id".into(), Value::U64(999))]));
    event.trace_chain = vec![TraceNode::typed(
        TraceKind::AuditReason, "Parent", Some(123),
        "PRIVATE-OPERAND PRIVATE-TOKEN PRIVATE-DYNAMIC parent 123 old 999",
    )];
    if input["bootstrap"].as_bool().unwrap() {
        event.bootstrap_audit = Some(BootstrapAuditIdentity {
            actor: "actor PRIVATE-TOKEN".into(), category: "bootstrap".into(),
            reason: "reason PRIVATE-OPERAND PRIVATE-DYNAMIC".into(),
            resulting_version: Some(3), occurred_at_millis: 17,
        });
    }
    if input["provenance"].as_bool().unwrap() {
        let source = SqlLogContext {
            parameter_policies: vec![SqlParameterLogPolicy::Masked, SqlParameterLogPolicy::Credential],
            ..Default::default()
        };
        event.diagnostic_redactions = Some(Arc::new(SqlIntentRedactions::from_bindings(
            &source, &[Value::from("PRIVATE-OPERAND"), Value::from("PRIVATE-TOKEN")], "",
        )));
    }
    let safe = event.build_safe_event(&[], None);
    let projected = log_privacy::audit_event(&event, allow);
    json!({"safe":format!("{safe:?}"),"safe_clone_equal":safe==safe.clone(),
           "projected":format!("{projected:?}"),
           "raw_has_provenance":event.diagnostic_redactions.is_some(),
           "projected_has_provenance":projected.diagnostic_redactions.is_some()})
}

fn main() {
    for line in io::stdin().lock().lines() {
        println!("{}", observe(&serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
