include!("topology_calls.rs");
use teaql_core::Entity;
use serde_json::{Value as Json, json};
use std::{
    future::Future,
    io::{self, BufRead},
    sync::{Arc, Mutex},
    task::{Context, Poll, Wake, Waker},
};
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
fn table(connection: &rusqlite::Connection, name: &str) -> Json {
    let exists = connection
        .query_row(
            "SELECT count(*) FROM sqlite_master WHERE type='table' AND name=?1",
            [name],
            |row| row.get::<_, i64>(0),
        )
        .unwrap()
        != 0;
    if !exists {
        return Json::Null;
    }
    let mut statement = connection
        .prepare(&format!("SELECT * FROM {name} ORDER BY 1"))
        .unwrap();
    let names = statement
        .column_names()
        .into_iter()
        .map(str::to_owned)
        .collect::<Vec<_>>();
    let values = statement
        .query_map([], |row| {
            let mut result = serde_json::Map::new();
            for (index, name) in names.iter().enumerate() {
                let value = match row.get_ref(index).unwrap() {
                    rusqlite::types::ValueRef::Null => Json::Null,
                    rusqlite::types::ValueRef::Integer(v) => json!(v),
                    rusqlite::types::ValueRef::Real(v) => json!(v),
                    rusqlite::types::ValueRef::Text(v) => json!(std::str::from_utf8(v).unwrap()),
                    _ => unreachable!(),
                };
                result.insert(name.clone(), value);
            }
            Ok(Json::Object(result))
        })
        .unwrap()
        .map(Result::unwrap)
        .collect::<Vec<_>>();
    json!(values)
}
fn snapshot(pool: &Arc<Mutex<rusqlite::Connection>>) -> Json {
    let connection = pool.lock().unwrap();
    json!({"platform":table(&connection,"platform_data"),"status":table(&connection,"status_data"),"category":table(&connection,"category_data"),"ids":table(&connection,"teaql_id_space")})
}

fn expression_read(read: impl FnOnce() -> Option<String>) -> (Option<String>, &'static str) {
    match std::panic::catch_unwind(std::panic::AssertUnwindSafe(read)) {
        Ok(value) => {
            let state = if value.is_some() { "Value" } else { "Null" };
            (value, state)
        }
        Err(error) => {
            let text = error
                .downcast_ref::<String>()
                .map(String::as_str)
                .or_else(|| error.downcast_ref::<&str>().copied());
            if text.is_some_and(|text| {
                text.contains("[Coding Logic Bug]")
                    && text.contains("missing_preload:")
                    && text.contains("severity: \"error\"")
            }) {
                (None, "NotLoaded")
            } else {
                std::panic::resume_unwind(error)
            }
        }
    }
}

struct Logic {
    model: &'static str,
    mode: String,
    hooks: Arc<Mutex<Vec<Json>>>,
}
macro_rules! checker {
    ($trait:ident,$method:ident,$entity:ty) => {
        impl domain::$trait for Logic {
            fn $method(&self, _user_context:&teaql_runtime::UserContext, entity:&mut $entity,
                status:teaql_runtime::CheckObjectStatus, location:&teaql_runtime::ObjectLocation,
                results:&mut teaql_runtime::CheckResults) {
                let before=entity.code().to_owned();
                if self.mode=="fix" { entity.update_code(format!("{before}-fixed")); }
                if self.mode=="deny" { results.push(teaql_runtime::CheckResult::required(location.clone().member("code"))); }
                self.hooks.lock().unwrap().push(json!({"kind":"checker","model":self.model,
                    "status":format!("{status:?}"),"before":before,"after":entity.code()}));
            }
        }
    };
}
checker!(CategoryCheckerLogic,check_and_fix_category,domain::Category);
checker!(StatusCheckerLogic,check_and_fix_status,domain::Status);

struct Behavior { hooks:Arc<Mutex<Vec<Json>>>, mode:String }
impl teaql_runtime::EntityDataServiceBehavior for Behavior {
    fn before_select(&self,_:&teaql_runtime::UserContext,query:&mut teaql_core::SelectQuery)
        -> Result<(),teaql_runtime::RuntimeError> {
        self.hooks.lock().unwrap().push(json!({"kind":"select","query":format!("{query:?}")}));
        if self.mode=="deny" {Err(teaql_runtime::RuntimeError::Event("topology select denial".to_owned()))}
        else {Ok(())}
    }
}

