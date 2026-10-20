//! Compile and execute original Entity/Audited/boxed defaults without Clone.
use std::collections::BTreeMap;
use teaql_core::{CompactRow, Entity, EntityDescriptor, EntityError, MutationValues,
    TeaqlBoxedRelations, TeaqlEntity, Value};

struct OwnedEntity { payload: String, comment: String }
impl TeaqlEntity for OwnedEntity {
    const ENTITY_NAME: &'static str = "OwnedEntity";
    fn entity_descriptor() -> EntityDescriptor { EntityDescriptor::new(Self::ENTITY_NAME) }
}
impl Entity for OwnedEntity {
    fn from_compact_row(row: CompactRow) -> Result<Self, EntityError> {
        let payload = row.get("payload").and_then(Value::try_text)
            .ok_or_else(|| EntityError::new(Self::ENTITY_NAME, "missing payload"))?;
        Ok(Self { payload: payload.to_owned(), comment: String::new() })
    }
    fn into_values(self) -> MutationValues {
        BTreeMap::from([("payload".into(), Value::Text(self.payload)),
                       ("comment".into(), Value::Text(self.comment))]).into()
    }
    fn set_comment(&mut self, text: String) { self.comment = text; }
}

struct OwnedRelations(String);
impl TeaqlBoxedRelations for OwnedRelations {
    fn extend_descriptor(descriptor: &mut EntityDescriptor) {
        *descriptor = descriptor.clone().property(teaql_core::PropertyDescriptor::new(
            "payload", teaql_core::DataType::Text));
    }
    fn extract_from_values(row: &CompactRow) -> Result<Self, EntityError> {
        row.get("payload").and_then(Value::try_text).map(|v| Self(v.to_owned()))
            .ok_or_else(|| EntityError::new("OwnedRelations", "missing payload"))
    }
    fn inject_into_values(self, values: &mut BTreeMap<String, Value>) {
        values.insert("payload".into(), Value::Text(self.0));
    }
}

fn main() {
    let entity = OwnedEntity { payload: "payload long enough to allocate".into(), comment: String::new() };
    let mut audit = entity.audit_as("  explain  ");
    assert_eq!(audit.get_comment(), "  explain  ");
    assert!(audit.entity().comment.is_empty());
    audit.entity_mut().payload.push_str("-updated");
    let entity = audit.into_entity();
    assert_eq!(entity.comment, "  explain  ");
    let json = entity.into_json();
    assert_eq!(json["payload"], "payload long enough to allocate-updated");
    assert_eq!(json["comment"], "  explain  ");
    let row = CompactRow::from_map(BTreeMap::from([("payload".into(), Value::from("relation"))]));
    let entity = OwnedEntity::from_compact_row_with_context(row.clone(), &()).unwrap();
    assert_eq!(entity.payload, "relation");
    let mut descriptor = EntityDescriptor::new("OwnedRelations");
    Box::<OwnedRelations>::extend_descriptor(&mut descriptor);
    assert_eq!(descriptor.properties.len(), 1);
    assert_eq!(descriptor.properties[0].name, "payload");
    let empty = CompactRow::from_map(BTreeMap::new());
    let error = match Box::<OwnedRelations>::extract_from_values(&empty) {
        Ok(_) => panic!("empty relation unexpectedly decoded"),
        Err(error) => error,
    };
    assert_eq!(error.entity, "OwnedRelations");
    assert_eq!(error.message, "missing payload");
    assert!(std::error::Error::source(&error).is_none());
    let boxed = Box::<OwnedRelations>::extract_from_values(&row).unwrap();
    let mut values = BTreeMap::new();
    boxed.inject_into_values(&mut values);
    assert_eq!(values["payload"], Value::from("relation"));
    println!("Original non-Clone Entity/Audited/boxed contracts passed");
}
