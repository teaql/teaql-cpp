//! Observe the unchanged registry's complete metadata-preserving list boundary.
use serde_json::{json, Value as J};
use std::{collections::BTreeMap, io::{self, BufRead}};
use teaql_core::{CompactRow, Entity, EntityDescriptor, EntityError, IdentifiableEntity,
                 MutationValues, SmartList, TeaqlEntity, Value};
use teaql_runtime::{EntityGraphBuilder, EntityRuntimeState, InMemoryEntityGraphDecoderRegistry};
#[path = "rust_values/src/fixture_value.rs"] mod fixture_value;

#[derive(Clone)]
struct Item<const K: usize>(CompactRow);
impl<const K: usize> TeaqlEntity for Item<K> {
    const ENTITY_NAME: &'static str = "Item";
    fn entity_descriptor() -> EntityDescriptor { EntityDescriptor::new("Item") }
}
impl<const K: usize> Entity for Item<K> {
    fn from_compact_row(row: CompactRow) -> Result<Self, EntityError> {
        if row.get("fail").and_then(Value::try_bool).unwrap_or(false) {
            return Err(EntityError::new("Item", "handwritten model decode failed"));
        }
        Ok(Self(row))
    }
    fn into_values(self) -> MutationValues { self.0.into_map().into() }
}
impl<const K: usize> IdentifiableEntity for Item<K> {
    fn id_value(&self) -> Value { self.0.get("id").cloned().unwrap_or(Value::Null) }
}
fn row(input: &J) -> CompactRow {
    CompactRow::from_map(input.as_object().unwrap().iter()
        .map(|(k,v)| (k.clone(), fixture_value::value(v))).collect())
}
fn metadata(input: &J) -> SmartList<CompactRow> {
    SmartList {
        data: input["rows"].as_array().unwrap().iter().map(row).collect(),
        total_count: input["total"].as_u64(),
        aggregations: input["aggregations"].as_object().unwrap().iter()
            .map(|(k,v)| (k.clone(), fixture_value::value(v))).collect(),
        summary: input["summary"].as_object().unwrap().iter()
            .map(|(k,v)| (k.clone(), fixture_value::value(v))).collect(),
        facets: input["facets"].as_object().unwrap().iter()
            .map(|(k,v)| (k.clone(), metadata(v))).collect(),
        is_loaded: input["loaded"].as_bool().unwrap(),
    }
}
fn fields(values: &BTreeMap<String,Value>) -> J {
    json!(values.iter().map(|(k,v)| (k.clone(),format!("{v:?}"))).collect::<BTreeMap<_,_>>())
}
trait RowAccess { fn row(&self) -> &CompactRow; }
impl<const K:usize> RowAccess for Item<K> { fn row(&self) -> &CompactRow { &self.0 } }
impl RowAccess for CompactRow { fn row(&self) -> &CompactRow { self } }
fn describe<T: RowAccess>(list: &SmartList<T>) -> J {
    json!({"rows":list.data.iter().map(|e|fields(&e.row().clone().into_map())).collect::<Vec<_>>(),
        "total":list.total_count, "loaded":list.is_loaded,
        "aggregations":fields(&list.aggregations), "summary":fields(&list.summary),
        "facets":list.facets.iter().map(|(k,v)| (k.clone(), describe(v)))
            .collect::<BTreeMap<_,_>>()})
}
fn evaluate(input: J) -> J {
    let root = EntityRuntimeState::default();
    let mut graph = EntityGraphBuilder::default();
    let mut registry = InMemoryEntityGraphDecoderRegistry::default();
    if input["seed"] == true {
        registry.register::<Item<0>>();
        registry.decode_compact_list("Item", vec![CompactRow::from_map(BTreeMap::from([
            ("id".into(),Value::U64(99)), ("label".into(),Value::Text("previous".into()))]))],
            &root, &mut graph, "Owner", 7, "items").unwrap();
    }
    registry = InMemoryEntityGraphDecoderRegistry::default();
    if input["registration"] != 0 { registry.register::<Item<0>>(); }
    let copied = registry.clone();
    registry.register::<Item<1>>();
    let registry = if input["registration"] == 2 { copied } else {
        let mut registry = InMemoryEntityGraphDecoderRegistry::default();
        if input["registration"] == 1 { registry.register::<Item<0>>(); }
        registry
    };
    let rows = metadata(&input["list"]);
    let outcome = if input["mode"] == "smart" {
        registry.decode_compact_smart_list("Item", rows, &root, &mut graph, "Owner", 7, "items")
    } else {
        registry.decode_compact_list("Item", rows.data, &root, &mut graph, "Owner", 7, "items")
    };
    let counts = [graph.entity_count(), graph.relation_list_count()];
    root.freeze_graph(graph).unwrap();
    json!({"error":outcome.err().map(|e|e.to_string()),"counts":counts,
        "list":root.resolve_relation_list::<Item<0>>("Owner",7,"items")
            .map(describe)})
}
fn main() {
    for line in io::stdin().lock().lines() {
        println!("{}", evaluate(serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
