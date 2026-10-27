use cpp_inheritance_graph_core::Q;
use serde_json::json;
use std::future::Future;
use std::sync::{Arc, Mutex};
use std::task::{Context, Poll, Wake, Waker};
use teaql_core::Entity;
use teaql_runtime::{
    LedgerEntity, MutationDecision, MutationPlan, MutationPolicy, MutationPolicyApproval,
    MutationPolicyApprovalStatus, MutationPolicyIdentity, RawAuditEventKind, RuntimeError,
    SafeAuditEvent, SafeAuditEventSink, UserContext,
};

struct ThreadWake(std::thread::Thread);

impl Wake for ThreadWake {
    fn wake(self: Arc<Self>) {
        self.0.unpark();
    }
}

fn block_on<T>(future: impl Future<Output = T>) -> T {
    let mut future = Box::pin(future);
    let waker = Waker::from(Arc::new(ThreadWake(std::thread::current())));
    let mut context = Context::from_waker(&waker);
    loop {
        match future.as_mut().poll(&mut context) {
            Poll::Ready(result) => return result,
            Poll::Pending => std::thread::park(),
        }
    }
}

struct Sink {
    events: Arc<Mutex<Vec<SafeAuditEvent>>>,
    fail: bool,
}

impl SafeAuditEventSink for Sink {
    fn on_safe_event(&self, _: &UserContext, event: &SafeAuditEvent) -> Result<(), RuntimeError> {
        self.events.lock().unwrap().push(event.clone());
        if self.fail {
            Err(RuntimeError::Event(
                "synthetic audit delivery failed".into(),
            ))
        } else {
            Ok(())
        }
    }
}

struct SchoolPolicy {
    reviews: Arc<Mutex<Vec<String>>>,
    operation_counts: Arc<Mutex<Vec<usize>>>,
    deny: bool,
}

impl MutationPolicy for SchoolPolicy {
    fn identity(&self) -> MutationPolicyIdentity {
        MutationPolicyIdentity::new("school-audit", "1", "source-derived")
    }

    fn review(&self, _: &UserContext, plan: &MutationPlan) -> MutationDecision {
        self.reviews.lock().unwrap().push(plan.request_key.clone());
        self.operation_counts
            .lock()
            .unwrap()
            .push(plan.operations.len());
        if self.deny {
            MutationDecision::deny("SCHOOL-DENIED", "synthetic policy denial", ["name"])
        } else {
            MutationDecision::allow()
        }
    }
}

