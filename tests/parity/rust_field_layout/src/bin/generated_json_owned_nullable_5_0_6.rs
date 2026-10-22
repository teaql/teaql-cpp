//! Generated nullable children with repeated owned parent projections.
use cpp_member_aliases_core::{Platform, Record};
use serde_json::{json, Value as J};
use std::io::{self,BufRead};
use teaql_core::{Entity,Value};
use teaql_runtime::{LedgerEntity,EntityKey};
fn run() -> J {
    let context=cpp_member_aliases_core::module().into_context();
    let input=json!({"id":1,"version":7,"name":"parent","record_list":[
        {"id":1,"version":7,"type_field":"first","platform_id":1,"platform":{"id":1,"version":7,"name":"parent"}},
        {"id":2,"version":7,"type_field":null,"platform_id":1}]});
    let root=context.decode_json_entity::<Platform>(&input).unwrap();
    let other=context.decode_json_entity::<Platform>(&input).unwrap();
    let state=root.entity_runtime_state().unwrap();
    let list=state.resolve_relation_list::<Record>("Platform",1,"record_list").unwrap();
    let child_state=list[0].entity_runtime_state().unwrap();
    let parent=child_state.resolve_relation_option::<Platform>("Record",1,"platform").unwrap().as_ref().unwrap();
    let same=std::ptr::eq(parent,child_state.resolve_entity::<Platform>(1).unwrap());
    let mut output=json!({"json":root.clone().into_json(),"child_values":list.iter().map(|r|r.type_field()).collect::<Vec<_>>(),
        "child_scalar_loaded":list.iter().map(|r|r.is_field_loaded("type_field")).collect::<Vec<_>>(),
        "child_parent_loaded":list.iter().map(|r|r.is_field_loaded("platform")).collect::<Vec<_>>(),
        "children_clean":list.iter().all(|r|r.dirty_fields().is_none()),"root_clean":root.dirty_fields().is_none(),
        "parent_name":parent.name(),"parent_reverse_loaded":parent.is_field_loaded("record_list"),"same_address":same});
    state.set(EntityKey::new("Platform",1),"name",Value::Text("one root".into()));
    output["root_dirty"]=json!(root.dirty_fields());output["other_dirty"]=json!(other.dirty_fields());output
}
fn main(){for line in io::stdin().lock().lines(){let _:J=serde_json::from_str(&line.unwrap()).unwrap();println!("{}",run());}}
