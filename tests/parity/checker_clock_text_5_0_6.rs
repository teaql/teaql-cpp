use cpp_checker_unique_string_core::{Note, NoteChecker};
use serde_json::{json, Value as Json};
use std::{collections::BTreeMap, io::{self, BufRead}};
use teaql_core::{CompactRow, Entity, TeaqlEntity, Value, eval::LoadState};
use teaql_runtime::{CheckObjectStatus, JsonFieldNamingProfile, ObjectLocation, TypedChecker, UserContext};

fn main() {
    for line in io::stdin().lock().lines() {
        let input: Json = serde_json::from_str(&line.unwrap()).unwrap();
        let user_context = UserContext::new();
        let mut entity = Note::from_compact_row(CompactRow::from_map(BTreeMap::from([
            ("id".into(), Value::U64(7)), ("version".into(), Value::I64(3)),
            ("code".into(), Value::Text(input["code"].as_str().unwrap().into()))
        ]))).unwrap();
        if input["sparse"] == true {
            entity.set_load_state(LoadState::Partial(["id".to_owned(), "version".to_owned()].into()));
        }
        let status = match input["status"].as_str().unwrap() {
            "create" => CheckObjectStatus::Create,
            "update" => CheckObjectStatus::Update,
            _ => CheckObjectStatus::Unknown,
        };
        let mut findings = Vec::new();
        NoteChecker::default().check_and_fix_typed(&user_context, &mut entity, status,
            &ObjectLocation::array_root(2).member("note_info"), &mut findings);
        let changed = entity.changed_code();
        let diagnostics = [JsonFieldNamingProfile::CamelCase, JsonFieldNamingProfile::SnakeCase,
            JsonFieldNamingProfile::PascalCase].map(|profile|
                findings.iter().map(|v|v.to_wire(profile)).collect::<Vec<_>>());
        let output = json!({"code":entity.code(), "loaded":entity.is_loaded("code"),
            "changed_present":changed.is_some(), "changed_value":changed.as_ref().map(Value::to_json_value),
            "timestamp_operand":matches!(changed, Some(Value::Timestamp(_))),
            "diagnostics":diagnostics,
            "wall_millis":teaql_core::time::Timestamp::now().0});
        println!("{output}");
    }
}
