use serde_json::{Value, json};
use std::collections::BTreeMap;
use std::io::{self, BufRead};
use teaql_core::{CompactRow, Entity, Value as TeaqlValue};
fn row(id: u64) -> BTreeMap<String, TeaqlValue> {
    BTreeMap::from([
        ("id".into(), TeaqlValue::U64(id)),
        ("version".into(), TeaqlValue::I64(1)),
    ])
}
fn generated(mode: &str, id: u64) {
    if mode.starts_with("constant_") {
        let record =
            cpp_candidates_core::Record::from_compact_row(CompactRow::from_map(row(id))).unwrap();
        match mode {
            "constant_draft" => {
                cpp_candidates_core::E::record(&record)
                    .status_is_draft()
                    .unwrap();
            }
            "constant_progress" => {
                cpp_candidates_core::E::record(&record)
                    .status_is_in_progress()
                    .eval();
            }
            "constant_done" => {
                cpp_candidates_core::E::record(&record)
                    .status_is_done()
                    .or_if_null(false);
            }
            "constant_status" => {
                cpp_candidates_core::E::record(&record)
                    .get_status()
                    .unwrap();
            }
            "constant_phase" => {
                cpp_candidates_core::E::record(&record).get_phase().unwrap();
            }
            _ => panic!("unknown constant mode"),
        }
        return;
    }
    if mode.starts_with("inherited_") {
        let academy =
            cpp_inheritance_core::ResearchAcademy::from_compact_row(CompactRow::from_map(row(id)))
                .unwrap();
        match mode {
            "inherited_name" => {
                cpp_inheritance_core::E::research_academy(&academy)
                    .get_name()
                    .unwrap();
            }
            "inherited_campus" => {
                cpp_inheritance_core::E::research_academy(&academy)
                    .get_campus_code()
                    .eval();
            }
            "inherited_budget" => {
                cpp_inheritance_core::E::research_academy(&academy)
                    .get_budget()
                    .unwrap();
            }
            _ => panic!("unknown inherited mode"),
        }
        return;
    }
    let mut fields = row(id);
    if mode == "loaded_platform_name" {
        fields.insert("platform".into(), TeaqlValue::Object(row(2)));
    }
    let person =
        cpp_dynamic_json_core::Person::from_compact_row(CompactRow::from_map(fields)).unwrap();
    match mode {
        "name" => {
            cpp_dynamic_json_core::E::person(&person)
                .get_name()
                .unwrap();
        }
        "name_eval" => {
            cpp_dynamic_json_core::E::person(&person).get_name().eval();
        }
        "fallback" => {
            cpp_dynamic_json_core::E::person(&person)
                .get_name()
                .or_if_null("fallback".into());
        }
        "fallback_callback" => {
            cpp_dynamic_json_core::E::person(&person)
                .get_name()
                .or_else_if_null(|| panic!("fallback was invoked"));
        }
        "fallback_default" => {
            cpp_dynamic_json_core::E::person(&person)
                .get_name()
                .or_default_if_null();
        }
        "platform_name" | "loaded_platform_name" => {
            cpp_dynamic_json_core::E::person(&person)
                .get_platform()
                .get_name()
                .unwrap();
        }
        "platform_id" => {
            cpp_dynamic_json_core::E::person(&person)
                .get_platform_id()
                .unwrap();
        }
        "profile_note" => {
            cpp_dynamic_json_core::E::person(&person)
                .get_profile()
                .get_note()
                .unwrap();
        }
        "orders_first_code" => {
            cpp_dynamic_json_core::E::person(&person)
                .get_orders()
                .first()
                .get_code()
                .unwrap();
        }
        "orders_get_code" => {
            cpp_dynamic_json_core::E::person(&person)
                .get_orders()
                .get(100)
                .get_code()
                .unwrap();
        }
        "orders_eval" => {
            cpp_dynamic_json_core::E::person(&person)
                .get_orders()
                .eval();
        }
        "orders_unwrap" => {
            cpp_dynamic_json_core::E::person(&person)
                .get_orders()
                .unwrap();
        }
        "list" => {
            cpp_dynamic_json_core::E::person(&person)
                .get_orders()
                .size()
                .unwrap();
        }
        _ => panic!("unknown person mode"),
    }
}
fn main() {
    std::panic::set_hook(Box::new(|_| {}));
    for line in io::stdin().lock().lines() {
        let input: Value = serde_json::from_str(&line.unwrap()).unwrap();
        let root = input["root"].as_str().unwrap();
        let node = input["node"].as_str().unwrap();
        let path = input["path"].as_str().unwrap();
        let panic = std::panic::catch_unwind(|| {
            if let Some(mode) = input["generated"].as_str() {
                generated(mode, input["id"].as_u64().unwrap());
            } else {
                cpp_dynamic_json_core::trigger_logic_bug_panic(root, node, path);
            }
        })
        .unwrap_err();
        let text = panic.downcast_ref::<String>().unwrap();
        let mut fields = serde_json::Map::new();
        for line in text.lines() {
            if let Some((name, value)) = line.split_once(": ") {
                fields.insert(
                    name.into(),
                    if name == "root" {
                        json!(value)
                    } else if name == "human_message" {
                        json!(&value[1..value.len() - 1])
                    } else {
                        serde_json::from_str(value).unwrap()
                    },
                );
            }
        }
        println!("{}", Value::Object(fields));
    }
}
