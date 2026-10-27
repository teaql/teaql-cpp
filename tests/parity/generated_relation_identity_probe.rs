use serde_json::{Value as Json, json};
use std::fmt::Debug;
use std::io::{self, BufRead};
use std::panic::{AssertUnwindSafe, catch_unwind};
use teaql_core::{
    CompactRow, Entity, TeaqlEntity, Value,
    eval::{EvalResult, LoadState},
};
use teaql_runtime::{EntityGraphBuilder, EntityRuntimeState};
fn diagnostic(message: &str) -> Json {
    let mut result = serde_json::Map::new();
    for line in message.lines() {
        if let Some((key, value)) = line.split_once(": ") {
            if [
                "original_expr_with_broken_point",
                "root",
                "access_path",
                "break_point",
                "missing_preload",
                "suggested_fix",
                "nested_fix",
                "severity",
                "human_message",
            ]
            .contains(&key)
            {
                result.insert(
                    key.into(),
                    if key == "root" {
                        json!(value)
                    } else if key == "human_message" {
                        // The source renderer surrounds this field in quotes,
                        // but leaves its embedded selection quotes unescaped.
                        json!(value.strip_prefix('"').unwrap().strip_suffix('"').unwrap())
                    } else {
                        serde_json::from_str(value)
                            .expect("Original diagnostic field must remain valid JSON")
                    },
                );
            }
        }
    }
    assert_eq!(result.len(), 9);
    Json::Object(result)
}

fn read<T: Clone + Debug>(expression: domain::ValueExpression<'_, T>, action: &str) -> Json {
    match catch_unwind(AssertUnwindSafe(|| {
        if action == "eval" {
            format!("{:?}", expression.eval())
        } else {
            format!("{:?}", expression.unwrap())
        }
    })) {
        Ok(value) => json!({"kind":"value","value":value}),
        Err(payload) => {
            let message = payload
                .downcast_ref::<String>()
                .cloned()
                .or_else(|| payload.downcast_ref::<&str>().map(|s| s.to_string()))
                .unwrap();
            if message.contains("[Coding Logic Bug]") {
                json!({"kind":"not_loaded","diagnostic":diagnostic(&message)})
            } else {
                assert_eq!(message, "Value was legitimately null in database!");
                json!({"kind":"null","message":message})
            }
        }
    }
}

fn edge(root: &EntityRuntimeState, owner: &str) -> Json {
    match root.resolve_relation_option::<domain::Platform>(owner, 101, "platform") {
        None => json!({"state":"absent"}),
        Some(None) => json!({"state":"empty"}),
        Some(Some(value)) => json!({"state":"present","id":value.id()}),
    }
}

fn platform(context: &teaql_runtime::UserContext, id: u64, name: &str) -> domain::Platform {
    context
        .decode_json_entity(&json!({"id":id,"version":1,"name":name}))
        .unwrap()
}

fn compact_row(embedded: bool, foreign: u64) -> CompactRow {
    let mut values = std::collections::BTreeMap::from([
        ("id".to_owned(), Value::U64(101)),
        ("version".to_owned(), Value::I64(1)),
        ("name".to_owned(), Value::Text("OWNER".into())),
    ]);
    if embedded {
        values.insert(
            "platform".into(),
            Value::Object(std::collections::BTreeMap::from([
                ("id".into(), Value::U64(77)),
                ("version".into(), Value::I64(1)),
                ("name".into(), Value::Text("EMBEDDED".into())),
            ])),
        );
    } else {
        values.insert("platform_id".into(), Value::U64(foreign));
    }
    CompactRow::from(values)
}

