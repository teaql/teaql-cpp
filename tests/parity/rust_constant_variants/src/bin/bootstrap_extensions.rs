use cpp_constants_numeric_core as domain;
use domain::request_support::AuditedSave;
use serde_json::{Value as Json, json};
use std::sync::atomic::{AtomicU64, Ordering};
use std::{
    future::Future,
    io::{self, BufRead},
    sync::{Arc, Mutex},
    task::{Context, Poll, Wake, Waker},
};
use teaql_core::Entity;
use teaql_provider_sqlite::SqliteProviderExt;

struct NoWake;
impl Wake for NoWake {
    fn wake(self: Arc<Self>) {}
}
fn wait<T>(future: impl Future<Output = T>) -> T {
    let mut future = Box::pin(future);
    let waker = Waker::from(Arc::new(NoWake));
    let mut context = Context::from_waker(&waker);
    loop {
        if let Poll::Ready(value) = future.as_mut().poll(&mut context) {
            return value;
        }
    }
}
fn database(connection: &rusqlite::Connection, tables: &[&str]) -> Json {
    let mut out = serde_json::Map::new();
    for table in tables {
        let mut statement = connection
            .prepare(&format!("SELECT * FROM {table} ORDER BY id"))
            .unwrap();
        let names = statement
            .column_names()
            .into_iter()
            .map(str::to_owned)
            .collect::<Vec<_>>();
        let rows = statement
            .query_map([], |row| {
                let mut values = serde_json::Map::new();
                for (index, name) in names.iter().enumerate() {
                    let value = match row.get_ref(index).unwrap() {
                        rusqlite::types::ValueRef::Null => Json::Null,
                        rusqlite::types::ValueRef::Integer(value) => json!(value),
                        rusqlite::types::ValueRef::Real(value) => json!(value),
                        rusqlite::types::ValueRef::Text(value) => {
                            json!(std::str::from_utf8(value).unwrap())
                        }
                        _ => unreachable!(),
                    };
                    values.insert(name.clone(), value);
                }
                Ok(values)
            })
            .unwrap();
        out.insert(
            (*table).into(),
            json!(rows.map(Result::unwrap).collect::<Vec<_>>()),
        );
    }
    let exists = connection
        .query_row(
            "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='teaql_id_space'",
            [],
            |row| row.get::<_, i64>(0),
        )
        .unwrap()
        != 0;
    let levels = if exists {
        let mut statement = connection
            .prepare("SELECT type_name,current_level FROM teaql_id_space ORDER BY type_name")
            .unwrap();
        statement
            .query_map([], |row| {
                Ok(json!([row.get::<_, String>(0)?, row.get::<_, i64>(1)?]))
            })
            .unwrap()
            .map(Result::unwrap)
            .collect::<Vec<_>>()
    } else {
        Vec::new()
    };
    out.insert("ids".into(), json!(levels));
    Json::Object(out)
}

