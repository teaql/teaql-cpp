use std::sync::Arc;
use teaql_core::{DataType, SelectQuery};
use teaql_core::dynamic_properties::DynamicPropertyDefinitions;

fn main() {
    let definitions = DynamicPropertyDefinitions::new(vec![("_rank".into(), DataType::I64)]).unwrap();
    let query = SelectQuery::new("Order").with_dynamic_property_definitions(definitions.clone());
    assert!(Arc::ptr_eq(query.dynamic_property_definitions.as_ref().unwrap(), &definitions));
    assert!(Arc::ptr_eq(query.clone().dynamic_property_definitions.as_ref().unwrap(), &definitions));
    assert!(query.projection.is_empty());
    assert!(query.trace_chain.is_empty());
    println!("Source schema builder preserves schema identity without selecting fields or adding trace");
}
