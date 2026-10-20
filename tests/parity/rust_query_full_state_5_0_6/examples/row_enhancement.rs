use serde_json::{Value as Json, json};
use std::{collections::HashMap, io::{self, BufRead}, sync::Arc};
use teaql_core::{CompactRow, CompactRowLayout, DataType, RelationShapeCache, SmartList, Value};
use teaql_core::dynamic_fields::{DynamicFieldDefinitions, DynamicFieldMergeShapes, DynamicFieldValues};
use teaql_core::dynamic_properties::DynamicPropertyDefinitions;
fn dt(n: u64) -> DataType { [DataType::Bool,DataType::I64,DataType::U64,DataType::F64,DataType::Decimal,DataType::Text,DataType::LargeText,DataType::Json,DataType::Date,DataType::Timestamp][n as usize] }
#[path = "../../rust_values/src/fixture_value.rs"]
mod fixture_value;
use fixture_value::value;
fn formatted<T: std::fmt::Debug>(value: &T) -> Json {
    json!({"compact":format!("{value:?}"),"pretty":format!("{value:#?}"),
        "lower_hex":format!("{value:x?}"),"upper_hex":format!("{value:X?}"),
        "pretty_lower_hex":format!("{value:#x?}"),"pretty_upper_hex":format!("{value:#X?}"),
        "width_precision":format!("{value:>40.2?}"),"fancy":format!("{value:🧊^+18.3?}"),"zero_pad":format!("{value:+012?}")})
}
fn definitions(v: &Json) -> Arc<DynamicPropertyDefinitions> {
    DynamicPropertyDefinitions::new(v.as_array().unwrap().iter().map(|p|(p[0].as_str().unwrap().to_owned(),dt(p[1].as_u64().unwrap())))).unwrap()
}
fn row(v: &Json) -> CompactRow {
    let names:Vec<String>=v["names"].as_array().unwrap().iter().map(|s|s.as_str().unwrap().to_owned()).collect();
    let values=v["values"].as_array().unwrap().iter().map(value).collect();
    let mut layout=CompactRowLayout::new(names.into());
    if !v["schema"].is_null() { layout=layout.with_dynamic_property_definitions(definitions(&v["schema"])); }
    let mut row=CompactRow::with_layout(layout,values);
    if !v["dynamic"].is_null() {
        let d=&v["dynamic"];
        let mut schema=DynamicFieldDefinitions::new(d["owner"].as_str().unwrap(),d["revision"].as_str().unwrap(),[("memo".to_owned(),dt(d["type"].as_u64().unwrap()))]).unwrap();
        if !d["binding"].is_null() { schema=schema.with_storage_binding(d["binding"]["profile"].as_str().unwrap(),d["binding"]["executor"].as_u64().unwrap()); }
        let values=HashMap::from([("memo".to_owned(),value(&d["value"]))]);
        row.set_loaded_dynamic_fields(DynamicFieldValues::from_values(schema,values).unwrap());
    }
    if v["relation"].as_bool().unwrap_or(false) { row.set_loaded_relation("lines".to_owned(),SmartList::new(vec![])); }
    row
}
fn merge(input:&Json)->Json {
    let mut left=row(&input["left"]);let right=row(&input["right"]);let original=left.clone();
    let before=left.shared_layout();let left_before=formatted(&left);let right_before=formatted(&right);
    let error=left.try_extend(right.clone(),&mut DynamicFieldMergeShapes::default()).err().map(|e|formatted(&e));
    json!({"left_before":left_before,"right_before":right_before,"error":error,
        "after":formatted(&left),"original":formatted(&original),"right_after":formatted(&right),
        "left_unchanged":left==original,"layout_same":Arc::ptr_eq(&before,&left.shared_layout()),
        "json":teaql_core::compact_row_to_json_value(&left),
        "validation":left.validate_dynamic_properties().err()})
}
fn state(rows:&[CompactRow])->Json {
    json!({"rows":rows.iter().map(formatted).collect::<Vec<_>>(),
        "shared":rows.iter().map(|a|rows.iter().map(|b|Arc::ptr_eq(&a.shared_layout(),&b.shared_layout())).collect::<Vec<_>>()).collect::<Vec<_>>(),
        "relations":rows.iter().map(|r|r.has_loaded_relations()).collect::<Vec<_>>(),
        "marks":rows.iter().map(|r|["","lines","雪\0"].map(|n|r.is_loaded_relation(n))).collect::<Vec<_>>()})
}
fn shapes(input:&Json)->Json {
    let mut rows:Vec<_>=input["rows"].as_array().unwrap().iter().map(row).collect();
    let initial=state(&rows);let old=rows[0].clone();let mut stages=Vec::new();
    CompactRow::share_layouts(&mut rows);stages.push(state(&rows));
    let mut cache=RelationShapeCache::default();
    for (indexes,name) in [(vec![0,1],"lines"),(vec![2,3],"lines"),(vec![0,2],""),(vec![3],"lines"),(vec![1],""),(vec![0,3],"雪\0")] {
        for i in indexes { rows[i].mark_relation_loaded(name,&mut cache); }
        stages.push(state(&rows));
    }
    CompactRow::attach_dynamic_property_definitions(&mut rows,definitions(&input["schema"]));stages.push(state(&rows));
    CompactRow::attach_dynamic_property_definitions(&mut rows,definitions(&input["schema"]));stages.push(state(&rows));
    CompactRow::share_layouts(&mut rows);stages.push(state(&rows));
    rows[1].insert("extra".to_owned(),Value::Text("payload\0雪".repeat(32)));stages.push(state(&rows));
    let removed=rows[1].remove("lines").map(|v|formatted(&v));stages.push(state(&rows));
    let consumed=rows.pop().unwrap().into_map();let map=formatted(&consumed);
    rows.push(CompactRow::from(consumed));stages.push(state(&rows));
    json!({"initial":initial,"stages":stages,"original":formatted(&old),"removed":removed,"map":map})
}
fn main(){for line in io::stdin().lock().lines(){let input:Json=serde_json::from_str(&line.unwrap()).unwrap();println!("{}",if input["kind"]=="merge"{merge(&input)}else if input["kind"]=="record"{formatted(&row(&input["row"]).into_map())}else{shapes(&input)});}}
