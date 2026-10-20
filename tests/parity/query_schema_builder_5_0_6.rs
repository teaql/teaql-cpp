//! Call the original schema builder and observe immutable binding identity.
use serde_json::{Value as Json, json};
use std::{io::{self, BufRead}, sync::Arc};
use teaql_core::{DataType, SelectQuery};
use teaql_core::dynamic_properties::DynamicPropertyDefinitions;

fn formatted<T: std::fmt::Debug>(value: &T) -> Json {
    json!({"compact":format!("{value:?}"),"pretty":format!("{value:#?}"),
        "lower_hex":format!("{value:x?}"),"upper_hex":format!("{value:X?}"),
        "pretty_lower_hex":format!("{value:#x?}"),"pretty_upper_hex":format!("{value:#X?}"),
        "width_precision":format!("{value:>40.2?}"),"fancy":format!("{value:🧊^+18.3?}"),
        "zero_pad":format!("{value:+012?}")})
}

fn definitions(fields: &Json) -> Arc<DynamicPropertyDefinitions> {
    let types = [DataType::Bool, DataType::I64, DataType::U64, DataType::F64,
        DataType::Decimal, DataType::Text, DataType::LargeText, DataType::Json,
        DataType::Date, DataType::Timestamp];
    DynamicPropertyDefinitions::new(fields.as_array().unwrap().iter().map(|field|
        (field[0].as_str().unwrap().to_owned(), types[field[1].as_u64().unwrap() as usize]))).unwrap()
}

fn observe(input: &Json) -> Json {
    let a = definitions(&input["first"]);
    let b = definitions(&input["second"]);
    let equal = definitions(&input["first"]);
    let flags = input["flags"].as_u64().unwrap();
    let mut query = SelectQuery::new("Order\0雪");
    if flags & 1 != 0 { query = query.project("id"); }
    if flags & 2 != 0 { query = query.comment("preserved\0comment"); }
    if flags & 4 != 0 { query = query.purpose("preserved\0purpose"); }
    let before = formatted(&query);
    query = query.with_dynamic_property_definitions(a.clone());
    let copied = query.clone();
    let first = formatted(&query);
    let first_identity = Arc::ptr_eq(query.dynamic_property_definitions.as_ref().unwrap(), &a);
    let clone_identity = Arc::ptr_eq(copied.dynamic_property_definitions.as_ref().unwrap(), &a);
    query = query.with_dynamic_property_definitions(b.clone());
    let second = formatted(&query);
    let second_identity = Arc::ptr_eq(query.dynamic_property_definitions.as_ref().unwrap(), &b);
    let second_equals_first = query == copied;
    query = query.with_dynamic_property_definitions(equal.clone());
    let equal_identity = Arc::ptr_eq(query.dynamic_property_definitions.as_ref().unwrap(), &equal);
    let distinct_equal = !Arc::ptr_eq(query.dynamic_property_definitions.as_ref().unwrap(), &a);
    let same = query.dynamic_property_definitions.clone().unwrap();
    query = query.with_dynamic_property_definitions(same);
    let prepared = query.clone().prepare_for_list().unwrap();
    let prepare_identity = Arc::ptr_eq(query.dynamic_property_definitions.as_ref().unwrap(),
                                     prepared.dynamic_property_definitions.as_ref().unwrap());
    drop((a, b, equal));
    json!({"before":before,"first":first,"first_identity":first_identity,
        "clone_identity":clone_identity,"second":second,"second_identity":second_identity,
        "second_equals_first":second_equals_first,"equal_identity":equal_identity,
        "distinct_equal":distinct_equal,"restored_equals_first":query==copied,
        "after_input_release":formatted(&query),"original_clone":formatted(&copied),
        "prepared":formatted(&prepared),"prepare_identity":prepare_identity})
}

fn main() {
    for line in io::stdin().lock().lines() {
        println!("{}", observe(&serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