struct ExtensionSettings {
    tag: String,
    delta: i64,
    destroyed: Arc<AtomicU64>,
}
impl Drop for ExtensionSettings {
    fn drop(&mut self) {
        self.destroyed.fetch_add(1, Ordering::SeqCst);
    }
}
struct ExtensionLogic {
    hooks: Arc<Mutex<Vec<Json>>>,
    mode: String,
}
impl domain::PriorityCheckerLogic for ExtensionLogic {
    fn check_and_fix_priority(
        &self,
        user_context: &teaql_runtime::UserContext,
        entity: &mut domain::Priority,
        status: teaql_runtime::CheckObjectStatus,
        location: &teaql_runtime::ObjectLocation,
        results: &mut teaql_runtime::CheckResults,
    ) {
        let settings = user_context
            .require_resource::<ExtensionSettings>()
            .unwrap();
        let rank = entity.rank();
        if self.mode == "fix" {
            entity.update_rank(rank + settings.delta);
        } else if self.mode == "deny" {
            results.push(teaql_runtime::CheckResult::required(
                location.clone().member("rank"),
            ));
        }
        self.hooks.lock().unwrap().push(json!({"hook":"checker","status":format!("{status:?}"),"rank":rank,"after_rank":entity.rank(),"tag":settings.tag}));
    }
}
struct ExtensionBehavior {
    hooks: Arc<Mutex<Vec<Json>>>,
    mode: String,
}
impl ExtensionBehavior {
    fn record(
        &self,
        user_context: &teaql_runtime::UserContext,
        kind: &str,
        command: String,
    ) -> Result<(), teaql_runtime::RuntimeError> {
        let settings = user_context
            .require_resource::<ExtensionSettings>()
            .map_err(|error| teaql_runtime::RuntimeError::Graph(error.to_string()))?;
        self.hooks
            .lock()
            .unwrap()
            .push(json!({"hook":kind,"command":command,"tag":settings.tag}));
        if self.mode == "deny" && kind != "select" {
            Err(teaql_runtime::RuntimeError::Event(
                "fixture behavior denial".into(),
            ))
        } else {
            Ok(())
        }
    }
}
impl teaql_runtime::EntityDataServiceBehavior for ExtensionBehavior {
    fn before_select(
        &self,
        c: &teaql_runtime::UserContext,
        q: &mut teaql_core::SelectQuery,
    ) -> Result<(), teaql_runtime::RuntimeError> {
        self.record(c, "select", format!("{q:?}"))
    }
    fn before_insert(
        &self,
        c: &teaql_runtime::UserContext,
        q: &mut teaql_core::InsertCommand,
    ) -> Result<(), teaql_runtime::RuntimeError> {
        self.record(c, "insert", format!("{q:?}"))
    }
    fn before_update(
        &self,
        c: &teaql_runtime::UserContext,
        q: &mut teaql_core::UpdateCommand,
    ) -> Result<(), teaql_runtime::RuntimeError> {
        self.record(c, "update", format!("{q:?}"))
    }
}

