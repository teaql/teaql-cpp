use cpp_types_core::{Person, Reading, ReadingChecker, ReadingCheckerLogic};
use cpp_relations_core::{Device, Platform, Profile};
use serde_json::{json, Value as Json};
use std::{collections::BTreeMap, io::{self, BufRead}};
use teaql_core::{CompactRow, Entity, VersionedEntity, Value, eval::{EvalResult, LoadState}};
use teaql_runtime::{EntityGraphBuilder, EntityRuntimeState, LedgerEntity};
#[path = "../../../rust_values/src/fixture_value.rs"]
mod fixture_value;

fn changed(value: Option<Value>) -> Json {
    json!({"present":value.is_some(), "value":value.map(|v|v.to_json_value())})
}
fn state<T>(result: EvalResult<T>, value: impl FnOnce(T) -> Json) -> Json {
    match result {
        EvalResult::Value(v) => json!({"state":"Value", "value":value(v)}),
        EvalResult::Null => json!({"state":"Null"}),
        EvalResult::NotLoaded { failed_node, attempted_path } =>
            json!({"state":"NotLoaded", "node":failed_node, "path":attempted_path}),
    }
}
fn view(entity: &Person) -> Json {
    json!({
        "email":entity.email(), "nickname":entity.nickname(),
        "active":entity.active(), "age":entity.age(),
        "eval_email":state(entity.eval_email(), |v|json!(v)),
        "eval_nickname":state(entity.eval_nickname(), |v|json!(v)),
        "eval_active":state(entity.eval_active(), |v|json!(v)),
        "eval_age":state(entity.eval_age(), |v|json!(v)),
        "changed_email":changed(entity.changed_email()),
        "changed_nickname":changed(entity.changed_nickname()),
        "changed_active":changed(entity.changed_active()),
        "changed_age":changed(entity.changed_age()),
    })
}
fn scalar(input: &Json) -> Json {
    let mut fields = BTreeMap::from([("id".into(), Value::U64(7)), ("version".into(), Value::I64(3))]);
    if input["loaded"] == true {
        fields.insert("email".into(), Value::Text("initial".into()));
        fields.insert("nickname".into(), if input["initial_null"] == true {Value::Null} else {Value::Text("initial".into())});
        fields.insert("active".into(), Value::Bool(false));
        fields.insert("age".into(), Value::I64(0));
    }
    let mut entity = Person::from_compact_row(CompactRow::from_map(fields)).unwrap();
    let root = EntityRuntimeState::default();
    entity.attach_runtime_state_recursive(root.clone());
    let copy = entity.clone();
    let before = view(&entity);
    let text = input["text"].as_str().unwrap();
    if input["op"] == "update" || input["op"] == "update_self" {
        entity.update_email(text);
        entity.update_nickname(if input["null"] == true {Value::Null} else {Value::Text(text.into())});
        entity.update_active(input["active"].as_bool().unwrap());
        entity.update_age(input["age"].as_i64().unwrap());
        if input["op"] == "update_self" {
            entity.update_nickname(entity.nickname().map(Value::Text).unwrap_or(Value::Null));
            entity.update_email(entity.email());
        }
    } else if input["op"] == "overlay" {
        root.set(entity.entity_key(), "email", Value::Text(text.into()));
        root.set(entity.entity_key(), "nickname", if input["null"] == true {Value::Null} else {Value::Text(text.into())});
        root.set(entity.entity_key(), "active", Value::Bool(input["active"].as_bool().unwrap()));
        root.set(entity.entity_key(), "age", Value::I64(input["age"].as_i64().unwrap()));
    }
    json!({"before":before, "after":view(&entity), "copy":view(&copy)})
}
fn relations(input: &Json) -> Json {
    let root = EntityRuntimeState::default();
    let mut device = Device::from_compact_row(CompactRow::from_map(BTreeMap::new())).unwrap();
    device.attach_runtime_state_recursive(root.clone());
    device.update_id(7_u64);
    if input["fk_loaded"] == true {
        device.update_platform_id(input["foreign_id"].as_u64().map(Value::U64).unwrap_or(Value::Null));
    }
    let mut fields = vec!["id".into()];
    if input["fk_loaded"] == true { fields.push("platform_id".into()); }
    if input["selected"] == true { fields.push("platform".into()); }
    device.set_load_state(LoadState::Partial(fields.into_iter().collect()));
    let mut owner = Platform::from_compact_row(CompactRow::from_map(BTreeMap::new())).unwrap();
    owner.attach_runtime_state_recursive(root.clone());
    owner.update_id(9_u64);
    let mut target = Platform::from_compact_row(CompactRow::from_map(BTreeMap::new())).unwrap();
    target.update_id(9_u64);
    let mut profile = Profile::from_compact_row(CompactRow::from_map(BTreeMap::new())).unwrap();
    profile.update_id(11_u64);
    let mut graph = EntityGraphBuilder::default();
    if input["cached"] == true { graph.install(9, target.clone()); }
    match input["forward"].as_str().unwrap() {
        "present" => graph.install_relation_option("Device", 7, "platform", Some(target)),
        "empty" => graph.install_relation_option::<Platform>("Device", 7, "platform", None),
        "wrong_type" => graph.install_relation_option("Device", 7, "platform", Some(99_u64)),
        _ => {},
    }
    match input["reverse"].as_str().unwrap() {
        "present" => graph.install_relation_option("Platform", 9, "profile", Some(profile)),
        "empty" => graph.install_relation_option::<Profile>("Platform", 9, "profile", None),
        "wrong_type" => graph.install_relation_option("Platform", 9, "profile", Some(99_u64)),
        _ => {},
    }
    root.freeze_graph(graph).unwrap();
    let handle = owner.profile();
    json!({"forward":state(device.eval_platform(), |v|json!(v.id())),
           "foreign":state(device.eval_platform_id(), |v|json!(v)),
           "reverse":state(owner.eval_profile(), |v|json!(v.id())),
           "handle":format!("{:?}",handle.state()), "empty":handle.is_empty(), "loaded":handle.is_loaded()})
}
fn field_view(entity: &Person, root: &EntityRuntimeState, name: &str) -> Json {
    macro_rules! inspect {
        ($getter:ident, $eval:ident, $changed:ident) => {{
            (Value::from(entity.$getter()).to_json_value(),
             state(entity.$eval(), |v|Value::from(v).to_json_value()),
             changed(entity.$changed()))
        }};
    }
    let (getter, evaluation, mutation) = match name {
        "balance" => inspect!(balance, eval_balance, changed_balance),
        "height" => inspect!(height, eval_height, changed_height),
        "ratio" => inspect!(ratio, eval_ratio, changed_ratio),
        "birthday" => inspect!(birthday, eval_birthday, changed_birthday),
        "updated_at" => inspect!(updated_at, eval_updated_at, changed_updated_at),
        "id" => inspect!(id, eval_id, changed_id),
        "version" => inspect!(version, eval_version, changed_version),
        "payload" => (json!(entity.payload()), state(entity.eval_payload(), |v|json!(v)), changed(entity.changed_payload())),
        _ => unreachable!(),
    };
    let original = root.original_snapshot().and_then(|row|row.get(name).cloned());
    json!({"getter":getter, "eval":evaluation, "changed":mutation,
           "original":changed(original), "metadata_version":VersionedEntity::version(entity)})
}
fn field_probe(input: &Json) -> Json {
    let name = input["field"].as_str().unwrap();
    let mut row = BTreeMap::from([("id".into(), Value::U64(7)), ("version".into(), Value::I64(3))]);
    row.remove(name);
    if input["loaded"] == true { row.insert(name.into(), fixture_value::value(&input["initial"])); }
    let mut entity = Person::from_compact_row(CompactRow::from_map(row)).unwrap();
    let root = entity.entity_runtime_state().unwrap();
    let copy = entity.clone();
    let before = field_view(&entity, &root, name);
    let value = fixture_value::value(&input["value"]);
    match input["op"].as_str().unwrap() {
        "update" => match name {
            "balance" => {entity.update_balance(value.try_decimal().unwrap());},
            "height" => {entity.update_height(value.try_f64().unwrap());},
            "ratio" => {entity.update_ratio(value.try_f64().unwrap() as f32);},
            "birthday" => {entity.update_birthday(value.try_date().unwrap());},
            "updated_at" => {entity.update_updated_at(value.try_timestamp().unwrap());},
            "id" => {entity.update_id(value.try_u64().unwrap());},
            "version" => {entity.update_version(value.try_i64().unwrap());},
            "payload" => {entity.update_payload(value);},
            _ => unreachable!(),
        },
        "overlay" => {root.set(entity.entity_key(), name, value);},
        _ => {},
    }
    json!({"before":before, "after":field_view(&entity, &root, name), "copy":field_view(&copy, &root, name)})
}
fn optional_view(entity: &Reading, name: &str) -> Json {
    macro_rules! inspect {
        ($getter:ident, $eval:ident, $changed:ident) => {{
            (entity.$getter().map(|v|Value::from(v).to_json_value()),
             state(entity.$eval(), |v|json!(v.map(|v|Value::from(v).to_json_value()))),
             changed(entity.$changed()))
        }};
    }
    let (getter, evaluation, mutation) = match name {
        "text" => inspect!(text, eval_text, changed_text),
        "active" => inspect!(active, eval_active, changed_active),
        "balance" => inspect!(balance, eval_balance, changed_balance),
        "height" => inspect!(height, eval_height, changed_height),
        "ratio" => inspect!(ratio, eval_ratio, changed_ratio),
        "count" => inspect!(count, eval_count, changed_count),
        "day" => inspect!(day, eval_day, changed_day),
        "recorded_at" => inspect!(recorded_at, eval_recorded_at, changed_recorded_at),
        "payload" => (entity.payload(), state(entity.eval_payload(), |v|json!(v)), changed(entity.changed_payload())),
        _ => unreachable!(),
    };
    let original = entity.entity_runtime_state().unwrap().original_snapshot().and_then(|row|row.get(name).cloned());
    json!({"getter":getter, "eval":evaluation, "changed":mutation, "original":changed(original)})
}
struct OptionalMarker { allow: bool, fix_text: bool }
struct OptionalLogic(std::sync::Arc<std::sync::Mutex<(usize,usize)>>);
impl ReadingCheckerLogic for OptionalLogic {
    fn check_and_fix_reading(&self, user_context: &teaql_runtime::UserContext, entity: &mut Reading,
                            _status: teaql_runtime::CheckObjectStatus, location: &teaql_runtime::ObjectLocation,
                            results: &mut teaql_runtime::CheckResults) {
        *self.0.lock().unwrap() = (1, results.len());
        let marker = user_context.require_resource::<OptionalMarker>().unwrap();
        self.required(marker.allow, "custom_field", location, results);
        if marker.fix_text { entity.update_text("custom fixed"); }
    }
}
fn optional_probe(input: &Json) -> Json {
    use teaql_runtime::{CheckObjectStatus, TypedChecker, ObjectLocation, JsonFieldNamingProfile};
    let name = input["field"].as_str().unwrap();
    let mut row = BTreeMap::from([("id".into(), Value::U64(7)), ("version".into(), Value::I64(3))]);
    if input["loaded"] == true { row.insert(name.into(), fixture_value::value(&input["initial"])); }
    let mut entity = Reading::from_compact_row(CompactRow::from_map(row)).unwrap();
    if input["full"] == true { entity.set_load_state(LoadState::FullyLoaded); }
    let root = entity.entity_runtime_state().unwrap();
    let copy = entity.clone();
    let before = optional_view(&entity, name);
    let value = fixture_value::value(&input["value"]);
    match input["op"].as_str().unwrap() {
        "update" => match name {
            "recorded_at" => {entity.update_recorded_at(if matches!(value,Value::Null) {None} else {value.try_timestamp()});},
            "ratio" => {entity.update_ratio(if matches!(value,Value::Null) {Value::Null} else {Value::from(value.try_f64().unwrap() as f32)});},
            "text" => {entity.update_text(value);},
            "active" => {entity.update_active(value);},
            "balance" => {entity.update_balance(value);},
            "height" => {entity.update_height(value);},
            "count" => {entity.update_count(value);},
            "day" => {entity.update_day(value);},
            "payload" => {entity.update_payload(value);},
            _ => unreachable!(),
        },
        "overlay" => {root.set(entity.entity_key(), name, value);},
        _ => {},
    }
    let mut user_context = teaql_runtime::UserContext::new();
    user_context.insert_resource(OptionalMarker {allow:input["allow"]==true,fix_text:input["fix"]==true});
    let calls = std::sync::Arc::new(std::sync::Mutex::new((0,0)));
    let checker = ReadingChecker::new(OptionalLogic(calls.clone()));
    let mut results = Vec::new();
    let status = match input["status"].as_str().unwrap() {
        "update" => CheckObjectStatus::Update, "create" => CheckObjectStatus::Create, _ => CheckObjectStatus::Unknown
    };
    let location = ObjectLocation::array_root(2).member("reading_info");
    checker.check_and_fix_typed(&user_context, &mut entity, status, &location, &mut results);
    let diagnostics: Vec<_> = [JsonFieldNamingProfile::CamelCase,JsonFieldNamingProfile::SnakeCase,JsonFieldNamingProfile::PascalCase]
        .into_iter().map(|profile|results.iter().map(|v|v.to_wire(profile)).collect::<Vec<_>>()).collect();
    let (calls, before_logic) = *calls.lock().unwrap();
    json!({"before":before,"after":optional_view(&entity,name),"copy":optional_view(&copy,name),
           "diagnostics":diagnostics,"calls":calls,"before_logic":before_logic,"text":entity.text()})
}
fn main() {
    for line in io::stdin().lock().lines() {
        let input: Json = serde_json::from_str(&line.unwrap()).unwrap();
        let result = std::panic::catch_unwind(|| match input["kind"].as_str().unwrap() {
            "scalar" => scalar(&input), "field" => field_probe(&input),
            "optional" => optional_probe(&input), _ => relations(&input)
        });
        let output = result.unwrap_or_else(|error| {
            let message = error.downcast_ref::<String>().cloned().or_else(|| error.downcast_ref::<&str>().map(|v|v.to_string())).unwrap();
            json!({"error":message})
        });
        println!("{}", output);
    }
}
