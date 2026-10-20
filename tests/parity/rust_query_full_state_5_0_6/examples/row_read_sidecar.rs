use serde_json::{Value as Json, json};
use std::{collections::HashMap, io::{self, BufRead}};
use teaql_core::{CompactRow, DataType, SmartList, Value};
use teaql_core::dynamic_fields::{DynamicFieldDefinitions, DynamicFieldMergeShapes, DynamicFieldValues};
fn formatted<T: std::fmt::Debug>(value: &T) -> Json {
    json!({"compact":format!("{value:?}"),"pretty":format!("{value:#?}"),
        "lower_hex":format!("{value:x?}"),"upper_hex":format!("{value:X?}"),
        "pretty_lower_hex":format!("{value:#x?}"),"pretty_upper_hex":format!("{value:#X?}"),
        "width_precision":format!("{value:>40.2?}"),"fancy":format!("{value:🧊^+18.3?}"),
        "zero_pad":format!("{value:+012?}")})
}
fn row(profile: usize, name: &str, loaded: bool) -> CompactRow {
    let mut row=CompactRow::new(vec!["id".to_owned()].into(),vec![Value::U64(7)]);
    if [1,2,7,8].contains(&profile) {
        let mut list=SmartList::new(Vec::new());
        list.is_loaded=loaded;
        if profile==7 { list.push(CompactRow::new(vec![name.to_owned()].into(),vec![Value::Text("payload\0雪".repeat(32))])); }
        row.set_loaded_relation(name.to_owned(),list);
        if profile==1 { row.take_loaded_relation(name); }
    }
    if [3,4,5,6,8].contains(&profile) {
        let definitions=DynamicFieldDefinitions::new("Row","revision",[("memo".to_owned(),DataType::Text)]).unwrap();
        let mut values=HashMap::new();
        if [5,8].contains(&profile) { values.insert("memo".to_owned(),Value::Null); }
        if profile==6 { values.insert("memo".to_owned(),Value::Text("dynamic\0雪".repeat(32))); }
        row.set_loaded_dynamic_fields(DynamicFieldValues::from_values(definitions,values).unwrap());
        if profile==3 { row.take_loaded_dynamic_fields(); }
    }
    row
}
fn observe(input: Json) -> Json {
    let name=input["name"].as_str().unwrap();let loaded=input["loaded"].as_bool().unwrap();
    let mut left=row(input["left"].as_u64().unwrap() as usize,name,loaded);
    let right=row(input["right"].as_u64().unwrap() as usize,name,loaded);
    let initial=left.clone();let pristine=row(0,name,loaded);let before=formatted(&left);let right_before=formatted(&right);
    let error=if input["checked"].as_bool().unwrap() {
        left.try_extend(right.clone(),&mut DynamicFieldMergeShapes::default()).err().map(|e|formatted(&e))
    } else { left.extend(right.clone());None };
    let copied=left.clone();let merged=formatted(&left);
    let equal_pristine=left==pristine;let equal_clone=left==copied;let has_relations=left.has_loaded_relations();
    let relation=left.take_loaded_relation(name).map(|v|formatted(&v));let after_relation=formatted(&left);
    let dynamic=left.take_loaded_dynamic_fields().map(|v|formatted(&v));let after_dynamic=formatted(&left);
    let json=teaql_core::compact_row_to_json_value(&left);
    left.clear_loaded_relations();
    json!({"before":before,"right":right_before,"error":error,"merged":merged,
        "equal_pristine":equal_pristine,"equal_clone":equal_clone,"has_relations":has_relations,
        "relation":relation,"after_relation":after_relation,"dynamic":dynamic,"after_dynamic":after_dynamic,
        "json":json,"cleared":formatted(&left),"cleared_equals_pristine":left==pristine,
        "initial_unchanged":formatted(&initial)==before,"right_unchanged":formatted(&right)==right_before})
}
fn main() {
    for line in io::stdin().lock().lines() { println!("{}",observe(serde_json::from_str(&line.unwrap()).unwrap())); }
}
