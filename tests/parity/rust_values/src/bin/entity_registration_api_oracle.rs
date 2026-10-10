use std::io::{self, BufRead};
use serde_json::{json, Value};
use teaql_core::{EntityDescriptor, EntityDescriptorStore, TeaqlEntity};

struct DefaultEntity;
impl TeaqlEntity for DefaultEntity {
    const ENTITY_NAME: &'static str = "Default";
    fn entity_descriptor() -> EntityDescriptor { EntityDescriptor::new(Self::ENTITY_NAME) }
}

struct CustomEntity;
impl TeaqlEntity for CustomEntity {
    const ENTITY_NAME: &'static str = "Custom";
    fn entity_descriptor() -> EntityDescriptor { EntityDescriptor::new(Self::ENTITY_NAME) }
    fn register_into(store: &mut impl EntityDescriptorStore) {
        store.register_descriptor(EntityDescriptor::new("OverrideA"));
        store.register_descriptor(EntityDescriptor::new("OverrideB"));
    }
}

struct SilentEntity;
impl TeaqlEntity for SilentEntity {
    const ENTITY_NAME: &'static str = "Silent";
    fn entity_descriptor() -> EntityDescriptor { EntityDescriptor::new(Self::ENTITY_NAME) }
    fn register_into(_store: &mut impl EntityDescriptorStore) {}
}

struct Store { names: Vec<String>, attempted: usize, limit: usize }
impl EntityDescriptorStore for Store {
    fn register_descriptor(&mut self, descriptor: EntityDescriptor) {
        self.attempted += 1;
        assert!(self.names.len() < self.limit, "fixture store rejects registration");
        self.names.push(descriptor.name);
    }
}

fn evaluate(input: Value) -> Value {
    let mut store = Store { names: Vec::new(), attempted: 0, limit: input["limit"].as_u64().unwrap() as usize };
    let completed = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
        match input["scenario"].as_u64().unwrap() {
            0 => teaql_core::register_entities!(&mut store, DefaultEntity),
            1 => teaql_core::register_entities!(&mut store, CustomEntity),
            2 => teaql_core::register_entities!(&mut store, SilentEntity),
            3 => teaql_core::register_entities!(&mut store, DefaultEntity, CustomEntity, SilentEntity, DefaultEntity,),
            4 => teaql_core::register_entities!(&mut store, CustomEntity, DefaultEntity, CustomEntity),
            5 => teaql_core::register_entities!(&mut store, SilentEntity, DefaultEntity, DefaultEntity),
            _ => unreachable!(),
        }
    })).is_ok();
    json!({"names":store.names,"attempted":store.attempted,"completed":completed})
}

fn main() {
    std::panic::set_hook(Box::new(|_| {}));
    for line in io::stdin().lock().lines() {
        println!("{}", evaluate(serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
