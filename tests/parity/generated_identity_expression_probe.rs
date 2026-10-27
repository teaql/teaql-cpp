use serde_json::{Value as Json, json};
use std::fmt::Debug;
use std::io::{self, BufRead};
use std::panic::{AssertUnwindSafe, catch_unwind};
use teaql_core::{Entity, TeaqlEntity, Value};
use teaql_runtime::{InternalIdGenerator, RuntimeError};

struct Ids;
impl InternalIdGenerator for Ids {
    fn generate_id(&self, _: &str) -> Result<u64, RuntimeError> {
        Ok(1001)
    }
}

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
        let mut context = domain::module().into_context();
        context.set_internal_id_generator(Ids);
        let state = input["state"].as_str().unwrap();
        let path = input["path"].as_str().unwrap();
        let action = input["action"].as_str().unwrap();
        let data = match state {
            "json_empty" => json!({}),
            "json_null" => json!({"id":101,"version":1,"note":null,"platform":null}),
            "json_full" => {
                json!({"id":101,"version":1,"name":"VISIBLE-NAME","note":"VISIBLE-NOTE","platform_id":100})
            }
            "json_relation_missing_name" => {
                json!({"id":101,"version":1,"platform":{"id":100,"version":1}})
            }
            "json_relation_named" => {
                json!({"id":101,"version":1,"platform":{"id":100,"version":1,"name":"PLATFORM-NAME"}})
            }
            _ => Json::Null,
        };
        macro_rules! inspect {
            ($model:ty, $query:expr, $entry:path) => {{
                let prepared = if state.starts_with("json_") {
                    context.decode_json_entity::<$model>(&data)
                } else {
                    let mut entity = $query.comment("prepare expression identity")
                        .purpose("qualify explicit expression reads").new_entity(&context);
                    if state == "new_name" { entity.update_name("VISIBLE-NAME"); }
                    if state == "new_note_null" { entity.update_note(Value::Null); }
                    Ok(entity)
                };
                match prepared {
                    Ok(entity) => {
                        let expression = $entry(&entity);
                        let result = match path {
                            "id" => read(expression.get_id(), action),
                            "name" => read(expression.get_name(), action),
                            "note" => read(expression.get_note(), action),
                            "version" => read(expression.get_version(), action),
                            "platform_id" => read(expression.get_platform_id(), action),
                            "platform_name" => read(expression.get_platform().get_name(), action),
                            _ => panic!("Unknown fixture path"),
                        };
                        json!({"prepared":true,"identity":<$model as TeaqlEntity>::ENTITY_NAME,
                            "id":entity.id(),"read":result})
                    }
                    Err(error) => json!({"prepared":false,"error":error.to_string(),"debug":format!("{error:?}")}),
                }
            }};
        }
        let mut output = if input["entity"] == "item" {
            inspect!(domain::Item, domain::Q::items(), domain::E::item)
        } else {
            inspect!(domain::Child, domain::Q::children(), domain::E::child)
        };
        output["sql_count"] = json!(context.sql_logs().len());
        println!("{output}");
    }
}