fn observe(input: &Json) -> Json {
    let pool = Arc::new(Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));
    let hooks=Arc::new(Mutex::new(Vec::new()));
    let create = |extensions:bool| {
        let mut module = match input["module"].as_str().unwrap() {
            "plain" => domain::module(),
            "checker" => domain::module_with_checkers(),
            "behavior" => domain::module_with_behaviors(),
            _ => domain::module_with_behaviors_and_checkers(),
        };
        if extensions {
            let mode=input["checker"].as_str().unwrap();
            let target=input["target"].as_str().unwrap();
            if mode!="none" && target=="category" {
                module=module.checker(teaql_runtime::TypedEntityChecker::<domain::Category,_>::new(
                    domain::CategoryChecker::new(Logic{model:"Category",mode:mode.into(),hooks:hooks.clone()})));
            } else if mode!="none" {
                module=module.checker(teaql_runtime::TypedEntityChecker::<domain::Status,_>::new(
                    domain::StatusChecker::new(Logic{model:"Status",mode:mode.into(),hooks:hooks.clone()})));
            }
            let mode=input["behavior"].as_str().unwrap();
            if mode!="none" {
                module=module.entity_with_behavior::<domain::Status,_>(Behavior{hooks:hooks.clone(),mode:mode.into()});
            }
        }
        let mut user_context = module.into_context();
        let transport = domain::DataServiceMutationExecutor::new(pool.clone());
        user_context.set_internal_id_generator(domain::DataServiceIdGenerator::from_executor(
            transport.clone(),
        ));
        user_context.use_sqlite_provider(transport.clone());
        let executor = domain::ServiceRuntimeExecutor::new(transport);
        user_context.register_executor(executor.clone());
        user_context.insert_resource(executor);
        user_context.set_trace_id("constant-topology");
        user_context.set_user_identifier("fixture");
        if input["logging"] == true {
            user_context.enable_all_sql_log();
        } else {
            user_context.disable_sql_log();
        }
        user_context
    };
    let state = input["state"].as_str().unwrap();
    let mut warmup_error = Json::Null;
    if state != "fresh" {
        let user_context = create(false);
        if let Err(error) = wait(user_context.ensure_schema()) {
            warmup_error = json!(error.to_string());
        }
        drop(user_context);
        if warmup_error.is_null() {
            let sql = match state {
                "existing" => "",
                "drift" => {
                    "UPDATE category_data SET code='STALE',version=3 WHERE id=(SELECT MIN(id) FROM category_data); UPDATE status_data SET code='STALE',version=3 WHERE id=(SELECT MIN(id) FROM status_data)"
                }
                "missing" => {
                    "DELETE FROM status_data WHERE id=12; DELETE FROM category_data WHERE id=22"
                }
                "deleted" => {
                    "UPDATE category_data SET version=-1 WHERE id=(SELECT MIN(id) FROM category_data); UPDATE status_data SET version=-1 WHERE id=11"
                }
                "extra" => extra_sql(),
                _ => panic!("Unknown constant topology state"),
            };
            pool.lock().unwrap().execute_batch(sql).unwrap();
        }
    }
    let mut rounds = Vec::new();
    for _ in 0..input["rounds"].as_u64().unwrap() {
        let before = snapshot(&pool);
        let user_context = create(true);
        let error = wait(user_context.ensure_schema())
            .err()
            .map(|error| error.to_string());
        let root = user_context
            .require_active_root("Platform")
            .map(|root| json!(root.id))
            .unwrap_or_else(|error| json!(error.to_string()));
        let query = domain::Q::statuses()
            .select_self_fields()
            .select_category_with(category_request())
            .order_by_id_asc()
            .comment("what: inspect constant topology rows")
            .purpose("why: verify generated identifier and seed behavior");
        let (values, query_error) = match wait(query.execute_for_list(&user_context)) {
            Ok(rows) => (rows.data.iter().map(|entity| {
                let (category_code, category_state) = expression_read(|| domain::E::status(entity).get_category().get_code().eval());
                let (parent_code, parent_state) = parent_read(entity);
                json!({"id": entity.id(), "version": entity.version(), "code": entity.code(),
                    "category_id":entity.category_id(), "category_code":category_code, "category_state":category_state,
                    "parent_code":parent_code, "parent_state":parent_state, "expression": domain::E::status(entity).get_code().unwrap()})
            }).collect::<Vec<_>>(), None),
            Err(error) => (Vec::new(), Some(error.to_string())),
        };
        let after = snapshot(&pool);
        let logs = user_context.sql_logs().iter().map(|entry| json!({"sql": entry.sql, "comment": entry.comment, "purpose": entry.purpose, "audit_as": entry.audit_reason, "params": entry.params.iter().map(|value| format!("{value:?}")).collect::<Vec<_>>()})).collect::<Vec<_>>();
        drop(user_context);
        let after_close = snapshot(&pool);
        rounds.push(json!({"before": before, "after": after, "after_close": after_close, "error": error, "root": root, "values": values, "query_error": query_error, "logs": logs}));
    }
    let hook_values=hooks.lock().unwrap().clone();
    json!({"warmup_error": warmup_error, "rounds": rounds,"hooks":hook_values})
}

fn main() {
    for line in io::stdin().lock().lines() {
        let input: Json = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", observe(&input));
    }
}
