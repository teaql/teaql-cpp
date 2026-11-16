use domain::{AuditedSave, Q};
use serde_json::{Value as Json, json};
use std::{
    collections::BTreeMap,
    future::Future,
    io::{self, BufRead},
    pin::pin,
    sync::{Arc, Mutex},
    task::{Context, Poll, Wake, Waker},
};
use teaql_core::Entity;
use teaql_runtime::{
    FixEvidenceSource, InternalIdGenerator, LedgerEntity, MutationDecision, MutationPlan,
    MutationPolicy, MutationPolicyApproval, MutationPolicyIdentity, RuntimeError, SafeAuditEvent,
    SafeAuditEventSink, UserContext,
};

struct Notify;
impl Wake for Notify {
    fn wake(self: Arc<Self>) {}
}
fn wait<T>(future: impl Future<Output = T>) -> T {
    let mut future = pin!(future);
    let waker = Waker::from(Arc::new(Notify));
    let mut context = Context::from_waker(&waker);
    loop {
        if let Poll::Ready(value) = future.as_mut().poll(&mut context) {
            return value;
        }
    }
}

fn rows(database: &rusqlite::Connection, sql: &str) -> Json {
    let mut statement = database.prepare(sql).unwrap();
    let names: Vec<_> = statement
        .column_names()
        .into_iter()
        .map(str::to_owned)
        .collect();
    let mut cursor = statement.query([]).unwrap();
    let mut result = Vec::new();
    while let Some(row) = cursor.next().unwrap() {
        let mut fields = serde_json::Map::new();
        for (index, name) in names.iter().enumerate() {
            let value = match row.get_ref(index).unwrap() {
                rusqlite::types::ValueRef::Null => Json::Null,
                rusqlite::types::ValueRef::Integer(value) => json!(value),
                rusqlite::types::ValueRef::Real(value) => json!(value),
                rusqlite::types::ValueRef::Text(value) => {
                    json!(std::str::from_utf8(value).unwrap())
                }
                _ => panic!("Unexpected scalar fixture SQLite storage"),
            };
            fields.insert(name.clone(), value);
        }
        result.push(Json::Object(fields));
    }
    json!(result)
}

fn snapshot(database: &rusqlite::Connection) -> Json {
    json!({
        "gateways": rows(database, "SELECT id,name,version FROM gateway_data ORDER BY id"),
        "readings": rows(database, "SELECT id,gateway,value,version FROM reading_data ORDER BY id"),
    })
}

fn schema(database: &rusqlite::Connection) -> Json {
    json!({
        "gateway": rows(database, "SELECT name,type,\"notnull\",dflt_value,pk FROM pragma_table_info('gateway_data') ORDER BY cid"),
        "reading": rows(database, "SELECT name,type,\"notnull\",dflt_value,pk FROM pragma_table_info('reading_data') ORDER BY cid"),
        "foreign_keys": rows(database, "SELECT \"table\",\"from\",\"to\",on_update,on_delete FROM pragma_foreign_key_list('reading_data') ORDER BY id,seq"),
    })
}

struct Ids {
    calls: Arc<Mutex<Vec<String>>>,
    counters: Mutex<BTreeMap<String, u64>>,
}
impl InternalIdGenerator for Ids {
    fn generate_id(&self, entity: &str) -> Result<u64, RuntimeError> {
        self.calls.lock().unwrap().push(entity.to_owned());
        let key = teaql_runtime::canonical_id_space_entity(entity);
        let initial = if key == "gateway" { 0 } else { 6 };
        let mut counters = self.counters.lock().unwrap();
        let counter = counters.entry(key).or_insert(initial);
        *counter += 1;
        Ok(*counter)
    }
    fn ensure_floor(&self, entity: &str, floor: u64) -> Result<(), RuntimeError> {
        let mut counters = self.counters.lock().unwrap();
        let counter = counters
            .entry(teaql_runtime::canonical_id_space_entity(entity))
            .or_insert(0);
        *counter = (*counter).max(floor);
        Ok(())
    }
}

