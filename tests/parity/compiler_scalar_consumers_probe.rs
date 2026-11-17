use domain::{Reading, ReadingChecker, ReadingCheckerLogic};
use serde_json::{Value as Json, json};
use std::{
    collections::BTreeMap,
    future::Future,
    io::{self, BufRead},
    sync::{Arc, Mutex},
    task::{Context, Poll, Wake, Waker},
};
use teaql_core::{CompactRow, Entity, Value, eval::LoadState};
use teaql_runtime::{
    CheckObjectStatus, CheckResults, FixEvidenceSource, JsonFieldNamingProfile, LedgerEntity,
    ObjectLocation, TypedChecker, UserContext,
};

struct NoWake;
impl Wake for NoWake {
    fn wake(self: Arc<Self>) {}
}
fn wait<T>(future: impl Future<Output = T>) -> T {
    let mut future = Box::pin(future);
    let waker = Waker::from(Arc::new(NoWake));
    let mut context = Context::from_waker(&waker);
    loop {
        if let Poll::Ready(result) = future.as_mut().poll(&mut context) {
            return result;
        }
    }
}

struct Logic {
    input: Json,
    calls: Arc<Mutex<Vec<Json>>>,
}
impl ReadingCheckerLogic for Logic {
    fn check_and_fix_reading(
        &self,
        _: &UserContext,
        entity: &mut Reading,
        _: CheckObjectStatus,
        location: &ObjectLocation,
        results: &mut CheckResults,
    ) {
        self.calls.lock().unwrap().push(json!({"before":entity.value(), "gateway":entity.gateway_id(),
                                              "errors":results.len(), "location":location.model_path()}));
        if self.input["logic"] == "fix" {
            let mut desired = entity.value();
            desired = serde_json::from_value(self.input["next_value"].clone()).unwrap();
            entity.update_value(desired);
        }
        if self.input["logic"] == "reject" {
            self.required(false, "value_issue", location, results);
        }
    }
}

fn check(user_context: &UserContext, input: &Json) -> Json {
    let status = match input["status"].as_str().unwrap() {
        "create" => CheckObjectStatus::Create,
        "update" => CheckObjectStatus::Update,
        _ => CheckObjectStatus::Unknown,
    };
    let mut entity = Reading::from_compact_row(CompactRow::from_map(BTreeMap::from([
        ("id".into(), Value::U64(7)),
        (
            "gateway_id".into(),
            Value::U64(input["gateway"].as_u64().unwrap()),
        ),
        (
            "version".into(),
            Value::I64(if status == CheckObjectStatus::Create {
                0
            } else {
                3
            }),
        ),
    ])))
    .unwrap();
    let mut desired = entity.value();
    desired = serde_json::from_value(input["value"].clone()).unwrap();
    entity.update_value(desired);
    let fields: Vec<String> = ["id", "value", "gateway_id", "version"]
        .into_iter()
        .filter(|name| match input["loaded"].as_str().unwrap() {
            "identity" => matches!(*name, "id" | "version"),
            "no_value" => *name != "value",
            _ => true,
        })
        .map(str::to_owned)
        .collect();
    entity.set_load_state(LoadState::Partial(fields.into_iter().collect()));
    if status == CheckObjectStatus::Create {
        entity
            .entity_runtime_state()
            .unwrap()
            .mark_as_new(entity.entity_key());
    }
    let calls = Arc::new(Mutex::new(Vec::new()));
    let mut results = Vec::new();
    ReadingChecker::new(Logic {
        input: input.clone(),
        calls: calls.clone(),
    })
    .check_and_fix_typed(
        user_context,
        &mut entity,
        status,
        &ObjectLocation::array_root(2).member("reading_info"),
        &mut results,
    );
    let diagnostics: Vec<_> = [
        JsonFieldNamingProfile::CamelCase,
        JsonFieldNamingProfile::SnakeCase,
        JsonFieldNamingProfile::PascalCase,
    ]
    .into_iter()
    .map(|profile| {
        results
            .iter()
            .map(|value| value.to_wire(profile))
            .collect::<Vec<_>>()
    })
    .collect();
    let fixes: Vec<_> = user_context.last_fix_evidence().iter().map(|evidence|json!({
        "entity":evidence.entity_type,"path":evidence.model_path,
        "source":match evidence.source {FixEvidenceSource::Context=>"Context",FixEvidenceSource::Clock=>"Clock"},
        "label":evidence.source_label,
    })).collect();
    let loaded = ["id", "value", "gateway_id", "version"].map(|name| entity.is_loaded(name));
    let output = json!({"value":entity.value(),"gateway":entity.gateway_id(),
        "loaded":loaded,
        "diagnostics":diagnostics,"fixes":fixes,"calls":calls.lock().unwrap().clone()});
    output
}

