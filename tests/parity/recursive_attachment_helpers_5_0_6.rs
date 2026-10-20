use cpp_relations_core::{Device, School};
use serde_json::{json, Value as Json};
use std::{collections::BTreeMap, io::{self, BufRead}};
use teaql_core::{CompactRow, Entity, Value, eval::{EvalResult, LoadState}};
use teaql_runtime::{EntityKey, EntityRuntimeState, LedgerEntity};

fn state(root:&EntityRuntimeState, keys:&[EntityKey]) -> Json {
    json!(keys.iter().map(|key|json!({"entity":key.entity,"id":key.id.to_json_value(),
        "name":root.get(key,"name").map(|v|v.to_json_value()),"new":root.is_new(key),
        "deleted":root.is_marked_as_delete(key),"version":root.get_original_version(key),
        "comment":root.get_entity_comment(key),"fields":root.changed_field_names(key)})).collect::<Vec<_>>())
}
fn eval_name(result:EvalResult<String>) -> Json {
    match result {
        EvalResult::Value(value)=>json!({"state":"Value","value":value}),
        EvalResult::Null=>json!({"state":"Null"}),
        EvalResult::NotLoaded{failed_node,attempted_path}=>json!({"state":"NotLoaded","node":failed_node,"path":attempted_path}),
    }
}
fn observe(input:&Json) -> Json {
    let mut row=BTreeMap::from([("id".into(),Value::U64(7)),("name".into(),Value::Text("parent".into())),
        ("version".into(),Value::I64(3)),("platform_id".into(),Value::U64(9))]);
    if input["embedded"] == true {
        row.insert("platform".into(),Value::Object(BTreeMap::from([
            ("id".into(),Value::U64(9)),("name".into(),Value::Text("child".into())),("version".into(),Value::I64(5))])));
    }
    macro_rules! run {
        ($model:ty) => {{
            let mut entity=<$model>::from_compact_row(CompactRow::from_map(row)).unwrap();
            if input["partial"] == true {entity.set_load_state(LoadState::Partial(["id".into(),"version".into()].into_iter().collect()));}
            let original_root=entity.entity_runtime_state().unwrap();
            entity.update_name(input["text"].as_str().unwrap());
            original_root.set_entity_comment(entity.entity_key(),"source parent");
            let parent_key=entity.entity_key();
            let child_key=EntityKey::new("Platform",Value::U64(9));
            if let Some(child)=entity.platform() {
                let child_root=child.entity_runtime_state().unwrap();
                child_root.set(child_key.clone(),"name",Value::Text("pending child".into()));
                child_root.set_entity_comment(child_key.clone(),"source child");
                if input["child_state"]=="new" {child_root.mark_as_new(child_key.clone());}
                if input["child_state"]=="delete" {child_root.mark_as_delete(child_key.clone());}
            }
            let target=EntityRuntimeState::default();
            if input["target"]=="name" {target.set(parent_key.clone(),"name",Value::Text("target name".into()));}
            if input["target"]=="version" {target.set_original_version(parent_key.clone(),99);}
            if input["target"]=="child" {target.set(child_key.clone(),"name",Value::Text("target child".into()));}
            let keys=[parent_key.clone(),child_key.clone()];
            let before=state(&target,&keys);
            let view=|entity:&$model|json!({"name":entity.name(),"name_eval":eval_name(entity.eval_name()),
                "child":entity.platform().map(|child|json!({"name":child.name(),"eval":eval_name(child.eval_name()),
                    "original":child.original_values().map(|v|v.get("name").map(|v|v.to_json_value()))})),
                "original":entity.original_values().map(|v|v.get("name").map(|v|v.to_json_value())),
                "loaded":[entity.is_loaded("name"),entity.is_loaded("platform_id"),entity.is_loaded("platform")]});
            let source=view(&entity);
            entity.attach_runtime_state_recursive(target.clone());
            let attached=view(&entity);let merged=state(&target,&keys);
            target.set(parent_key,"name",Value::Text("after parent".into()));
            target.set(child_key,"name",Value::Text("after child".into()));
            json!({"before":before,"source":source,"attached":attached,"merged":merged,
                "after":view(&entity),"old":state(&original_root,&keys),"target":state(&target,&keys)})
        }};
    }
    if input["model"]=="school" {run!(School)} else {run!(Device)}
}
fn main() {
    for line in io::stdin().lock().lines() {
        let input:Json=serde_json::from_str(&line.unwrap()).unwrap();println!("{}",observe(&input));
    }
}
