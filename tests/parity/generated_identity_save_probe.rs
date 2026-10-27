use domain::AuditedSave;
use serde_json::{Value as Json, json};
use std::io::{self, BufRead};
use std::sync::{Arc, Mutex};
use teaql_core::Entity;
use teaql_runtime::{
    InternalIdGenerator, MutationDecision, MutationPlan, MutationPolicy, MutationPolicyIdentity,
    RuntimeError, SafeAuditEvent, SafeAuditEventSink, SqlLogOptions, UserContext,
};

const SCHEMA: &str = r#"
CREATE TABLE platform_data(id INTEGER PRIMARY KEY,version INTEGER,name TEXT);
INSERT INTO platform_data VALUES(100,1,'platform');
CREATE TABLE business_items(id INTEGER PRIMARY KEY,version INTEGER,platform INTEGER,display_name TEXT,note TEXT);
INSERT INTO business_items VALUES(11,1,100,'PRIVATE-OLD-SENTENCE','PUBLIC-OLD-NOTE');
CREATE TABLE other_table(id INTEGER PRIMARY KEY,version INTEGER,platform INTEGER,display_name TEXT,note TEXT);
INSERT INTO other_table VALUES(22,1,100,'PRIVATE-OLD-SENTENCE','PUBLIC-OLD-NOTE');
CREATE TABLE business_children(id INTEGER PRIMARY KEY,version INTEGER,platform INTEGER,display_name TEXT,note TEXT,password TEXT);
"#;

struct Ids {
    fail: bool,
    calls: Arc<Mutex<Vec<String>>>,
}
impl InternalIdGenerator for Ids {
    fn generate_id(&self, entity: &str) -> Result<u64, RuntimeError> {
        self.calls.lock().unwrap().push(entity.into());
        if self.fail {
            Err(RuntimeError::IdGeneration("fixture ID failure".into()))
        } else {
            Ok(1001)
        }
    }
}

struct Policy {
    deny: bool,
    plans: Arc<Mutex<Vec<String>>>,
}
impl MutationPolicy for Policy {
    fn identity(&self) -> MutationPolicyIdentity {
        MutationPolicyIdentity::new("identity-policy", "1", "identity-fingerprint")
    }
    fn review(&self, _: &UserContext, plan: &MutationPlan) -> MutationDecision {
        self.plans.lock().unwrap().push(format!("{plan:?}"));
        if self.deny {
            MutationDecision::deny("IDENTITY-DENIED", "deny protected identity", ["name"])
        } else {
            MutationDecision::allow()
        }
    }
}

struct Sink {
    fail: bool,
    events: Arc<Mutex<Vec<String>>>,
}
impl SafeAuditEventSink for Sink {
    fn on_safe_event(&self, _: &UserContext, event: &SafeAuditEvent) -> Result<(), RuntimeError> {
        self.events.lock().unwrap().push(format!("{event:?}"));
        if self.fail {
            Err(RuntimeError::Event("fixture audit failure".into()))
        } else {
            Ok(())
        }
    }
}

fn snapshot(database: &rusqlite::Connection) -> Json {
    let mut result = serde_json::Map::new();
    for table in [
        "platform_data",
        "business_items",
        "other_table",
        "business_children",
    ] {
        let mut statement = database
            .prepare(&format!("SELECT * FROM {table} ORDER BY id"))
            .unwrap();
        let names = statement
            .column_names()
            .into_iter()
            .map(str::to_owned)
            .collect::<Vec<_>>();
        let mut cursor = statement.query([]).unwrap();
        let mut rows = Vec::new();
        while let Some(row) = cursor.next().unwrap() {
            let mut values = serde_json::Map::new();
            for (index, name) in names.iter().enumerate() {
                let value = match row.get_ref(index).unwrap() {
                    rusqlite::types::ValueRef::Null => Json::Null,
                    rusqlite::types::ValueRef::Integer(value) => json!(value),
                    rusqlite::types::ValueRef::Text(value) => {
                        json!(std::str::from_utf8(value).unwrap())
                    }
                    _ => panic!("Unexpected fixture database value"),
                };
                values.insert(name.clone(), value);
            }
            rows.push(Json::Object(values));
        }
        result.insert(table.into(), json!(rows));
    }
    Json::Object(result)
}

