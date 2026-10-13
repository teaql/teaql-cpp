use std::collections::{BTreeMap, BTreeSet};
use std::io::{self, BufRead};
use serde_json::{json, Value as Json};
use teaql_core::{BaseEntity, BaseEntityData, CompactRow, Entity, EntityDescriptor,
    EntityError, IdentifiableEntity, MutationValues, TeaqlEntity, Value, VersionedEntity};
#[path = "../../../rust_values/src/fixture_value.rs"]
mod fixture_value;

#[derive(Clone)]
struct Legacy(BaseEntityData);
impl TeaqlEntity for Legacy {
    const ENTITY_NAME: &'static str = "Legacy";
    fn entity_descriptor() -> EntityDescriptor { EntityDescriptor::new(Self::ENTITY_NAME) }
}
impl Entity for Legacy {
    fn from_compact_row(row: CompactRow) -> Result<Self, EntityError> {
        BaseEntityData::from_values_map(&row.into_map()).map(Self)
    }
    fn into_values(self) -> MutationValues { self.0.to_values_map().into() }
}
impl BaseEntity for Legacy {
    fn base(&self) -> &BaseEntityData { &self.0 }
    fn base_mut(&mut self) -> &mut BaseEntityData { &mut self.0 }
}
impl IdentifiableEntity for Legacy { fn id_value(&self) -> Value { Value::U64(self.id()) } }
impl VersionedEntity for Legacy { fn version(&self) -> i64 { self.version_value() } }

fn record(values: &BTreeMap<String, Value>) -> Json {
    Json::Object(values.iter().map(|(k,v)| (k.clone(), json!(format!("{v:?}")))).collect())
}
fn defaults(entity: &Legacy) -> Json {
    json!({"loaded":entity.is_field_loaded("unknown"),"dirty_none":entity.dirty_fields().is_none(),
        "snapshot_none":entity.original_values().is_none(),"new":entity.is_new(),
        "delete":entity.is_marked_as_delete(),"comment":entity.get_comment()})
}
fn state(entity: &Legacy, key: &str) -> Json {
    json!({"id":entity.id().to_string(),"version":entity.version_value().to_string(),
        "id_value":format!("{:?}",entity.id_value()),"versioned":entity.version().to_string(),
        "dynamic":record(&entity.base().dynamic),"values":record(&entity.base().to_values_map()),
        "value":entity.dynamic(key).map(|v|format!("{v:?}")),
        "i64":entity.dynamic_i64(key).map(|v|v.to_string()),
        "u64":entity.dynamic_u64(key).map(|v|v.to_string()),
        "decimal":entity.dynamic_decimal(key).map(|v|v.to_string()),
        "f64":entity.dynamic_f64(key).map(|v|format!("{:?}",Value::F64(v))),
        "text":entity.dynamic_text(key),"bool":entity.dynamic_bool(key)})
}
fn evaluate(input: Json) -> Json {
    if input["kind"] == "error_source" {
        let error = EntityError::new(input["entity"].as_str().unwrap(), input["message"].as_str().unwrap());
        return json!({"source_none":std::error::Error::source(&error).is_none(),
            "debug":format!("{error:?}"),"display":error.to_string(),"clone_equal":error == error.clone()});
    }
    let mut entity = Legacy(BaseEntityData::new().with_id(input["id"].as_str().unwrap().parse().unwrap())
        .with_version(input["version"].as_str().unwrap().parse().unwrap()));
    let initial = defaults(&entity);
    entity.mark_as_new(); entity.set_comment("ignored".into());
    entity.set_checker_loaded_fields(BTreeSet::from(["id".into()]));
    entity.set_checker_dirty_fields(BTreeSet::from(["payload".into()]), &MutationValues::new());
    entity.on_loaded(&());
    let after_defaults = defaults(&entity);
    let mut steps = Vec::new();
    for op in input["ops"].as_array().unwrap() {
        let key = op["key"].as_str().unwrap_or("");
        let mut old = None;
        match op["kind"].as_str().unwrap() {
            "with" => entity.0 = std::mem::take(&mut entity.0).with_dynamic(key, fixture_value::value(&op["value"])),
            "put" => old = entity.put_dynamic(key, fixture_value::value(&op["value"])),
            "remove" => old = entity.base_mut().remove_dynamic(key),
            "id" => entity.set_id(op["value"].as_str().unwrap().parse().unwrap()),
            "version" => entity.set_version(op["value"].as_str().unwrap().parse().unwrap()),
            "get" => {},
            _ => unreachable!(),
        }
        steps.push(json!({"old":old.map(|v|format!("{v:?}")),"state":state(&entity,key)}));
    }
    let json = entity.clone().into_json();
    let loaded = Legacy::from_compact_row_with_context(CompactRow::from_map(entity.0.to_values_map()), &());
    let load = match loaded {
        Ok(e) => json!({"state":state(&e,"payload"),"defaults":defaults(&e)}),
        Err(e) => json!({"entity":e.entity,"message":e.message}),
    };
    json!({"initial_defaults":initial,"after_defaults":after_defaults,"steps":steps,"json":json,"load":load})
}
fn main() {
    for line in io::stdin().lock().lines() {
        println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
