use serde_json::{Value as Json, json};
use std::{
    cell::Cell,
    collections::BTreeMap,
    io::{self, BufRead},
    rc::Rc,
    sync::Arc,
};
use teaql_core::dynamic_properties::DynamicPropertyDefinitions;
use teaql_core::{
    CompactRow, CompactRowLayout, DataType, Entity, EntityDescriptor, EntityError,
    EntityJsonTraversal, MutationValues, RelationShapeCache, TeaqlEntity, Value,
};

struct PresentationFixture {
    borrowed: bool,
    payload: Json,
    consumed: Rc<Cell<usize>>,
}
impl TeaqlEntity for PresentationFixture {
    const ENTITY_NAME: &'static str = "Fixture";
    fn entity_descriptor() -> EntityDescriptor {
        EntityDescriptor::new(Self::ENTITY_NAME)
    }
}
impl Entity for PresentationFixture {
    fn from_compact_row(_: CompactRow) -> Result<Self, EntityError> {
        unreachable!()
    }
    fn into_values(self) -> MutationValues {
        self.consumed.set(self.consumed.get() + 1);
        BTreeMap::from([("id".into(), Value::U64(7))]).into()
    }
    fn into_json_values(self) -> BTreeMap<String, Value> {
        self.consumed.set(self.consumed.get() + 10);
        BTreeMap::from([
            ("id".into(), Value::U64(7)),
            ("_is_new".into(), Value::Bool(true)),
            ("_readonly".into(), Value::Bool(false)),
        ])
    }
    fn borrowed_json(&self, traversal: &mut EntityJsonTraversal) -> Option<Json> {
        if !self.borrowed {
            return None;
        }
        assert!(traversal.enter(Self::ENTITY_NAME, self as *const Self as usize));
        traversal.leave();
        Some(self.payload.clone())
    }
}

fn observe(input: &Json) -> Json {
    match input["kind"].as_str().unwrap() {
        "layout" => {
            let names: Arc<[String]> = input["names"]
                .as_array()
                .unwrap()
                .iter()
                .map(|v| v.as_str().unwrap().to_owned())
                .collect::<Vec<_>>()
                .into();
            let initial = CompactRowLayout::new(names);
            let mut row =
                CompactRow::with_layout(initial.clone(), vec![Value::Null; initial.len()]);
            let mut cache = RelationShapeCache::default();
            for relation in input["relations"].as_array().unwrap() {
                row.mark_relation_loaded(relation.as_str().unwrap(), &mut cache);
            }
            let base = row.shared_layout();
            let mut current = base.clone();
            let stages: Vec<_> = input["schemas"].as_array().unwrap().iter().map(|schema| {
                let definitions = DynamicPropertyDefinitions::new(schema.as_array().unwrap().iter()
                    .map(|entry| (entry[0].as_str().unwrap().to_owned(),
                         if entry[1] == "text" { DataType::Text } else { DataType::I64 }))).unwrap();
                let next = current.with_dynamic_property_definitions(definitions.clone());
                let next_row = CompactRow::with_layout(next.clone(), vec![Value::Null; next.len()]);
                let out = json!({"reused": Arc::ptr_eq(&next, &current), "equals_base": next == base,
                    "idempotent": Arc::ptr_eq(&next, &next.with_dynamic_property_definitions(definitions)),
                    "equal_copy": next == next.clone(), "names": next.iter().collect::<Vec<_>>(),
                    "relations": input["relations"].as_array().unwrap().iter().map(|r| next_row.is_loaded_relation(r.as_str().unwrap())).collect::<Vec<_>>()});
                current = next;
                out
            }).collect();
            json!(stages)
        }
        "presentation" => {
            let consumed = Rc::new(Cell::new(0));
            let model = PresentationFixture {
                borrowed: input["borrowed"].as_bool().unwrap(),
                payload: input["payload"].clone(),
                consumed: consumed.clone(),
            };
            let output = model.into_json();
            json!({"json": output, "consumed": consumed.get()})
        }
        _ => {
            let mut traversal = EntityJsonTraversal::default();
            let operations: Vec<_> = input["operations"]
                .as_array()
                .unwrap()
                .iter()
                .map(|operation| {
                    if operation[0] == "leave" {
                        traversal.leave();
                        Json::Null
                    } else {
                        let entity = match operation[1].as_str().unwrap() {
                            "Item" => "Item",
                            "Other" => "Other",
                            "" => "",
                            _ => panic!("unknown fixture type"),
                        };
                        json!(traversal.enter(entity, operation[2].as_u64().unwrap() as usize))
                    }
                })
                .collect();
            json!(operations)
        }
    }
}

fn main() {
    for line in io::stdin().lock().lines() {
        let input: Json = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", observe(&input));
    }
}