fn logs(context: &UserContext) -> Json {
    json!(context.sql_logs().iter().map(|entry| json!({
        "log_context": format!("{:?}", entry.log_context),
        "operation": format!("{:?}", entry.operation), "comment": entry.comment,
        "purpose": entry.purpose, "audit_reason": entry.audit_reason,
        "trace_path": format!("{:?}", entry.trace_path), "sql": entry.sql,
        "params": entry.params.iter().map(|value| value.to_json_value()).collect::<Vec<_>>(),
        "params_debug": format!("{:?}", entry.params), "debug_sql": entry.debug_sql,
        "pretty_sql": entry.pretty_sql, "result_count": entry.result_count,
        "result_type": entry.result_type, "affected_rows": entry.affected_rows,
        "result_summary": entry.result_summary,
    })).collect::<Vec<_>>())
}

#[tokio::main(flavor = "current_thread")]
async fn main() {
    for line in io::stdin().lock().lines() {
        let input: Json = serde_json::from_str(&line.unwrap()).unwrap();
        let pool = Arc::new(Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));
        pool.lock().unwrap().execute_batch(SCHEMA).unwrap();
        if input["failure"] == "provider" {
            for table in ["business_items", "other_table", "business_children"] {
                for operation in ["INSERT", "UPDATE"] {
                    pool.lock().unwrap().execute_batch(&format!(
                        "CREATE TRIGGER fail_{table}_{operation} BEFORE {operation} ON {table} BEGIN SELECT RAISE(ABORT,'fixture provider failure'); END"
                    )).unwrap();
                }
            }
        }
        let mut context = domain::service_runtime_from_pool(pool.clone())
            .await
            .unwrap()
            .with_active_root("Platform", 100);
        context.set_trace_id("identity");
        context.set_user_identifier("identity");
        context.set_sql_log_options(if input["logging"] == true {
            SqlLogOptions::all()
        } else {
            SqlLogOptions::disabled()
        });
        let calls = Arc::new(Mutex::new(Vec::new()));
        let plans = Arc::new(Mutex::new(Vec::new()));
        let events = Arc::new(Mutex::new(Vec::new()));
        context.set_internal_id_generator(Ids {
            fail: input["failure"] == "id",
            calls: calls.clone(),
        });
        if input["policy"] != "none" {
            let policy: Arc<dyn MutationPolicy> = Arc::new(Policy {
                deny: input["policy"] == "deny",
                plans: plans.clone(),
            });
            context.set_mutation_policy_registry(move |_: &str| Some(policy.clone()));
        }
        context.set_custom_event_sink(Sink {
            fail: input["failure"] == "audit",
            events: events.clone(),
        });
        macro_rules! save {
            ($entity:expr) => {
                match $entity.audit_as("save PRIVATE-NEW-SENTENCE").save(&context).await {
                    Ok(value) => json!({"ok": true, "entity": value.into_json()}),
                    Err(error) => json!({"ok": false, "error": error.to_string(), "debug": format!("{error:?}")}),
                }
            };
        }
        let outcome = if input["operation"] == "create_child" {
            let mut entity = domain::Q::children()
                .comment("prepare protected creation")
                .purpose("qualify generated identity save")
                .new_entity(&context);
            entity.update_name("PRIVATE-NEW-SENTENCE");
            entity.update_note("PUBLIC-NEW-LONG-NOTE");
            entity.update_secret("CREDENTIAL-SENTINEL");
            save!(entity)
        } else if input["operation"] == "create" {
            let mut entity = domain::Q::items()
                .comment("prepare protected creation")
                .purpose("qualify generated identity save")
                .new_entity(&context);
            entity.update_name("PRIVATE-NEW-SENTENCE");
            entity.update_note("PUBLIC-NEW-LONG-NOTE");
            save!(entity)
        } else {
            match domain::Q::items()
                .select_self_fields()
                .order_by_id_asc()
                .comment("load protected identity")
                .purpose("qualify generated identity save")
                .execute_for_one(&context)
                .await
            {
                Ok(Some(mut entity)) => {
                    entity.update_name("PRIVATE-NEW-SENTENCE");
                    entity.update_note("PUBLIC-NEW-LONG-NOTE");
                    save!(entity)
                }
                Ok(None) => panic!("Missing fixture entity"),
                Err(error) => {
                    json!({"ok": false, "error": error.to_string(), "debug": format!("{error:?}")})
                }
            }
        };
        println!(
            "{}",
            json!({"outcome": outcome, "calls": calls.lock().unwrap().clone(),
            "plans": plans.lock().unwrap().clone(), "events": events.lock().unwrap().clone(),
            "logs": logs(&context), "fixes": format!("{:?}", context.last_fix_evidence()),
            "database": snapshot(&pool.lock().unwrap())})
        );
    }
}
