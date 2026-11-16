//! Execute the unchanged reference derive rather than copying its conversion rules.
use serde_json::{json, Value as Json};
use std::{collections::BTreeMap, io::{self, BufRead}};
use teaql_core::{eval::LoadState, CompactRow, Entity, Value};

#[path = "rust_values/src/fixture_value.rs"]
mod fixture_value;

macro_rules! model {
    ($name:ident, $($value:tt)+) => {
        #[derive(Clone, Debug, teaql_macros::TeaqlEntity)]
        struct $name {
            #[teaql(id)]
            id: u64,
            #[teaql(version)]
            version: i64,
            value: $($value)+,
            #[teaql(skip)]
            __load_state: LoadState,
        }
    };
}

model!(I16, i16);
model!(I32, i32);
model!(U16, u16);
model!(U32, u32);
model!(OptionalI16, Option<i16>);
model!(OptionalI32, Option<i32>);
model!(OptionalU16, Option<u16>);
model!(OptionalU32, Option<u32>);

fn observe(input: &Json) -> Json {
    let mut values = BTreeMap::from([
        ("id".to_owned(), Value::U64(1)),
        ("version".to_owned(), Value::I64(1)),
    ]);
    if input["present"] == true {
        values.insert("value".to_owned(), fixture_value::value(&input["value"]));
    }
    let row = CompactRow::from_map(values);
    macro_rules! decode {
        ($model:ty) => {
            match <$model>::from_compact_row(row) {
                Ok(value) => json!({"ok": true, "value": value.value}),
                Err(error) => json!({"ok": false, "entity": error.entity, "error": error.message}),
            }
        };
    }
    match (input["carrier"].as_str().unwrap(), input["optional"] == true) {
        ("i16", false) => decode!(I16),
        ("i32", false) => decode!(I32),
        ("u16", false) => decode!(U16),
        ("u32", false) => decode!(U32),
        ("i16", true) => decode!(OptionalI16),
        ("i32", true) => decode!(OptionalI32),
        ("u16", true) => decode!(OptionalU16),
        ("u32", true) => decode!(OptionalU32),
        _ => panic!("unknown narrow integer case"),
    }
}

fn main() {
    for line in io::stdin().lock().lines() {
        let input = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", observe(&input));
    }
}