fn main() {
    let default_hook = std::panic::take_hook();
    std::panic::set_hook(Box::new(move |info| {
        let message = info
            .payload()
            .downcast_ref::<String>()
            .map(String::as_str)
            .or_else(|| info.payload().downcast_ref::<&str>().copied());
        if !message.is_some_and(|message| {
            message.contains("[Coding Logic Bug]")
                || message == "Value was legitimately null in database!"
        }) {
            default_hook(info);
        }
    }));
    for line in io::stdin().lock().lines() {
        let input: Json = serde_json::from_str(&line.unwrap()).unwrap();
        let context = domain::module().into_context();
        let selected = input["selected"].as_bool().unwrap();
        let cached = input["cached"].as_bool().unwrap();
        let embedded = input["embedded"].as_bool().unwrap();
        let foreign: u64 = input["foreign_id"].as_str().unwrap().parse().unwrap();
        let mode = input["owner"].as_str().unwrap();
        let view = input["view"].as_str().unwrap();
        let mut payload = json!({"id":101,"version":1,"name":"OWNER"});
        if embedded {
            payload["platform"] = json!({"id":77,"version":1,"name":"EMBEDDED"});
        } else {
            payload["platform_id"] = json!(foreign);
        }
        macro_rules! inspect {
            ($model:ty, $canonical:expr, $entry:path) => {{
                let mut entity = if input["construction"] == "row" {
                    <$model>::from_compact_row(compact_row(embedded, foreign)).unwrap()
                } else {
                    context.decode_json_entity::<$model>(&payload).unwrap()
                };
                let root = EntityRuntimeState::default();
                entity.attach_runtime_state_recursive(root.clone());
                let mut fields = vec!["id".to_owned(), "version".to_owned(), "name".to_owned(), "platform_id".to_owned()];
                if selected { fields.push("platform".to_owned()); }
                entity.set_load_state(LoadState::Partial(fields.into_iter().collect()));
                let mut graph = EntityGraphBuilder::default();
                if cached { graph.install(100, platform(&context, 100, "CACHE")); }
                let mut install = |owner: &str, id: u64, name: &str| {
                    match view {
                        "present" => graph.install_relation_option(owner, 101, "platform", Some(platform(&context,id,name))),
                        "empty" => graph.install_relation_option::<domain::Platform>(owner,101,"platform",None),
                        "wrong_type" => graph.install_relation_option(owner,101,"platform",Some(900_u64)),
                        _ => panic!("Unknown view"),
                    }
                };
                let raw = <$model>::ENTITY_NAME;
                let runtime_name = <$model as TeaqlEntity>::ENTITY_NAME;
                match mode {
                    "raw" => install(raw, 200, "MODEL"),
                    "canonical" => install($canonical, 200, "MODEL"),
                    "trait" => install(runtime_name, 300, "TRAIT"),
                    "both" => { install($canonical,200,"MODEL"); install(runtime_name,300,"TRAIT"); },
                    "none" => {},
                    _ => panic!("Unknown owner"),
                }
                drop(install);
                assert!(root.freeze_graph(graph).is_ok());
                let eval = match entity.eval_platform() {
                    EvalResult::Value(v) => json!({"state":"Value","id":v.id()}),
                    EvalResult::Null => json!({"state":"Null"}),
                    EvalResult::NotLoaded {failed_node,attempted_path} => json!({"state":"NotLoaded","node":failed_node,"path":attempted_path}),
                };
                let getter = entity.platform().map(|value| value.id());
                let expression = read($entry(&entity).get_platform().get_name(), "eval");
                let kernel = json!({"raw":edge(&root,raw),"canonical":edge(&root,$canonical),"trait":edge(&root,runtime_name)});
                json!({"raw":raw,"identity":runtime_name,"getter":getter,"eval":eval,"expression":expression,
                    "json":entity.clone().into_json(),"kernel":kernel,"sql_count":context.sql_logs().len()})
            }};
        }
        let output = if input["entity"] == "item" {
            inspect!(domain::Item, "Item", domain::E::item)
        } else {
            inspect!(domain::Child, "Child", domain::E::child)
        };
        println!("{output}");
    }
}
