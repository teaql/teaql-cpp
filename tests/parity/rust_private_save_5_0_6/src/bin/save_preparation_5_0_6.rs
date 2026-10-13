//! Original extraction/preflight/ledger helpers via access-only archive bridges.
use serde_json::{Value as J, json};
use std::{
    collections::{BTreeMap, BTreeSet},
    io::{self, BufRead},
    sync::{Arc, Mutex},
};
use teaql_core::{
    CompactRow, DataType, Entity, EntityDescriptor, EntityError, EntitySnapshot, MutationValues,
    PropertyDescriptor, RelationDescriptor, TeaqlEntity, Value,
};
use teaql_runtime::{
    __teaql_cpp_cancelled, __teaql_cpp_extract, __teaql_cpp_hydrate, __teaql_cpp_locations,
    __teaql_cpp_merge, __teaql_cpp_preflight, __teaql_cpp_preserve, CheckResult, CheckResults,
    Checker, EntityKey, EntityRuntimeState, EntityValues, GraphNode, GraphOperation,
    InMemoryCheckerRegistry, ObjectLocation, RuntimeError, RuntimeModule, UserContext,
    graph_node_from_entity,
};
#[path = "../../../rust_values/src/fixture_value.rs"]
mod fixture_value;
fn record(input: &J) -> BTreeMap<String, Value> {
    input
        .as_object()
        .unwrap()
        .iter()
        .map(|(k, v)| (k.clone(), fixture_value::value(v)))
        .collect()
}
fn record_out(values: &BTreeMap<String, Value>) -> J {
    json!(
        values
            .iter()
            .map(|(k, v)| (k.clone(), format!("{v:?}")))
            .collect::<BTreeMap<_, _>>()
    )
}
fn fields(input: &J) -> Option<BTreeSet<String>> {
    input
        .as_array()
        .map(|v| v.iter().map(|s| s.as_str().unwrap().to_owned()).collect())
}
fn node(input: &J) -> GraphNode {
    let mut out = GraphNode::new(input["entity"].as_str().unwrap()).operation(
        [
            GraphOperation::Upsert,
            GraphOperation::Create,
            GraphOperation::Reference,
            GraphOperation::Remove,
        ][input["operation"].as_u64().unwrap() as usize],
    );
    out.values = record(&input["values"]).into();
    out.dirty_fields = fields(&input["dirty"]);
    out.original_values = input["old"]
        .as_object()
        .map(|_| record(&input["old"]).into());
    if let Some(comment) = input["comment"].as_str() {
        out.set_comment(comment);
    }
    for (name, children) in input["relations"].as_object().unwrap() {
        out.relations.insert(
            name.clone(),
            children.as_array().unwrap().iter().map(node).collect(),
        );
    }
    out
}
fn node_out(node: &GraphNode) -> J {
    json!({"entity":node.entity,"values":record_out(&node.values),"operation":format!("{:?}",node.operation),"dirty":node.dirty_fields,"old":node.original_values.as_ref().map(|v|record_out(v)),"comment":node.comment,"relations":node.relations.iter().map(|(k,v)|(k.clone(),v.iter().map(node_out).collect::<Vec<_>>())).collect::<BTreeMap<_,_>>()})
}
fn key(input: &J) -> EntityKey {
    EntityKey::new(
        input["entity"].as_str().unwrap(),
        fixture_value::value(&input["id"]),
    )
}
fn state_out(root: &EntityRuntimeState, input: &J) -> J {
    json!({"changes":root.current_change_set().changes().iter().map(|(k,v)|json!({"key":format!("{k:?}"),"values":record_out(v)})).collect::<Vec<_>>(),"new":root.new_keys().iter().map(|k|format!("{k:?}")).collect::<Vec<_>>(),"deleted":root.deleted_keys().iter().map(|k|format!("{k:?}")).collect::<Vec<_>>(),"versions":input["keys"].as_array().unwrap().iter().map(|j|{let k=key(j);json!({"key":format!("{k:?}"),"version":root.get_original_version(&k)})}).collect::<Vec<_>>()})
}
#[derive(Clone)]
struct Model {
    values: MutationValues,
    loaded: Option<BTreeSet<String>>,
    dirty: Option<BTreeSet<String>>,
    old: Option<EntitySnapshot>,
    comment: Option<String>,
    fresh: bool,
    removed: bool,
}
impl TeaqlEntity for Model {
    const ENTITY_NAME: &'static str = "Parent";
    fn entity_descriptor() -> EntityDescriptor {
        EntityDescriptor::new("Parent")
            .property(PropertyDescriptor::new("id", DataType::U64).id())
            .property(PropertyDescriptor::new("version", DataType::I64).version())
            .property(PropertyDescriptor::new("name", DataType::Text))
    }
}
impl Entity for Model {
    fn from_compact_row(row: CompactRow) -> Result<Self, EntityError> {
        Ok(Self {
            values: row.into_map().into(),
            loaded: None,
            dirty: None,
            old: None,
            comment: None,
            fresh: false,
            removed: false,
        })
    }
    fn into_values(self) -> MutationValues {
        self.values
    }
    fn is_field_loaded(&self, name: &str) -> bool {
        self.loaded.as_ref().is_none_or(|v| v.contains(name))
    }
    fn dirty_fields(&self) -> Option<BTreeSet<String>> {
        self.dirty.clone()
    }
    fn original_values(&self) -> Option<EntitySnapshot> {
        self.old.clone()
    }
    fn get_comment(&self) -> Option<String> {
        self.comment.clone()
    }
    fn is_new(&self) -> bool {
        self.fresh
    }
    fn is_marked_as_delete(&self) -> bool {
        self.removed
    }
}
struct TestChecker {
    name: String,
    action: String,
    calls: Arc<Mutex<Vec<String>>>,
}
impl Checker for TestChecker {
    fn entity(&self) -> &str {
        &self.name
    }
    fn check_and_fix(
        &self,
        _: &UserContext,
        values: &mut EntityValues,
        location: &ObjectLocation,
        results: &mut CheckResults,
    ) {
        self.calls.lock().unwrap().push(location.to_string());
        match self.action.as_str() {
            "fill" => {
                values.insert("name".into(), "fixed".into());
            }
            "set_id" => {
                values.insert("id".into(), Value::U64(77));
            }
            "erase" => {
                values.remove("name");
            }
            "error" => results.push(CheckResult::required(location.clone().member("name"))),
            _ => {}
        }
    }
}
fn evaluate(input: J) -> J {
    let mut module = RuntimeModule::new();
    let mut checkers = InMemoryCheckerRegistry::new();
    let mut callbacks = Vec::new();
    for info in input["descriptors"].as_array().unwrap() {
        let name = info["name"].as_str().unwrap();
        let mut descriptor = EntityDescriptor::new(name);
        for name in ["id", "version", "name", "parent_id"] {
            let mut p = PropertyDescriptor::new(name, DataType::Text);
            if name == "id" {
                p = p.id();
            }
            if name == "version" {
                p = p.version();
            }
            if info["required"].as_bool().unwrap_or(false) && name != "parent_id" {
                p = p.not_null();
            }
            descriptor = descriptor.property(p);
        }
        for relation in info["relations"].as_array().unwrap() {
            descriptor.relations.push(
                RelationDescriptor::new(
                    relation["name"].as_str().unwrap(),
                    relation["target"].as_str().unwrap(),
                )
                .local_key(relation["local"].as_str().unwrap())
                .foreign_key(relation["foreign"].as_str().unwrap()),
            );
        }
        module = module.descriptor(descriptor);
        if let Some(action) = info["checker"].as_str() {
            let calls = Arc::new(Mutex::new(Vec::new()));
            callbacks.push((name.to_owned(), calls.clone()));
            checkers.register(TestChecker {
                name: name.into(),
                action: action.into(),
                calls,
            });
        }
    }
    let mut context = UserContext::new();
    module.apply_to(&mut context);
    context.set_checker_registry(checkers);
    let root = EntityRuntimeState::default();
    for entry in input["seed"].as_array().unwrap() {
        let k = key(entry);
        for (f, v) in record(&entry["values"]) {
            root.set(k.clone(), f, v);
        }
        if entry["new"].as_bool().unwrap_or(false) {
            root.mark_as_new(k.clone());
        }
        if entry["delete"].as_bool().unwrap_or(false) {
            root.mark_as_delete(k.clone());
        }
        if let Some(version) = entry["version"].as_i64() {
            root.set_original_version(k, version);
        }
    }
    let op = input["op"].as_str().unwrap();
    let mut out = json!({});
    let mut graph = None;
    let mut status: Result<(), RuntimeError> = Ok(());
    if op == "extract" || op == "typed" {
        let values: MutationValues = record(&input["values"]).into();
        let result = if op == "extract" {
            __teaql_cpp_extract(&context, input["entity"].as_str().unwrap(), values)
        } else {
            graph_node_from_entity(
                &context,
                Model {
                    values,
                    loaded: fields(&input["loaded"]),
                    dirty: fields(&input["dirty"]),
                    old: input["old"]
                        .as_object()
                        .map(|_| record(&input["old"]).into()),
                    comment: input["comment"].as_str().map(str::to_owned),
                    fresh: input["fresh"].as_bool().unwrap_or(false),
                    removed: input["removed"].as_bool().unwrap_or(false),
                },
            )
        };
        match result {
            Ok(node) => graph = Some(node),
            Err(error) => status = Err(error),
        };
    } else {
        let mut n = node(&input["node"]);
        match op {
            "preflight" => {
                status = __teaql_cpp_preflight(
                    &context,
                    &mut n,
                    &ObjectLocation::root(),
                    if input["root"].as_bool().unwrap_or(false) {
                        Some(&root)
                    } else {
                        None
                    },
                )
            }
            "merge" => status = __teaql_cpp_merge(&root, &n),
            "hydrate" => {
                let mut visited = BTreeSet::new();
                if let Some(id) = n.id() {
                    visited.insert(EntityKey::new(n.entity.clone(), id.clone()));
                }
                status = __teaql_cpp_hydrate(&context, &root, &mut n, &mut visited);
                out["visited"] =
                    json!(visited.iter().map(|k| format!("{k:?}")).collect::<Vec<_>>());
            }
            "flags" => {
                let k = key(&input["key"]);
                status = __teaql_cpp_cancelled(&root, &k);
                if let Ok(descriptor) = context.require_entity(&n.entity) {
                    out["preserve"] = json!(__teaql_cpp_preserve(&root, &k, descriptor));
                }
            }
            _ => panic!("fixture operation"),
        }
        graph = Some(n);
    }
    out["error"] = json!(status.err().map(|v| v.to_string()));
    out["graph"] = graph.as_ref().map(node_out).unwrap_or(J::Null);
    out["state"] = state_out(&root, &input);
    out["calls"] = json!(
        callbacks
            .iter()
            .map(|(name, calls)| json!({"entity":name,"paths":*calls.lock().unwrap()}))
            .collect::<Vec<_>>()
    );
    out["locations"]=json!(graph.as_ref().map(|node|__teaql_cpp_locations(node).iter().map(|(k,path)|json!({"key":format!("{k:?}"),"model":path.model_path(),"instance":path.instance_path()})).collect::<Vec<_>>()).unwrap_or_default());
    out
}
fn main() {
    for line in io::stdin().lock().lines() {
        println!(
            "{}",
            evaluate(serde_json::from_str(&line.unwrap()).unwrap())
        );
    }
}
