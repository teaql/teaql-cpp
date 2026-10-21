//! Actual generated models: snapshot identity, graph view identity and ledger isolation.
use cpp_dynamic_json_core::*;
use serde_json::{json, Value as J};
use std::{io::{self, BufRead}, sync::Arc};
use teaql_core::{Entity, Value};
use teaql_runtime::{EntityKey, LedgerEntity};

fn shared<A: Entity, B: Entity>(a: &A, b: &B) -> bool {
    Arc::ptr_eq(&a.loaded_state_snapshot().unwrap(), &b.loaded_state_snapshot().unwrap())
}
fn run(case: &str) -> J {
    let ctx = module().into_context();
    match case {
        "batch" => {
            let rows = ctx.decode_json_entities::<Profile>(&json!([
                {"id":1,"version":7,"note":null},
                {"version":7,"note":"private","id":1},
                {"id":2,"version":7}
            ])).unwrap();
            let sharing:Vec<Vec<bool>> = rows.iter().map(|a|rows.iter().map(|b|shared(a,b)).collect()).collect();
            let before:Vec<J> = rows.iter().map(|r|r.clone().into_json()).collect();
            rows[0].entity_runtime_state().unwrap().set(EntityKey::new("Profile",1),"note",Value::Text("one private mutation".into()));
            json!({"sharing":sharing,"before":before,"dirty":rows.iter().map(|r|r.dirty_fields()).collect::<Vec<_>>(),
                "loaded":rows.iter().map(|r|r.is_field_loaded("note")).collect::<Vec<_>>()})
        },
        "owned" => {
            let input=json!({"id":1,"version":7,"code":"parent","lines":[
                {"id":1,"version":7,"quantity":1,"order_id":1,"order":{"id":1,"version":7,"code":"parent"}},
                {"id":2,"version":7,"quantity":0,"order_id":1}]});
            let root=ctx.decode_json_entity::<Order>(&input).unwrap();
            let other=ctx.decode_json_entity::<Order>(&input).unwrap();
            let state=root.entity_runtime_state().unwrap();
            let children=state.resolve_relation_list::<Line>("Order",1,"lines").unwrap();
            let child_state=children[0].entity_runtime_state().unwrap();
            let parent=child_state.resolve_relation_option::<Order>("Line",1,"order").unwrap().as_ref().unwrap();
            let same=std::ptr::eq(parent,child_state.resolve_entity::<Order>(1).unwrap());
            let output=root.clone().into_json();
            let loaded:Vec<bool>=children.iter().map(|c|c.is_field_loaded("order")).collect();
            let children_clean=children.iter().all(|c|c.dirty_fields().is_none());
            let parent_lines=parent.is_field_loaded("lines");
            state.set(EntityKey::new("Order",1),"code",Value::Text("one root".into()));
            json!({"same_entity_address":same,"json":output,"child_order_loaded":loaded,"children_clean":children_clean,
                "parent_lines_loaded":parent_lines,"dirty":root.dirty_fields(),"other_dirty":other.dirty_fields()})
        },
        "independent" => {
            let roots=ctx.decode_json_entities::<Order>(&json!([
                {"id":1,"lines":[{"id":1,"quantity":1}]},
                {"id":1,"lines":[{"id":1,"quantity":2}]}])).unwrap();
            let mut values=Vec::new();let mut clean=Vec::new();
            for root in &roots {
                let state=root.entity_runtime_state().unwrap();
                let rows=state.resolve_relation_list::<Line>("Order",1,"lines").unwrap();
                values.push(rows[0].quantity());clean.push(rows[0].dirty_fields().is_none());
            }
            json!({"shared_snapshot":shared(&roots[0],&roots[1]),"quantities":values,"clean":clean})
        },
        "views" => {
            let input=json!({"id":1,"version":7,"code":"shared","lines":[
                {"id":1,"order_id":1,"order":{"id":1,"version":7,"code":"shared","lines":[]}},
                {"id":2,"order_id":1,"order":{"id":1,"version":7}}]});
            let (detached, result)={
                let root=ctx.decode_json_entity::<Order>(&input).unwrap();
                let state=root.entity_runtime_state().unwrap();
                let children=state.resolve_relation_list::<Line>("Order",1,"lines").unwrap();
                let first_state=children[0].entity_runtime_state().unwrap();
                let second_state=children[1].entity_runtime_state().unwrap();
                let first=first_state.resolve_entity::<Order>(1).unwrap();
                let second=second_state.resolve_entity::<Order>(1).unwrap();
                let first_root=first.entity_runtime_state().unwrap();let second_root=second.entity_runtime_state().unwrap();
                let result=json!({"root_first_snapshot":shared(&root,first),"first_second_snapshot":shared(first,second),
                    "loaded":[first.is_field_loaded("code"),second.is_field_loaded("code")],
                    "first_empty":first_root.resolve_relation_list::<Line>("Order",1,"lines").unwrap().is_empty(),
                    "second_missing":second_root.resolve_relation_list::<Line>("Order",1,"lines").is_none(),
                    "children_clean":children.iter().all(|c|c.dirty_fields().is_none()),"json":root.clone().into_json()});
                (first.clone(),result)
            };
            let mut result=result;result["detached"]=detached.into_json();result
        },
        _=>panic!("unknown fixture"),
    }
}
fn main(){for line in io::stdin().lock().lines(){let input:J=serde_json::from_str(&line.unwrap()).unwrap();println!("{}",run(input["case"].as_str().unwrap()));}}