struct Policy {
    reviews: Arc<Mutex<Vec<Json>>>,
    deny: bool,
}
impl MutationPolicy for Policy {
    fn identity(&self) -> MutationPolicyIdentity {
        MutationPolicyIdentity::new("scalar-policy", "1", "scalar-policy-v1")
    }
    fn review(&self, _: &UserContext, plan: &MutationPlan) -> MutationDecision {
        let operations: Vec<_> = plan.operations.iter().map(|operation| json!({
            "kind": format!("{:?}", operation.kind),
            "entity": operation.entity.entity,
            "id": operation.entity.id.to_json_value(),
            "original_version": operation.original_version,
            "fields": operation.changed_values.iter().map(|(name,value)| (name.clone(),value.to_json_value())).collect::<serde_json::Map<_,_>>(),
        })).collect();
        self.reviews.lock().unwrap().push(json!({
            "request": plan.request_key, "root": plan.root_entity_type,
            "reason": plan.audit_reason, "operations": operations,
        }));
        if self.deny {
            MutationDecision::deny(
                "SCALAR-DENIED",
                "fixture scalar policy denial",
                std::iter::empty::<String>(),
            )
        } else {
            MutationDecision::allow()
        }
    }
}

struct Sink {
    events: Arc<Mutex<Vec<Json>>>,
    fail: bool,
}
impl SafeAuditEventSink for Sink {
    fn on_safe_event(&self, _: &UserContext, event: &SafeAuditEvent) -> Result<(), RuntimeError> {
        let fields: Vec<_> = event
            .fields
            .iter()
            .map(|field| {
                json!({
                    "name": field.name, "value": field.value,
                    "masked": field.masked, "truncated": field.truncated,
                })
            })
            .collect();
        let trace: Vec<_> = event
            .trace_chain
            .iter()
            .map(|node| {
                json!({
                    "kind": format!("{:?}", node.kind), "entity": node.entity_type,
                    "id": node.entity_id, "comment": node.comment,
                })
            })
            .collect();
        self.events.lock().unwrap().push(json!({
            "entity": event.entity, "id": event.entity_id, "kind": format!("{:?}", event.kind),
            "fields": fields, "trace": trace,
            "approved": event.mutation_governance.as_ref().is_some_and(|value| value.approval_status == teaql_runtime::MutationPolicyApprovalStatus::Approved),
        }));
        if self.fail {
            Err(RuntimeError::Event(
                "fixture scalar audit delivery failed".into(),
            ))
        } else {
            Ok(())
        }
    }
}

fn fixes(user_context: &UserContext) -> Json {
    json!(user_context.last_fix_evidence().iter().map(|evidence| json!({
        "entity": evidence.entity_type, "path": evidence.model_path,
        "source": match evidence.source { FixEvidenceSource::Context => "Context", FixEvidenceSource::Clock => "Clock" },
        "label": evidence.source_label,
    })).collect::<Vec<_>>())
}

fn outcome<T>(result: Result<T, RuntimeError>) -> Json {
    if let Err(error) = &result {
        eprintln!("{}", json!({"error": error.to_string()}));
    }
    json!({
        "ok": result.is_ok(), "policy_error": matches!(&result, Err(RuntimeError::Policy(_))),
        "conflict": matches!(&result, Err(RuntimeError::OptimisticLockConflict { .. })),
        "committed_error": matches!(&result, Err(RuntimeError::AuditAfterCommit { .. })),
    })
}

