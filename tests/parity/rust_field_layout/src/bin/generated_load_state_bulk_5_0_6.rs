//! Real locked generated model batches; retain every row value and sharing result.
use serde_json::{Value as Json, json};
use std::{io::{self, BufRead}, sync::Arc};
use teaql_core::{CompactRow, CompactRowLayout, Entity, Value};

fn payload(index: usize) -> String {
    format!("row-{index}-{}", "x".repeat(64))
}

macro_rules! observe {
    ($model:ty, $input:expr) => {{
        let input = $input;
        let count = input["count"].as_u64().unwrap() as usize;
        assert!([1, 100, 10000].contains(&count));
        let independent = input["independent"].as_bool().unwrap();
        let shape = || CompactRowLayout::new(Arc::from([
            "id".to_owned(), "version".to_owned(), "type_field".to_owned(),
        ]));
        let columns = shape();
        let mut rows: Vec<_> = (0..count).map(|index| {
            <$model>::from_compact_row(CompactRow::with_layout(
                if independent { shape() } else { columns.clone() },
                vec![Value::U64(index as u64 + 1), Value::I64(1),
                     if index == 0 { Value::Null } else { Value::Text(payload(index)) }],
            )).unwrap()
        }).collect();
        let first = rows[0].loaded_state_snapshot().expect("actual generated indexed state");
        let mut shared = true;
        let mut shared_layout = true;
        let mut no_overflow = true;
        let mut no_names = true;
        let mut loaded = true;
        let mut omitted = true;
        let mut clean = true;
        let mut private_values = true;
        let mut values = Vec::new();
        for (index, row) in rows.iter().enumerate() {
            let state = row.loaded_state_snapshot().unwrap();
            shared &= Arc::ptr_eq(&first, &state);
            shared_layout &= Arc::ptr_eq(first.layout(), state.layout());
            no_overflow &= state.overflow().is_none();
            no_names &= state.selected_names().is_none();
            loaded &= ["type", "type_field", "legacy_type"].iter().all(|name| row.is_field_loaded(name));
            omitted &= !row.is_field_loaded("name") && !row.is_field_loaded("platform_id");
            clean &= row.dirty_fields().is_none() && !row.is_new();
            private_values &= row.id() == index as u64 + 1 &&
                row.type_field() == if index == 0 { None } else { Some(payload(index)) };
            values.push(json!({"id":row.id(),"value":row.type_field()}));
        }
        rows.last_mut().unwrap().update_type_field("changed-last-row-payload");
        let siblings = rows[..count-1].iter().enumerate().all(|(index,row)| {
            row.type_field() == if index == 0 { None } else { Some(payload(index)) }
        });
        let value_keeps = Arc::ptr_eq(&first, &rows.last().unwrap().loaded_state_snapshot().unwrap());
        let updated = rows.last().unwrap().type_field().as_deref() == Some("changed-last-row-payload");
        rows.last_mut().unwrap().set_checker_loaded_fields(std::collections::BTreeSet::from(["id".to_owned()]));
        let detached = !Arc::ptr_eq(&first, &rows.last().unwrap().loaded_state_snapshot().unwrap());
        let narrowed = rows.last().unwrap().is_field_loaded("id") &&
            !rows.last().unwrap().is_field_loaded("version") && !rows.last().unwrap().is_field_loaded("type_field");
        let sibling_projection = count == 1 ||
            (Arc::ptr_eq(&first, &rows[0].loaded_state_snapshot().unwrap()) && rows[0].is_field_loaded("type_field"));
        json!({"rows":values,"shared_snapshot":shared,"shared_layout":shared_layout,
               "overflow_absent":no_overflow,"selected_names_absent":no_names,
               "all_aliases_loaded":loaded,"omitted_unloaded":omitted,"clean":clean,
               "private_values":private_values,"siblings_unchanged":siblings,
               "value_keeps_snapshot":value_keeps,"updated_value":updated,
               "checker_detaches":detached,"checker_narrows":narrowed,
               "sibling_projection":sibling_projection})
    }};
}

fn main() {
    for line in io::stdin().lock().lines() {
        let input: Json = serde_json::from_str(&line.unwrap()).unwrap();
        let output = if input["model"] == "record" {
            observe!(cpp_member_aliases_core::Record, &input)
        } else {
            observe!(cpp_member_aliases_core::ChildRecord, &input)
        };
        println!("{output}");
    }
}