fn observe(input: &Json) -> Json {
    let pool = Arc::new(Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));
    let hooks = Arc::new(Mutex::new(Vec::<Json>::new()));
    let destroyed = Arc::new(AtomicU64::new(0));
    let create = |extensions: bool| {
        let mode = if extensions {
            input["module"].as_str().unwrap()
        } else {
            "combined"
        };
        let mut module = match mode {
            "plain" => domain::module(),
            "checker" => domain::module_with_checkers(),
            "behavior" => domain::module_with_behaviors(),
            _ => domain::module_with_behaviors_and_checkers(),
        };
        if extensions {
            let mode = input["checker"].as_str().unwrap();
            if mode != "none" {
                let checker = domain::PriorityChecker::new(ExtensionLogic {
                    hooks: hooks.clone(),
                    mode: mode.into(),
                });
                module = module.checker(
                    teaql_runtime::TypedEntityChecker::<domain::Priority, _>::new(checker),
                );
            }
            let mode = input["behavior"].as_str().unwrap();
            if mode != "none" {
                module = module.entity_with_behavior::<domain::Priority, _>(ExtensionBehavior {
                    hooks: hooks.clone(),
                    mode: mode.into(),
                });
            }
        }
        let mut user_context = module.into_context();
        if extensions {
            user_context.insert_resource(ExtensionSettings {
                tag: input["tag"].as_str().unwrap().into(),
                delta: input["delta"].as_i64().unwrap(),
                destroyed: destroyed.clone(),
            });
        }
        let transport = domain::DataServiceMutationExecutor::new(pool.clone());
        user_context.set_internal_id_generator(domain::DataServiceIdGenerator::from_executor(
            transport.clone(),
        ));
        user_context.use_sqlite_provider(transport.clone());
        let executor = domain::ServiceRuntimeExecutor::new(transport);
        user_context.register_executor(executor.clone());
        user_context.insert_resource(executor);
        user_context.set_trace_id("bootstrap-extension");
        user_context.set_user_identifier("fixture");
        user_context.enable_all_sql_log();
        user_context
    };
    let state = input["state"].as_str().unwrap();
    if state != "fresh" {
        let warmup = create(false);
        wait(warmup.ensure_schema()).unwrap();
        drop(warmup);
        let sql = match state {
            "existing" => "",
            "drift" => "UPDATE priority_data SET rank=55,version=3 WHERE id=1;",
            "missing" => "DELETE FROM priority_data WHERE id=22;",
            "deleted" => "UPDATE priority_data SET version=-1 WHERE id=1;",
            "extra" => {
                "INSERT INTO priority_data(id,name,code,platform,rank,weight,ratio,cost,day,note,optional_rank,version) VALUES(99,'Custom','CUSTOM',1,42,1.0,1.0,'0.00','1970-01-01','Extra',0,7);"
            }
            _ => panic!("unknown extension state"),
        };
        pool.lock().unwrap().execute_batch(sql).unwrap();
    }
    let mut rounds = Vec::new();
    for _ in 0..input["rounds"].as_u64().unwrap() {
        let user_context = create(true);
        let result = wait(user_context.ensure_schema());
        let snapshot = database(
            &pool.lock().unwrap(),
            &["platform_data", "priority_data", "label_data"],
        );
        let root = user_context
            .require_active_root("Platform")
            .ok()
            .map(|r| r.id);
        let mut round =
            json!({"error":result.err().map(|e|e.to_string()),"database":snapshot,"root":root});
        if input["continuation"] == true {
            let query = domain::Q::priorities()
                .select_self_fields()
                .order_by_id_asc()
                .comment("what: read extension-controlled priorities")
                .purpose("why: verify behavior after schema scope");
            let result = wait(query.execute_for_list(&user_context));
            let mut saved = false;
            let mut save_error = None;
            let mut query_error = None;
            match result {
                Ok(rows) => {
                    if let Some(mut entity) = rows.data.first().cloned() {
                        entity.update_rank(100_i64);
                        match wait(
                            entity
                                .audit_as("Verify custom extension on ordinary update")
                                .save(&user_context),
                        ) {
                            Ok(_) => saved = true,
                            Err(error) => save_error = Some(error.to_string()),
                        }
                    }
                }
                Err(error) => query_error = Some(error.to_string()),
            }
            round["query_error"] = json!(query_error);
            round["saved"] = json!(saved);
            round["save_error"] = json!(save_error);
            round["after_continuation"] = database(
                &pool.lock().unwrap(),
                &["platform_data", "priority_data", "label_data"],
            );
            let service = user_context
                .entity_data_service::<domain::ServiceRuntimeExecutor>("Priority")
                .unwrap();
            let insert = service
                .insert_command()
                .value("id", 99_u64)
                .value("name", "Extension")
                .value("code", "EXTENSION")
                .value("platform_id", 1_u64)
                .value("rank", 100_i64)
                .value("weight", 1.0_f64)
                .value("ratio", 1.0_f32)
                .value("cost", teaql_core::Value::Decimal("0.00".parse().unwrap()))
                .value(
                    "day",
                    teaql_core::Value::Date("1970-01-01".parse().unwrap()),
                )
                .value("note", teaql_core::Value::Null)
                .value("optional_rank", teaql_core::Value::Null);
            let prepared = service.prepare_insert_command(&insert);
            round["prepared_insert"] =
                json!(prepared.as_ref().ok().map(|command| format!("{command:?}")));
            round["prepare_insert_error"] = json!(prepared.err().map(|error| error.to_string()));
            let update = service.update_command(1_u64).value("rank", 200_i64);
            let prepared = service.prepare_update_command(&update);
            round["prepared_update"] =
                json!(prepared.as_ref().ok().map(|command| format!("{command:?}")));
            round["prepare_update_error"] = json!(prepared.err().map(|error| error.to_string()));
        }
        let logs=user_context.sql_logs().iter().map(|e|json!({"sql":e.sql,"comment":e.comment,"purpose":e.purpose,"audit_as":e.audit_reason,"params":e.params.iter().map(|v|format!("{v:?}")).collect::<Vec<_>>()})).collect::<Vec<_>>();
        drop(user_context);
        round["logs"] = json!(logs);
        round["destroyed"] = json!(destroyed.load(Ordering::SeqCst));
        rounds.push(round);
    }
    json!({"rounds":rounds,"hooks":hooks.lock().unwrap().clone()})
}
fn main() {
    for line in io::stdin().lock().lines() {
        let input: Json = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", observe(&input));
    }
}