fn main() {
    let mode = std::env::args()
        .nth(1)
        .unwrap_or_else(|| "success".to_owned());
    assert!(
        [
            "success",
            "write_failure",
            "sink_failure",
            "policy_denied",
            "stale_version",
            "policy_approved",
            "graph_success",
            "graph_policy_denied",
            "graph_policy_approved",
            "graph_write_failure",
            "graph_stale_child",
            "graph_stale_root",
        ]
        .contains(&mode.as_str())
    );
    let pool = Arc::new(Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));
    pool.lock()
        .unwrap()
        .execute_batch(include_str!(
            "../../../support/dynamic_json_inherited_rows.sql"
        ))
        .unwrap();
    let mut user_context = block_on(cpp_inheritance_graph_core::service_runtime_from_pool(
        pool.clone(),
    ))
    .unwrap()
    .with_active_root("Platform", 1);
    let events = Arc::new(Mutex::new(Vec::new()));
    user_context.set_custom_event_sink(Sink {
        events: events.clone(),
        fail: mode == "sink_failure",
    });
    let reviews = Arc::new(Mutex::new(Vec::new()));
    let operation_counts = Arc::new(Mutex::new(Vec::new()));
    if [
        "policy_denied",
        "policy_approved",
        "graph_policy_denied",
        "graph_policy_approved",
    ]
    .contains(&mode.as_str())
    {
        let policy: Arc<dyn MutationPolicy> = Arc::new(SchoolPolicy {
            reviews: reviews.clone(),
            operation_counts: operation_counts.clone(),
            deny: mode != "policy_approved" && mode != "graph_policy_approved",
        });
        user_context.set_mutation_policy_registry(move |_: &str| Some(policy.clone()));
        if mode == "policy_approved" || mode == "graph_policy_approved" {
            user_context.set_mutation_policy_approval_provider(
                |identity: &MutationPolicyIdentity| {
                    Some(MutationPolicyApproval::new(
                        identity.clone(),
                        "reviewer",
                        std::time::UNIX_EPOCH,
                    ))
                },
            );
        }
    }
    let mut school = block_on(
        Q::schools()
            .with_id_is(7_u64)
            .comment("Load School for audited update")
            .purpose("Verify generated audit policy on persistence")
            .execute_for_one(&user_context),
    )
    .unwrap()
    .unwrap();
    school.update_name("PRIVATE-SCHOOL-CANARY");
    if [
        "graph_success",
        "graph_policy_denied",
        "graph_policy_approved",
        "graph_write_failure",
        "graph_stale_child",
        "graph_stale_root",
    ]
    .contains(&mode.as_str())
    {
        let mut enrollment = block_on(
            Q::enrollments()
                .with_id_is(21_u64)
                .comment("Load Enrollment for audited graph update")
                .purpose("Verify generated graph audit policy")
                .execute_for_one(&user_context),
        )
        .unwrap()
        .unwrap();
        enrollment.update_note("PRIVATE-NOTE-CANARY");
        school.include_pending_mutations_from(&enrollment).unwrap();
    }
    if mode == "write_failure" {
        pool.lock()
            .unwrap()
            .execute_batch(
                "CREATE TRIGGER fixture_failure BEFORE UPDATE ON school_data \
                 BEGIN SELECT RAISE(ABORT,'fixture write failed'); END;",
            )
            .unwrap();
    }
    if mode == "graph_write_failure" {
        pool.lock()
            .unwrap()
            .execute_batch(
                "CREATE TRIGGER fixture_graph_failure BEFORE UPDATE ON school_data \
                 BEGIN SELECT RAISE(ABORT,'fixture root write failed'); END;",
            )
            .unwrap();
    }
    if mode == "stale_version" {
        pool.lock()
            .unwrap()
            .execute("UPDATE school_data SET version=version+1 WHERE id=7", [])
            .unwrap();
    }
    if mode == "graph_stale_child" {
        pool.lock()
            .unwrap()
            .execute(
                "UPDATE enrollment_data SET version=version+1 WHERE id=21",
                [],
            )
            .unwrap();
    }
    if mode == "graph_stale_root" {
        pool.lock()
            .unwrap()
            .execute("UPDATE school_data SET version=version+1 WHERE id=7", [])
            .unwrap();
    }
    let saved = block_on(teaql_runtime::save_audited_ledger_entity(
        school.audit_as("Update synthetic School name"),
        &user_context,
    ));
    let (db_name, version): (String, i64) = pool
        .lock()
        .unwrap()
        .query_row(
            "SELECT name,version FROM school_data WHERE id=7",
            [],
            |row| Ok((row.get(0)?, row.get(1)?)),
        )
        .unwrap();
    let (enrollment_note, enrollment_version): (String, i64) = pool
        .lock()
        .unwrap()
        .query_row(
            "SELECT note,version FROM enrollment_data WHERE id=21",
            [],
            |row| Ok((row.get(0)?, row.get(1)?)),
        )
        .unwrap();
    let events = events.lock().unwrap();
    let event = events.iter().find(|event| event.entity == "School");
    if let Some(event) = event {
        assert_eq!(event.kind, RawAuditEventKind::Updated);
    }
    let field = event.and_then(|event| event.fields.iter().find(|field| field.name == "name"));
    let conflict = matches!(&saved, Err(RuntimeError::OptimisticLockConflict { .. }));
    let conflict_entity = match &saved {
        Err(RuntimeError::OptimisticLockConflict { entity, .. }) => Some(entity.as_str()),
        _ => None,
    };
    let policy_error = matches!(&saved, Err(RuntimeError::Policy(_)));
    let fixture_write_failure = saved.as_ref().err().is_some_and(|error| {
        let message = error.to_string();
        message.contains("fixture write failed") || message.contains("fixture root write failed")
    });
    let reviews = reviews.lock().unwrap();
    let operation_counts = operation_counts.lock().unwrap();
    let mut event_entities = events
        .iter()
        .map(|event| event.entity.as_str())
        .collect::<Vec<_>>();
    event_entities.sort_unstable();
    let approved_event = event
        .and_then(|event| event.mutation_governance.as_ref())
        .is_some_and(|state| state.approval_status == MutationPolicyApprovalStatus::Approved);
    let mut approved_event_entities = events
        .iter()
        .filter(|event| {
            event.mutation_governance.as_ref().is_some_and(|state| {
                state.approval_status == MutationPolicyApprovalStatus::Approved
            })
        })
        .map(|event| event.entity.as_str())
        .collect::<Vec<_>>();
    approved_event_entities.sort_unstable();
    println!(
        "{}",
        json!({
            "mode": mode,
            "save_ok": saved.is_ok(),
            "optimistic_conflict": conflict,
            "conflict_entity": conflict_entity,
            "policy_error": policy_error,
            "fixture_write_failure": fixture_write_failure,
            "policy_reviews": reviews.as_slice(),
            "policy_operation_counts": operation_counts.as_slice(),
            "approved_event": approved_event,
            "approved_event_entities": approved_event_entities,
            "event_entities": event_entities,
            "db_name": db_name,
            "version": version,
            "enrollment_note": enrollment_note,
            "enrollment_version": enrollment_version,
            "event_count": events.len(),
            "entity": event.map(|event| event.entity.as_str()),
            "safe_field": field.map(|field| json!({
                "name": field.name,
                "value": field.value,
                "masked": field.masked,
                "truncated": field.truncated,
                "raw_length": field.raw_length,
                "output_length": field.output_length,
                "mask_reason": field.mask_reason,
                "truncate_reason": field.truncate_reason,
            })),
        })
    );
}
