//! Original three-name derive model; exercise all ordered subsets, with Text/NULL.
use std::sync::Arc;
use teaql_core::{CompactRow, CompactRowLayout, Entity, Value, eval::LoadState};

// Same model and generated tables as the original shared_load_state.rs test.
#[derive(Clone, Debug, teaql_macros::TeaqlEntity)]
#[teaql(entity = "ThreeNameRow", indexed_layout)]
struct Row {
    #[teaql(id)]
    id: u64,
    #[teaql(version)]
    version: i64,
    #[teaql(column = "legacy_url")]
    display_name: Option<String>,
    #[teaql(skip)]
    __load_state: LoadState,
}

impl Row {
    const __TEAQL_FIELD_LAYOUT_REVISION: &'static str = "three-name-v1";
    const __TEAQL_FIXED_FIELD_INDEXES: &'static [(&'static str, usize)] =
        &[("id", 0), ("version", 1), ("business_name", 2)];
    const __TEAQL_FIXED_FIELD_MAPPINGS: &'static [(&'static str, &'static str, &'static str)] = &[
        ("id", "id", "id"),
        ("version", "version", "version"),
        ("business_name", "display_name", "legacy_url"),
    ];
}

const ALIASES: [&str; 4] = ["business_name", "display_name", "legacy_url", "business_nmae"];

fn visit(order: &mut [usize; 4], count: usize, used: u32) {
    for null_value in [false, true] {
        let mut columns = vec!["id".to_owned(), "version".to_owned()];
        let mut values = vec![Value::U64(1), Value::I64(1)];
        for index in &order[..count] {
            let name = ALIASES[*index];
            columns.push(name.to_owned());
            values.push(if null_value { Value::Null } else { Value::Text(name.to_owned()) });
        }
        let row = Row::from_compact_row(CompactRow::with_layout(
            CompactRowLayout::new(Arc::<[String]>::from(columns)), values,
        )).unwrap();
        assert!(row.dirty_fields().is_none());
        let loaded = row.is_field_loaded("business_name");
        println!("{} {} {} {}", row.display_name.as_deref().unwrap_or(if loaded { "NULL" } else { "MISSING" }),
                 u8::from(loaded), u8::from(row.is_field_loaded("display_name")),
                 u8::from(row.is_field_loaded("legacy_url")));
    }
    for index in 0..ALIASES.len() {
        if used & (1 << index) == 0 {
            order[count] = index;
            visit(order, count + 1, used | (1 << index));
        }
    }
}

fn main() {
    visit(&mut [0; 4], 0, 0);
}
