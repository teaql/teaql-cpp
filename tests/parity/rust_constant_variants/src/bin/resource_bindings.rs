use cpp_constants_numeric_core as domain;
use serde_json::{Value as Json, json};
use std::{
    collections::BTreeMap,
    io::{self, BufRead},
    sync::{
        Arc, Mutex,
        atomic::{AtomicU64, Ordering},
    },
};

struct BindingSettings {
    tag: String,
    payload: Vec<String>,
    delta: i64,
    destroyed: Arc<AtomicU64>,
}
impl Drop for BindingSettings {
    fn drop(&mut self) {
        self.destroyed.fetch_add(1, Ordering::SeqCst);
    }
}
struct OtherBindingSettings {
    value: i64,
    destroyed: Arc<AtomicU64>,
}
impl Drop for OtherBindingSettings {
    fn drop(&mut self) {
        self.destroyed.fetch_add(1, Ordering::SeqCst);
    }
}
fn settings_view(settings: Option<&BindingSettings>) -> Json {
    settings
        .map(|s| json!({"tag":s.tag,"delta":s.delta,"payload":s.payload}))
        .unwrap_or(Json::Null)
}
struct BindingLogic {
    named: bool,
    key: String,
    hooks: Arc<Mutex<Vec<Json>>>,
}
impl domain::PriorityCheckerLogic for BindingLogic {
    fn check_and_fix_priority(
        &self,
        user_context: &teaql_runtime::UserContext,
        entity: &mut domain::Priority,
        status: teaql_runtime::CheckObjectStatus,
        location: &teaql_runtime::ObjectLocation,
        results: &mut teaql_runtime::CheckResults,
    ) {
        let required = if self.named {
            user_context.require_named_resource::<BindingSettings>(&self.key)
        } else {
            user_context.require_resource::<BindingSettings>()
        };
        let before = entity.rank();
        match required {
            Ok(settings) => {
                entity.update_rank(before + settings.delta);
            }
            Err(_) => results.push(teaql_runtime::CheckResult::required(
                location.clone().member("rank"),
            )),
        }
        self.hooks.lock().unwrap().push(json!({"status":format!("{status:?}"),"before":before,"after":entity.rank(),"resource":settings_view(required.ok())}));
    }
}

fn observe(input: &Json) -> Json {
    let destroyed = Arc::new(AtomicU64::new(0));
    let hooks = Arc::new(Mutex::new(Vec::<Json>::new()));
    let key = input["key"].as_str().unwrap();
    let named = input["lookup"] == "named";
    let mut created_total = 0_u64;
    let mut semantic = Vec::new();
    let mut ownership = Vec::new();
    for _ in 0..input["rounds"].as_u64().unwrap() {
        let module = match input["module"].as_str().unwrap() {
            "plain" => domain::module(),
            "checker" => domain::module_with_checkers(),
            "behavior" => domain::module_with_behaviors(),
            _ => domain::module_with_behaviors_and_checkers(),
        }
        .checker(
            teaql_runtime::TypedEntityChecker::<domain::Priority, _>::new(
                domain::PriorityChecker::new(BindingLogic {
                    named,
                    key: key.into(),
                    hooks: hooks.clone(),
                }),
            ),
        );
        let mut user_context = module.into_context().with_active_root("Platform", 1);
        let mut steps = Vec::new();
        let mut drops = Vec::new();
        for (sequence, action) in input["actions"].as_array().unwrap().iter().enumerate() {
            let name = action.as_str().unwrap();
            if name == "typed" || name == "named" {
                let tag = format!("{}-{sequence}", input["tag"].as_str().unwrap());
                let settings = BindingSettings {
                    payload: vec![format!("{tag}-first"), format!("{tag}-second")],
                    tag,
                    delta: input["delta"].as_i64().unwrap() * (sequence as i64 + 1),
                    destroyed: destroyed.clone(),
                };
                created_total += 1;
                if name == "typed" {
                    user_context.insert_resource(settings);
                } else {
                    user_context.insert_named_resource(key, settings);
                }
            } else if name == "other" {
                created_total += 1;
                user_context.insert_named_resource(
                    key,
                    OtherBindingSettings {
                        value: sequence as i64,
                        destroyed: destroyed.clone(),
                    },
                );
            } else if name != "check" {
                panic!("Unknown resource binding action");
            }
            let typed = settings_view(user_context.get_resource::<BindingSettings>());
            let named = settings_view(user_context.get_named_resource::<BindingSettings>(key));
            let other = user_context
                .get_named_resource::<OtherBindingSettings>(key)
                .map(|s| s.value);
            let named_error = user_context
                .require_named_resource::<BindingSettings>(key)
                .err()
                .map(|e| e.to_string());
            let required = user_context.require_resource::<BindingSettings>();
            let typed_missing = required.is_err();
            if let Err(error) = required {
                eprintln!("{error}");
            }
            let mut values: teaql_runtime::EntityValues = BTreeMap::from([
                ("id".into(), teaql_core::Value::U64(7)),
                ("version".into(), teaql_core::Value::I64(0)),
                ("platform_id".into(), teaql_core::Value::U64(1)),
                ("name".into(), teaql_core::Value::Text("Fixture".into())),
                ("code".into(), teaql_core::Value::Text("FIXTURE".into())),
                ("rank".into(), teaql_core::Value::I64(10)),
                ("weight".into(), teaql_core::Value::F64(1.0)),
                ("ratio".into(), teaql_core::Value::F64(1.0)),
                (
                    "cost".into(),
                    teaql_core::Value::Decimal("0".parse().unwrap()),
                ),
                (
                    "day".into(),
                    teaql_core::Value::Date("1970-01-01".parse().unwrap()),
                ),
                ("note".into(), teaql_core::Value::Null),
                ("optional_rank".into(), teaql_core::Value::Null),
            ])
            .into();
            teaql_runtime::mark_entity_status(
                &mut values,
                teaql_runtime::CheckObjectStatus::Create,
            );
            let checked = user_context.check_and_fix_values("Priority", &mut values);
            let rank = match values.get("rank").unwrap() {
                teaql_core::Value::I64(v) => *v,
                _ => panic!("unexpected rank type"),
            };
            steps.push(json!({"action":name,"typed":typed,"named":named,"other":other,"named_error":named_error,
                "typed_missing":typed_missing,"checker_error":checked.err().map(|e|e.to_string()),"rank":rank}));
            drops.push(destroyed.load(Ordering::SeqCst));
        }
        drop(user_context);
        semantic.push(json!({"steps":steps,"created":created_total,"destroyed_after_close":destroyed.load(Ordering::SeqCst)}));
        ownership.push(json!({"during":drops}));
    }
    json!({"semantic":{"rounds":semantic,"hooks":hooks.lock().unwrap().clone()},"ownership":ownership})
}
fn main() {
    for line in io::stdin().lock().lines() {
        let input: Json = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", observe(&input));
    }
}
