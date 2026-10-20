use cpp_relations_core::{Device, Platform, School};
use serde_json::{json, Value as Json};
use std::{collections::BTreeMap, io::{self, BufRead}, panic::{catch_unwind, AssertUnwindSafe}};
use teaql_core::{CompactRow, Entity, SmartList, Value, eval::{EvalResult, LoadState}};
use teaql_runtime::{EntityGraphBuilder, LedgerEntity};

fn row(id: u64, name: Option<&str>) -> CompactRow {
    let mut fields = BTreeMap::from([("id".into(), Value::U64(id)), ("version".into(), Value::I64(3))]);
    if let Some(name) = name { fields.insert("name".into(), Value::Text(name.into())); }
    CompactRow::from_map(fields)
}

fn observe(input: &Json) -> Json {
    let mut owner = Platform::from_compact_row(row(9, Some("owner"))).unwrap();
    owner.set_load_state(LoadState::Partial(if input["selected"] == true {
        ["id", "version", "name", "school_list", "device_list"].into_iter().map(str::to_owned).collect()
    } else { ["id", "version", "name"].into_iter().map(str::to_owned).collect() }));
    let root = owner.entity_runtime_state().unwrap();
    let mut graph = EntityGraphBuilder::default();
    let stored_owner = if input["key"] == "owner" { "School" } else { "Platform" };
    let stored_id = if input["key"] == "id" { 10 } else { 9 };
    let field = input["field"].as_str().unwrap();
    if input["edge"] == "wrong" {
        graph.install_relation_list(stored_owner, stored_id, field, SmartList::new(vec![42_u64]));
    } else if input["edge"] == "list" {
        macro_rules! install {
            ($model:ty) => {{
                let mut list = SmartList::new((0..input["size"].as_u64().unwrap()).map(|index|
                    <$model>::from_compact_row(row(11+index, if input["child_loaded"] == true {
                        Some(input["name"].as_str().unwrap())
                    } else { None })).unwrap()).collect());
                list.is_loaded = input["list_loaded"] == true;
                graph.install_relation_list(stored_owner, stored_id, field, list);
            }};
        }
        if field == "school_list" { install!(School); } else { install!(Device); }
    }
    root.freeze_graph(graph).unwrap();
    let observe = || {
        macro_rules! read {
            ($getter:ident, $evaluation:ident, $model:ty) => {{
                let handle = owner.$getter();
                let value = |list: &SmartList<$model>| json!({"loaded":list.is_loaded,
                    "items":list.data.iter().map(|entity|json!({"id":entity.id(),"name":entity.name(),
                        "eval":match entity.eval_name() {
                            EvalResult::Value(value)=>json!({"state":"Value","value":value}),
                            EvalResult::Null=>json!({"state":"Null"}),
                            EvalResult::NotLoaded{failed_node,attempted_path}=>json!({"state":"NotLoaded","node":failed_node,"path":attempted_path}),
                        }})).collect::<Vec<_>>()});
                json!({"handle":format!("{:?}",handle.state()),"loaded":handle.is_loaded(),
                    "empty":handle.is_empty(),"value":handle.value().map(value),
                    "eval":match owner.$evaluation() {
                        EvalResult::Value(list)=>json!({"state":"Value","value":value(list)}),
                        EvalResult::Null=>json!({"state":"Null"}),
                        EvalResult::NotLoaded{failed_node,attempted_path}=>json!({"state":"NotLoaded","node":failed_node,"path":attempted_path}),
                    }})
            }};
        }
        if field == "school_list" { read!(school_list, eval_school_list, School) }
        else { read!(device_list, eval_device_list, Device) }
    };
    match catch_unwind(AssertUnwindSafe(observe)) {
        Ok(value)=>value,
        Err(error)=>json!({"error":error.downcast_ref::<String>().cloned()
            .or_else(||error.downcast_ref::<&str>().map(|s|s.to_string())).unwrap()}),
    }
}

fn main() {
    std::panic::set_hook(Box::new(|_| {}));
    for line in io::stdin().lock().lines() {
        let input: Json = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", observe(&input));
    }
}
