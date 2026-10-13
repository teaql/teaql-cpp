//! Observe the original precomputed readonly-schema hash and complete diagnostics.
use serde_json::{Value as Json, json};
use std::{hash::{Hash, Hasher}, io::{self, BufRead}};
use teaql_core::{CompactRow, CompactRowLayout, DataType, SelectQuery};
use teaql_core::dynamic_properties::DynamicPropertyDefinitions;

#[derive(Default)]
struct Capture(u64);
impl Hasher for Capture {
    fn finish(&self) -> u64 { self.0 }
    fn write(&mut self, _: &[u8]) { panic!("schema Hash must write its cached u64"); }
    fn write_u64(&mut self, value: u64) { self.0 = value; }
}

fn observe(input: &Json) -> Json {
    let types = [DataType::Bool, DataType::I64, DataType::U64, DataType::F64,
        DataType::Decimal, DataType::Text, DataType::LargeText, DataType::Json,
        DataType::Date, DataType::Timestamp];
    let fields = input["fields"].as_array().unwrap().iter().map(|pair| {
        (pair[0].as_str().unwrap().to_owned(), types[pair[1].as_u64().unwrap() as usize])
    }).collect::<Vec<_>>();
    let definitions = DynamicPropertyDefinitions::new(fields).unwrap();
    let mut hash = Capture::default();
    definitions.hash(&mut hash);
    let columns = CompactRowLayout::new(Vec::<String>::new().into())
        .with_dynamic_property_definitions(definitions.clone());
    let row = CompactRow::with_layout(columns.clone(), vec![]);
    let mut query = SelectQuery::new("Item");
    query.dynamic_property_definitions = Some(definitions.clone());
    json!({"hash":hash.finish().to_string(),"definitions":format!("{definitions:?}"),
           "definitions_pretty":format!("{definitions:#?}"),
           "layout":format!("{columns:?}"),"row":format!("{row:?}"),
           "query":format!("{query:?}")})
}

fn main() {
    for line in io::stdin().lock().lines() {
        println!("{}", observe(&serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