fn observe(input: &Json) -> Json {
    let pool = Arc::new(Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));
    let mut user_context = wait(domain::service_runtime_from_pool(pool.clone()))
        .unwrap()
        .with_active_root("Gateway", 1);
    let calls = Arc::new(Mutex::new(Vec::new()));
    user_context.set_internal_id_generator(Ids {
        calls: calls.clone(),
        counters: Mutex::new(BTreeMap::new()),
    });
    wait(user_context.ensure_schema()).unwrap();
    let first = snapshot(&pool.lock().unwrap());
    wait(user_context.ensure_schema()).unwrap();
    let second = snapshot(&pool.lock().unwrap());
    let mut output = json!({"schema": schema(&pool.lock().unwrap()), "bootstrap_first": first, "bootstrap_second": second});
    let mode = input["mode"].as_str().unwrap();
    calls.lock().unwrap().clear();
    let mut log_start = user_context.sql_logs().len();
    if mode != "create" {
        let mut entity = Q::readings()
            .comment("Stage typed scalar seed")
            .purpose("Prepare governed scalar fixture")
            .new_entity(&user_context);
        let mut desired = entity.value();
        desired = serde_json::from_value(input["value"].clone()).unwrap();
        entity.update_value(desired);
        output["seed"] = outcome(wait(
            entity
                .audit_as("Create typed scalar seed")
                .save(&user_context),
        ));
        output["seed_fixes"] = fixes(&user_context);
        calls.lock().unwrap().clear();
        log_start = user_context.sql_logs().len();
    }
    let reviews = Arc::new(Mutex::new(Vec::new()));
    let events = Arc::new(Mutex::new(Vec::new()));
    let policy: Arc<dyn MutationPolicy> = Arc::new(Policy {
        reviews: reviews.clone(),
        deny: mode == "deny",
    });
    user_context.set_mutation_policy_registry(move |_: &str| Some(policy.clone()));
    if mode == "approved" {
        user_context.set_mutation_policy_approval_provider(|identity: &MutationPolicyIdentity| {
            Some(MutationPolicyApproval::new(
                identity.clone(),
                "fixture-reviewer",
                std::time::UNIX_EPOCH,
            ))
        });
    }
    user_context.set_custom_event_sink(Sink {
        events: events.clone(),
        fail: mode == "sink_fail",
    });
    if mode == "create" {
        let mut entity = Q::readings()
            .comment("Stage typed scalar creation")
            .purpose("Qualify generated scalar persistence")
            .new_entity(&user_context);
        let mut desired = entity.value();
        desired = serde_json::from_value(input["value"].clone()).unwrap();
        entity.update_value(desired);
        output["outcome"] = outcome(wait(
            entity
                .audit_as("Save typed scalar creation")
                .save(&user_context),
        ));
    } else {
        let selected = wait(
            Q::readings()
                .select_self_fields()
                .with_id_is(7_u64)
                .comment("Load typed scalar for update")
                .purpose("Qualify generated scalar persistence")
                .execute_for_one(&user_context),
        );
        match selected {
            Err(error) => {
                eprintln!("{}", error);
                output["outcome"] = json!({"stage": "load", "ok": false});
            }
            Ok(None) => panic!("Saved scalar seed row is absent"),
            Ok(Some(mut entity)) => {
                let mut desired = entity.value();
                desired = serde_json::from_value(input["next_value"].clone()).unwrap();
                entity.update_value(desired);
                if mode == "update" {
                    output["outcome"] = outcome(wait(
                        entity
                            .audit_as("Save typed scalar update")
                            .save(&user_context),
                    ));
                } else {
                    let mut root = wait(
                        Q::gateways()
                            .select_self_fields()
                            .with_id_is(1_u64)
                            .comment("Load scalar aggregate root")
                            .purpose("Qualify generated scalar graph Save")
                            .execute_for_one(&user_context),
                    )
                    .unwrap()
                    .unwrap();
                    root.update_name("UPDATED-ROOT");
                    root.include_pending_mutations_from(&entity).unwrap();
                    if mode == "stale_root" || mode == "stale_child" {
                        pool.lock()
                            .unwrap()
                            .execute_batch(if mode == "stale_root" {
                                "UPDATE gateway_data SET version=version+1 WHERE id=1"
                            } else {
                                "UPDATE reading_data SET version=version+1 WHERE id=7"
                            })
                            .unwrap();
                    }
                    if mode == "fail_root" || mode == "fail_child" {
                        pool.lock().unwrap().execute_batch(if mode == "fail_root" { "CREATE TRIGGER fixture_failure BEFORE UPDATE ON gateway_data BEGIN SELECT RAISE(ABORT,'fixture root update failed'); END" } else { "CREATE TRIGGER fixture_failure BEFORE UPDATE ON reading_data BEGIN SELECT RAISE(ABORT,'fixture child update failed'); END" }).unwrap();
                    }
                    output["outcome"] = outcome(wait(
                        root.audit_as("Save typed scalar aggregate")
                            .save(&user_context),
                    ));
                }
            }
        }
    }
    let logs: Vec<_> = user_context.sql_logs().iter().skip(log_start).map(|entry| json!({
        "sql": entry.sql, "comment": entry.comment, "purpose": entry.purpose,
        "reason": entry.audit_reason, "params": entry.params.iter().map(|value| value.to_json_value()).collect::<Vec<_>>(),
    })).collect();
    output["database"] = snapshot(&pool.lock().unwrap());
    output["events"] = json!(events.lock().unwrap().clone());
    output["plans"] = json!(reviews.lock().unwrap().clone());
    output["ids"] = json!(calls.lock().unwrap().clone());
    output["fixes"] = fixes(&user_context);
    output["logs"] = json!(logs);
    output
}

fn main() {
    for line in io::stdin().lock().lines() {
        let input: Json = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", observe(&input));
    }
}