fn readings(database: &rusqlite::Connection) -> Json {
    let mut statement = database
        .prepare("SELECT id,gateway,value,version FROM reading_data ORDER BY id")
        .unwrap();
    let names = statement
        .column_names()
        .iter()
        .map(|name| name.to_string())
        .collect::<Vec<_>>();
    let values = statement
        .query_map([], |row| {
            let mut result = serde_json::Map::new();
            for (index, name) in names.iter().enumerate() {
                let value = match row.get_ref(index).unwrap() {
                    rusqlite::types::ValueRef::Null => Json::Null,
                    rusqlite::types::ValueRef::Integer(value) => json!(value),
                    rusqlite::types::ValueRef::Real(value) => json!(value),
                    rusqlite::types::ValueRef::Text(value) => {
                        json!(std::str::from_utf8(value).unwrap())
                    }
                    _ => panic!("Unexpected scalar sample storage"),
                };
                result.insert(name.clone(), value);
            }
            Ok(Json::Object(result))
        })
        .unwrap();
    json!(values.map(Result::unwrap).collect::<Vec<_>>())
}

fn sample(
    user_context: &UserContext,
    input: &Json,
    pool: &Arc<Mutex<rusqlite::Connection>>,
) -> Json {
    wait(user_context.ensure_schema()).unwrap();
    if input["root"] == false {
        pool.lock()
            .unwrap()
            .execute_batch("DELETE FROM gateway_data")
            .unwrap();
    }
    if input["fault"] == "trigger" {
        pool.lock().unwrap().execute_batch("CREATE TRIGGER sample_failure BEFORE INSERT ON reading_data BEGIN SELECT RAISE(ABORT,'fixture sample insert failed'); END").unwrap();
    }
    let mut rounds = Vec::new();
    for _ in 0..input["rounds"].as_u64().unwrap() {
        let scale = match input["scale"].as_str().unwrap() {
            "Tiny" => domain::SampleDataScale::Tiny,
            "Small" => domain::SampleDataScale::Small,
            _ => domain::SampleDataScale::Medium,
        };
        let result = wait(domain::generate_sample_data(
            user_context,
            domain::SampleDataPlan {
                scale,
                seed: input["seed"].as_u64().unwrap(),
            },
        ));
        let report = match result {
            Ok(report) => {
                json!({"error":null,"generated":report.generated,"skipped":report.skipped.iter().map(|value|json!({"entity":value.entity,"reason":value.reason})).collect::<Vec<_>>()})
            }
            Err(error) => json!({"error":error.to_string()}),
        };
        rounds.push(json!({"report":report,"rows":readings(&pool.lock().unwrap())}));
    }
    json!(rounds)
}

fn observe(input: &Json) -> Json {
    let pool = Arc::new(Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));
    let mut user_context = wait(domain::service_runtime_from_pool(pool.clone())).unwrap();
    let kind = input["kind"].as_str().unwrap();
    if kind == "sample" {
        user_context = user_context.with_active_root("Gateway", 1);
    } else if let Some(root) = input["root"].as_u64() {
        user_context = user_context.with_active_root("Gateway", root);
    }
    let wall = teaql_core::time::Timestamp::now().0;
    json!({"wall_millis":wall,"result":if kind=="sample" {sample(&user_context,input,&pool)} else {check(&user_context,input)}})
}

fn main() {
    for line in io::stdin().lock().lines() {
        let input: Json = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", observe(&input));
    }
}
