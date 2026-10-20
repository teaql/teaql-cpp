use cpp_checker_clock_composition_core::{Note, NoteChecker};
use serde_json::{json, Value as Json};
use std::{collections::BTreeMap, io::{self, BufRead}};
use teaql_core::{CompactRow, Entity, Value, eval::LoadState};
use teaql_runtime::{CheckObjectStatus, JsonFieldNamingProfile, ObjectLocation, TypedChecker, UserContext};

fn main() {
    for line in io::stdin().lock().lines() {
        let input: Json = serde_json::from_str(&line.unwrap()).unwrap();
        let mut user_context = UserContext::new();
        if let Some(root) = input["root"].as_u64() {
            user_context = user_context.with_active_root("Tenant", root);
        }
        let id = input["foreign_id"].as_u64().unwrap();
        let mut row = BTreeMap::from([
            ("id".into(), Value::U64(7)), ("version".into(), Value::I64(3)),
            ("code".into(), Value::Text("  界\0parent  ".into())),
            ("count".into(), Value::I64(17)),
            ("payload".into(), Value::Json(json!({"nested":[null,true,"界\0"]}))),
            ("owner_id".into(), Value::U64(id)), ("linked_id".into(), Value::U64(id)),
            ("optional_linked_id".into(), if input["null_optional"]==true {Value::Null} else {Value::U64(id)})
        ]);
        let mut entity = Note::from_compact_row(CompactRow::from_map(std::mem::take(&mut row))).unwrap();
        if input["sparse"] == true {
            entity.set_load_state(LoadState::Partial(["id".to_owned(),"version".to_owned()].into()));
        }
        let status = match input["status"].as_str().unwrap() {
            "create"=>CheckObjectStatus::Create, "update"=>CheckObjectStatus::Update, _=>CheckObjectStatus::Unknown,
        };
        let mut findings=Vec::new();
        NoteChecker::default().check_and_fix_typed(&user_context,&mut entity,status,
            &ObjectLocation::array_root(2).member("note_info"),&mut findings);
        let mut fields=serde_json::Map::new();
        macro_rules! observe {
            ($($name:ident => $changed:ident),* $(,)?) => {$({
                let changed=entity.$changed();
                fields.insert(stringify!($name).into(),json!({"stored":entity.$name(),
                    "loaded":entity.is_loaded(stringify!($name)),"changed_present":changed.is_some(),
                    "changed_value":changed.as_ref().map(Value::to_json_value),
                    "timestamp_operand":matches!(changed,Some(Value::Timestamp(_)))}));
            })*};
        }
        observe!(code=>changed_code,count=>changed_count,payload=>changed_payload,
            owner_id=>changed_owner_id,linked_id=>changed_linked_id,optional_linked_id=>changed_optional_linked_id);
        let diagnostics=[JsonFieldNamingProfile::CamelCase,JsonFieldNamingProfile::SnakeCase,
            JsonFieldNamingProfile::PascalCase].map(|p|findings.iter().map(|v|v.to_wire(p)).collect::<Vec<_>>());
        println!("{}",json!({"fields":fields,"diagnostics":diagnostics,
            "wall_millis":teaql_core::time::Timestamp::now().0}));
    }
}
