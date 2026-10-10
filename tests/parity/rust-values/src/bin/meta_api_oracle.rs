use serde_json::{json, Value as Json};
use std::io::{self, BufRead};
use teaql_core::{DataType, EntityDescriptor, PropertyDescriptor, RelationDescriptor};

fn formatted<T: std::fmt::Debug>(v: &T) -> Json {
    json!({"compact":format!("{v:?}"),"pretty":format!("{v:#?}"),
        "lower_hex":format!("{v:x?}"),"upper_hex":format!("{v:X?}"),
        "pretty_lower_hex":format!("{v:#x?}"),"pretty_upper_hex":format!("{v:#X?}"),
        "width_precision":format!("{v:>40.2?}"),"fancy":format!("{v:🧊^+18.3?}"),
        "zero_pad":format!("{v:+012?}")})
}
fn evaluate(input: Json) -> Json {
    if input["kind"] == "naming" {
        let name = input["name"].as_str().unwrap();
        return json!({"table":teaql_core::default_table_name(name),
            "entity_table":EntityDescriptor::new(name).table_name});
    }
    if input["kind"] == "equality" {
        return equality(&input);
    }
    let types = [DataType::Bool, DataType::I64, DataType::U64, DataType::F64,
        DataType::Decimal, DataType::Text, DataType::LargeText, DataType::Json,
        DataType::Date, DataType::Timestamp];
    let mode = input["mode"].as_u64().unwrap();
    let property = PropertyDescriptor::new(input["property"].as_str().unwrap(), types[input["type"].as_u64().unwrap() as usize]);
    let relation = RelationDescriptor::new(input["relation"].as_str().unwrap(), input["target"].as_str().unwrap());
    let entity = EntityDescriptor::new(input["entity"].as_str().unwrap());
    let defaults = json!({"property":formatted(&property),"relation":formatted(&relation),"entity":formatted(&entity)});
    let mut property = property.column_name(input["column"].as_str().unwrap());
    if mode & 1 != 0 { property = property.not_null(); }
    if mode & 2 != 0 { property = property.id(); }
    if mode & 4 != 0 { property = property.version(); }
    if let Some(n) = input["length"].as_u64() { property = property.max_length(n as u32); }
    if let Some(n) = input["precision"].as_u64() { property = property.numeric_precision(n as u32); }
    if let Some(n) = input["scale"].as_u64() { property = property.numeric_scale(n as u32); }
    let mut relation = relation.local_key(input["local"].as_str().unwrap()).foreign_key(input["foreign"].as_str().unwrap());
    if mode & 1 != 0 { relation = relation.many(); }
    relation = if mode & 2 != 0 { relation.detached().attach().detached() } else { relation.detached().attach() };
    relation = if mode & 4 != 0 { relation.keep_missing().delete_missing().keep_missing() } else { relation.keep_missing().delete_missing() };
    let duplicate = property.clone().column_name("duplicate-column").id().version();
    let duplicate_relation = relation.clone().foreign_key("duplicate-foreign");
    let mut entity = entity.table_name(input["table"].as_str().unwrap())
        .property(property.clone()).property(duplicate).relation(relation.clone()).relation(duplicate_relation);
    if let Some(service) = input["service"].as_str() { entity = entity.data_service(service); }
    if let Some(fields) = input["masks"].as_array() {
        entity = entity.audit_mask_fields(fields.iter().map(|v|v.as_str().unwrap().to_owned()).collect());
    }
    entity = entity.audit_value_max_len(input["max"].as_u64().map(|v|v as usize));
    let cloned = entity.clone();
    let changed = entity.clone().table_name("changed").audit_mask_fields(Vec::new()).audit_value_max_len(None);
    json!({"defaults":defaults,"property":formatted(&property),"relation":formatted(&relation),
        "entity":formatted(&entity),"clone":formatted(&cloned),"changed":formatted(&changed),
        "equal":entity==cloned,"changed_equal":entity==changed,
        "property_lookup":entity.property_by_name(property.name.as_str()).map(|v|format!("{v:?}")),
        "relation_lookup":entity.relation_by_name(relation.name.as_str()).map(|v|format!("{v:?}")),
        "missing_property":entity.property_by_name("definitely missing").is_none(),
        "missing_relation":entity.relation_by_name("definitely missing").is_none(),
        "id":entity.id_property().map(|v|format!("{v:?}")),
        "version":entity.version_property().map(|v|format!("{v:?}")),
        "writable":entity.writable_properties().map(|v|format!("{v:?}")).collect::<Vec<_>>()})
}
fn equality(input: &Json) -> Json {
    let field = input["field"].as_str().unwrap();
    match input["model"].as_str().unwrap() {
        "property" => {
            let before = PropertyDescriptor::new("field", DataType::Text);
            let mut after = before.clone();
            match field {
                "name" => after.name = "changed".into(),
                "data_type" => after.data_type = DataType::LargeText,
                "nullable" => after.nullable = false,
                "column_name" => after.column_name = String::new(),
                "is_id" => after.is_id = true,
                "is_version" => after.is_version = true,
                "max_length" => after.max_length = Some(0),
                "numeric_precision" => after.numeric_precision = Some(0),
                "numeric_scale" => after.numeric_scale = Some(0),
                "same" => (),
                _ => panic!("Unknown property field"),
            }
            json!({"equal":before==after,"before":format!("{before:?}"),"after":format!("{after:?}")})
        }
        "relation" => {
            let before = RelationDescriptor::new("relation", "Target");
            let mut after = before.clone();
            match field {
                "name" => after.name = "changed".into(),
                "target_entity" => after.target_entity = "changed".into(),
                "local_key" => after.local_key = String::new(),
                "foreign_key" => after.foreign_key = String::new(),
                "many" => after.many = true,
                "attach" => after.attach = false,
                "delete_missing" => after.delete_missing = false,
                "same" => (),
                _ => panic!("Unknown relation field"),
            }
            json!({"equal":before==after,"before":format!("{before:?}"),"after":format!("{after:?}")})
        }
        "entity" => {
            let before = EntityDescriptor::new("Entity")
                .property(PropertyDescriptor::new("first", DataType::Text))
                .property(PropertyDescriptor::new("second", DataType::I64))
                .relation(RelationDescriptor::new("first", "First"))
                .relation(RelationDescriptor::new("second", "Second"))
                .audit_mask_fields(vec!["first".into(), "second".into()]);
            let mut after = before.clone();
            match field {
                "name" => after.name = "changed".into(),
                "table_name" => after.table_name = String::new(),
                "data_service" => after.data_service = Some(String::new()),
                "properties" => { after.properties.pop(); },
                "relations" => { after.relations.pop(); },
                "audit_mask_fields" => { after.audit_mask_fields.pop(); },
                "audit_mask_fields_declared" => after.audit_mask_fields_declared = false,
                "audit_value_max_len" => after.audit_value_max_len = Some(0),
                "property_value" => after.properties[0].column_name = "changed".into(),
                "relation_value" => after.relations[0].attach = false,
                "property_order" => after.properties.swap(0, 1),
                "relation_order" => after.relations.swap(0, 1),
                "mask_order" => after.audit_mask_fields.swap(0, 1),
                "same" => (),
                _ => panic!("Unknown entity field"),
            }
            json!({"equal":before==after,"before":format!("{before:?}"),"after":format!("{after:?}")})
        }
        _ => panic!("Unknown descriptor model"),
    }
}
fn main() {
    for line in io::stdin().lock().lines() {
        println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
