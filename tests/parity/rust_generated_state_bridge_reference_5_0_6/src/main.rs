//! Observe the unchanged locked generated models and their actual runtime derive.
use serde_json::{Value as Json, json};
use std::io::{self, BufRead};
use teaql_core::{CompactRow, Entity, Value};
use teaql_runtime::{EntityRuntimeState, LedgerEntity};

macro_rules! observe {
    ($model:ty, $input:expr) => {{
        let input = $input;
        let mut row = <$model>::from_compact_row(CompactRow::new(
            vec!["id".into(), "version".into()].into(),
            vec![Value::U64(1), Value::I64(1)],
        )).unwrap();
        let (stable, wrong_type, before, typed_sees, automatic_sees, after) = {
            let first = row.__teaql_runtime_state_any().unwrap();
            let second = row.__teaql_runtime_state_any().unwrap();
            let state = first.downcast_ref::<EntityRuntimeState>().unwrap();
            let stable = std::ptr::eq(state, second.downcast_ref::<EntityRuntimeState>().unwrap());
            let wrong_type = first.downcast_ref::<i32>().is_none();
            let before = state.get_comment();
            let typed = row.entity_runtime_state().unwrap();
            let mut payload = "x".repeat(input["size"].as_u64().unwrap() as usize);
            state.set_comment(payload.clone());
            let typed_sees = typed.get_comment().as_ref() == Some(&payload);
            payload.push_str("-typed");
            typed.set_comment(payload.clone());
            let automatic = state.clone();
            let automatic_sees = automatic.get_comment().as_ref() == Some(&payload);
            (stable, wrong_type, before, typed_sees, automatic_sees, state.get_comment())
        };
        row.update_type_field("edited");
        json!({"present":true, "stable_borrow":stable, "wrong_type_rejected":wrong_type,
               "comment_before":before, "typed_sees_borrow":typed_sees,
               "automatic_sees_typed":automatic_sees, "dirty":row.dirty_fields().is_some(),
               "value":row.type_field(), "comment_after":after})
    }};
}

fn main() {
    for line in io::stdin().lock().lines() {
        let input: Json = serde_json::from_str(&line.unwrap()).unwrap();
        let output = match input["model"].as_str().unwrap() {
            "record" => observe!(cpp_member_aliases_core::Record, &input),
            "child_record" => observe!(cpp_member_aliases_core::ChildRecord, &input),
            _ => panic!("Unknown generated model"),
        };
        println!("{output}");
    }
}
