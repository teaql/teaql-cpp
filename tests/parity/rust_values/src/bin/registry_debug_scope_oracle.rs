//! Original registry Debug/Clone/Default and replacement/snapshot ordering.
use serde_json::{json, Value as Json};
use std::{fmt::Debug, io::{self, BufRead}};
use teaql_core::{DataType, EntityDescriptor, PropertyDescriptor};
use teaql_data_service::SchemaProvider;
use teaql_runtime::{EntityRegistry, InMemoryEntityRegistry, InMemoryMetadataStore, MetadataStore};

fn formatted<T: Debug>(value: &T) -> Json {
    json!({"compact":format!("{value:?}"),"pretty":format!("{value:#?}"),
        "lower_hex":format!("{value:x?}"),"upper_hex":format!("{value:X?}"),
        "pretty_lower_hex":format!("{value:#x?}"),"pretty_upper_hex":format!("{value:#X?}"),
        "width_precision":format!("{value:>40.2?}"),"fancy":format!("{value:🧊^+18.3?}"),
        "zero_pad":format!("{value:+012?}")})
}
fn descriptor(input: &Json) -> EntityDescriptor {
    let mut result = EntityDescriptor::new(input["name"].as_str().unwrap())
        .table_name(input["table"].as_str().unwrap());
    for property in input["fields"].as_array().unwrap() {
        result = result.property(PropertyDescriptor::new(property.as_str().unwrap(),DataType::Text));
    }
    result
}
fn evaluate(input: Json) -> Json {
    let mut metadata = InMemoryMetadataStore::default();
    let mut registry = InMemoryEntityRegistry::default();
    let defaults = json!({"metadata":formatted(&metadata),"registry":formatted(&registry)});
    for value in input["entries"].as_array().unwrap() {
        metadata.register(descriptor(value));
    }
    for name in input["names"].as_array().unwrap() {
        registry.register(name.as_str().unwrap());
    }
    let cloned_metadata = metadata.clone();
    let cloned_registry = registry.clone();
    let snapshot = metadata.get_entity(input["snapshot"].as_str().unwrap());
    let before = formatted(&metadata);
    let replacement = descriptor(&input["replacement"]);
    metadata = metadata.with_entity(replacement);
    registry = registry.with_entity(input["replacement"]["name"].as_str().unwrap());
    json!({"defaults":defaults,"before":before,"metadata":formatted(&metadata),
        "metadata_clone":formatted(&cloned_metadata),"registry":formatted(&registry),
        "registry_clone":formatted(&cloned_registry),"snapshot":snapshot.as_ref().map(|v|formatted(v.as_ref())),
        "all":metadata.all_entities().iter().map(|v|formatted(*v)).collect::<Vec<_>>(),
        "contains":input["names"].as_array().unwrap().iter().map(|v|registry.contains(v.as_str().unwrap())).collect::<Vec<_>>()})
}
fn main() {
    for line in io::stdin().lock().lines() {
        println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
