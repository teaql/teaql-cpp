use cpp_checker_clock_scalars_core::{Note, NoteChecker};
use serde_json::{json, Value as Json};
use std::{collections::BTreeMap, io::{self, BufRead}};
use teaql_core::{CompactRow, Entity, Value, eval::LoadState};
use teaql_runtime::{CheckObjectStatus, JsonFieldNamingProfile, ObjectLocation, TypedChecker, UserContext};

fn value(input: &Json) -> Value {
    let data = &input["value"];
    match input["kind"].as_str().unwrap() {
        "null" => Value::Null,
        "text" => Value::Text(data.as_str().unwrap().into()),
        "bool" => Value::Bool(data.as_bool().unwrap()),
        "i64" => Value::I64(data.as_str().unwrap().parse().unwrap()),
        "f64" => Value::F64(data.as_str().unwrap().parse().unwrap()),
        "decimal" => Value::Decimal(data.as_str().unwrap().parse().unwrap()),
        "date" => Value::Date(data.as_str().unwrap().parse().unwrap()),
        "timestamp" => Value::Timestamp(teaql_core::time::Timestamp(data.as_str().unwrap().parse().unwrap())),
        _ => Value::Json(data.clone()),
    }
}

fn main() {
    for line in io::stdin().lock().lines() {
        let input: Json = serde_json::from_str(&line.unwrap()).unwrap();
        let user_context = UserContext::new();
        let mut row = BTreeMap::from([("id".into(), Value::U64(7)), ("version".into(), Value::I64(3))]);
        for (name, data) in input["values"].as_object().unwrap() {
            row.insert(name.clone(), value(data));
        }
        let mut entity = Note::from_compact_row(CompactRow::from_map(row)).unwrap();
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
        let mut fields = serde_json::Map::new();
        macro_rules! observe {
            ($($name:ident => $changed:ident),* $(,)?) => {$({
                let changed = entity.$changed();
                fields.insert(stringify!($name).into(), json!({"stored":entity.$name(),
                    "loaded":entity.is_loaded(stringify!($name)), "changed_present":changed.is_some(),
                    "changed_value":changed.as_ref().map(Value::to_json_value),
                    "timestamp_operand":matches!(changed, Some(Value::Timestamp(_)))}));
            })*};
        }
        observe!(text=>changed_text, optional_text=>changed_optional_text,
            code=>changed_code, optional_code=>changed_optional_code,
            integer=>changed_integer, optional_integer=>changed_optional_integer,
            long=>changed_long, optional_long=>changed_optional_long,
            decimal=>changed_decimal, optional_decimal=>changed_optional_decimal,
            float=>changed_float, optional_float=>changed_optional_float,
            double=>changed_double, optional_double=>changed_optional_double,
            active=>changed_active, optional_active=>changed_optional_active,
            date=>changed_date, optional_date=>changed_optional_date,
            time=>changed_time, optional_time=>changed_optional_time,
            payload=>changed_payload, optional_payload=>changed_optional_payload,
            timestamp=>changed_timestamp);
        let diagnostics = [JsonFieldNamingProfile::CamelCase, JsonFieldNamingProfile::SnakeCase,
            JsonFieldNamingProfile::PascalCase].map(|profile|
                findings.iter().map(|v|v.to_wire(profile)).collect::<Vec<_>>());
        println!("{}", json!({"fields":fields,"diagnostics":diagnostics,
            "wall_millis":teaql_core::time::Timestamp::now().0}));
    }
}
