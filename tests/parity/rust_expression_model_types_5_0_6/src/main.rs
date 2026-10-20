use teaql_expression_model_types_506::{E, OptionalE, Person, Reading, Device, ValueExpression};
use serde_json::{Value as Json, json};
use std::{collections::BTreeMap, io::{self, BufRead}, panic::{catch_unwind, AssertUnwindSafe}};
use teaql_core::{CompactRow, Entity, Value};
#[path = "../../rust_values/src/fixture_value.rs"]
mod fixture_value;

fn capture(function: impl FnOnce() -> Json) -> Json {
    match catch_unwind(AssertUnwindSafe(function)) {
        Ok(value) => json!({"ok": value}),
        Err(error) => {
            let text = error.downcast_ref::<String>().cloned()
                .or_else(|| error.downcast_ref::<&str>().map(|s|s.to_string())).unwrap();
            json!({"panic":text})
        }
    }
}
fn summary<T: Clone + Default>(expression: ValueExpression<'_, T>, fallback: T,
                              encode: impl Fn(T) -> Json) -> Json {
    let mut callbacks = 0;
    let evaluation = capture(|| match expression.clone().eval() {
        Some(v) => json!({"present":true,"value":encode(v)}),
        None => json!({"present":false,"value":Json::Null}),
    });
    let unwrapped = capture(||encode(expression.clone().unwrap()));
    let eager = capture(||encode(expression.clone().or_if_null(fallback.clone())));
    let lazy = capture(||encode(expression.clone().or_else_if_null(|| {
        callbacks += 1;
        fallback.clone()
    })));
    let default = capture(||encode(expression.or_default_if_null()));
    json!({"eval":evaluation,"unwrap":unwrapped,"eager":eager,"lazy":lazy,"default":default,"callbacks":callbacks})
}
fn row(input: &Json, fallback: bool) -> CompactRow {
    let name = input["field"].as_str().unwrap();
    let mut fields = BTreeMap::from([("id".into(),Value::U64(7)),("version".into(),Value::I64(1))]);
    fields.remove(name);
    if fallback || input["loaded"] == true {
        fields.insert(name.into(),fixture_value::value(&input[if fallback {"fallback"} else {"value"}]));
    }
    CompactRow::from_map(fields)
}
fn observe(input: &Json) -> Json {
    let field = input["field"].as_str().unwrap();
    if input["model"] == "Person" {
        let entity = match Person::from_compact_row(row(input,false)) {
            Ok(entity) => entity,
            Err(error) => return json!({"decode_error":error.to_string()}),
        };
        let fallback = Person::from_compact_row(row(input,true)).unwrap();
        let expression = E::person(&entity);
        return match field {
            "id" => summary(expression.get_id(),fallback.id(),|v|Value::from(v).to_json_value()),
            "email" => summary(expression.get_email(),fallback.email(),|v|Value::from(v).to_json_value()),
            "active" => summary(expression.get_active(),fallback.active(),|v|Value::from(v).to_json_value()),
            "nickname" => summary(expression.get_nickname(),fallback.nickname(),|v|v.map(|v|Value::from(v).to_json_value()).unwrap_or(Json::Null)),
            "balance" => summary(expression.get_balance(),fallback.balance(),|v|Value::from(v).to_json_value()),
            "height" => summary(expression.get_height(),fallback.height(),|v|Value::from(v).to_json_value()),
            "ratio" => summary(expression.get_ratio(),fallback.ratio(),|v|Value::from(v).to_json_value()),
            "age" => summary(expression.get_age(),fallback.age(),|v|Value::from(v).to_json_value()),
            "birthday" => summary(expression.get_birthday(),fallback.birthday(),|v|Value::from(v).to_json_value()),
            "updated_at" => summary(expression.get_updated_at(),fallback.updated_at(),|v|Value::from(v).to_json_value()),
            "payload" => summary(expression.get_payload(),fallback.payload(),|v|v.map(|v|Value::from(v).to_json_value()).unwrap_or(Json::Null)),
            "version" => summary(expression.get_version(),fallback.version(),|v|Value::from(v).to_json_value()),
            _ => panic!("Unknown expression field"),
        };
    }
    if input["model"] == "Reading" {
        let entity = match Reading::from_compact_row(row(input,false)) {
            Ok(entity) => entity,
            Err(error) => return json!({"decode_error":error.to_string()}),
        };
        let fallback = Reading::from_compact_row(row(input,true)).unwrap();
        let expression = OptionalE::reading(&entity);
        return match field {
            "id" => summary(expression.get_id(),fallback.id(),|v|Value::from(v).to_json_value()),
            "text" => summary(expression.get_text(),fallback.text(),|v|v.map(|v|Value::from(v).to_json_value()).unwrap_or(Json::Null)),
            "active" => summary(expression.get_active(),fallback.active(),|v|v.map(|v|Value::from(v).to_json_value()).unwrap_or(Json::Null)),
            "balance" => summary(expression.get_balance(),fallback.balance(),|v|v.map(|v|Value::from(v).to_json_value()).unwrap_or(Json::Null)),
            "height" => summary(expression.get_height(),fallback.height(),|v|v.map(|v|Value::from(v).to_json_value()).unwrap_or(Json::Null)),
            "ratio" => summary(expression.get_ratio(),fallback.ratio(),|v|v.map(|v|Value::from(v).to_json_value()).unwrap_or(Json::Null)),
            "count" => summary(expression.get_count(),fallback.count(),|v|v.map(|v|Value::from(v).to_json_value()).unwrap_or(Json::Null)),
            "day" => summary(expression.get_day(),fallback.day(),|v|v.map(|v|Value::from(v).to_json_value()).unwrap_or(Json::Null)),
            "recorded_at" => summary(expression.get_recorded_at(),fallback.recorded_at(),|v|v.map(|v|Value::from(v).to_json_value()).unwrap_or(Json::Null)),
            "payload" => summary(expression.get_payload(),fallback.payload(),|v|v.map(|v|Value::from(v).to_json_value()).unwrap_or(Json::Null)),
            "version" => summary(expression.get_version(),fallback.version(),|v|Value::from(v).to_json_value()),
            _ => panic!("Unknown expression field"),
        };
    }
    if input["model"] == "Device" {
        let entity = match Device::from_compact_row(row(input,false)) {
            Ok(entity) => entity,
            Err(error) => return json!({"decode_error":error.to_string()}),
        };
        let fallback = Device::from_compact_row(row(input,true)).unwrap();
        let expression = E::device(&entity);
        return match field {
            "owner_id" => summary(expression.get_owner_id(),fallback.owner_id(),|v|Value::from(v).to_json_value()),
            _ => panic!("Unknown expression field"),
        };
    }
    panic!("Unknown expression model");
}
fn main() {
    std::panic::set_hook(Box::new(|_|{}));
    for line in io::stdin().lock().lines() {
        let input: Json = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}",observe(&input));
    }
}
