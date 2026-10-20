use teaql_entity_constructor_helpers_506::*;
use serde_json::{json, Value as Json};
use std::io::{self, BufRead};
use teaql_core::{Entity, Value, eval::EvalResult};
use teaql_runtime::{EntityRuntimeState, LedgerEntity};
#[path="rust_values/src/fixture_value.rs"] mod fixture_value;

fn changed(value: Option<Value>) -> Json {
    json!({"present":value.is_some(),"value":value.map(|v|v.to_json_value())})
}
fn evaluated<T:Into<Value>>(value:EvalResult<T>) -> Json {
    match value {
        EvalResult::Value(value)=>json!({"state":"Value","value":value.into().to_json_value()}),
        EvalResult::Null=>json!({"state":"Null"}),
        EvalResult::NotLoaded{failed_node,attempted_path}=>json!({"state":"NotLoaded","node":failed_node,"path":attempted_path}),
    }
}
fn observe(input:&Json) -> Json {
    let root=EntityRuntimeState::default();
    macro_rules! run {
        ($create:ident,$model:ty,$getter:ident,$eval:ident,$changed:ident,$setter:ident) => {{
            let mut entity=$create(root.clone());
            let view=|entity:&$model| json!({"id":entity.id(),"version":entity.version(),
                "name":entity.name(),"active":entity.active(),"note":entity.note(),
                "created":entity.created_at().0,"updated":entity.updated_at().0,"tenant":entity.tenant_id(),
                "getter":Value::from(entity.$getter()).to_json_value(),
                "eval":evaluated(entity.$eval()),"changed":changed(entity.$changed()),
                "loaded":entity.is_loaded(input["field"].as_str().unwrap()),
                "original":entity.original_values().map(|v|v.get(input["field"].as_str().unwrap()).map(|v|v.to_json_value()))});
            if input["initial"] == "row" {
                entity=<$model>::from_compact_row(teaql_core::CompactRow::from_map(std::collections::BTreeMap::from([
                    ("id".into(),Value::U64(7)),("name".into(),Value::Text("stored".into())),("version".into(),Value::I64(3)),
                    ("active".into(),Value::Bool(true)),("note".into(),Value::Text("note".into())),("tenant_id".into(),Value::U64(42)),
                    ("created_at".into(),Value::Timestamp(teaql_core::time::Timestamp(111))),
                    ("updated_at".into(),Value::Timestamp(teaql_core::time::Timestamp(222))),
                    ("amount".into(),Value::Decimal("12.50".parse().unwrap())),("status".into(),Value::Text("open".into())),
                    ("reference".into(),Value::Text("ref".into()))]))).unwrap();
                entity.attach_runtime_state_recursive(root.clone());
            }
            let before=view(&entity);
            let copy=entity.clone();
            let value=fixture_value::value(&input["value"]);
            match input["op"].as_str().unwrap() {
                "update"=>{entity.$setter(value);},
                "overlay"=>root.set(entity.entity_key(),input["field"].as_str().unwrap(),value),
                "self"=>{entity.$setter(entity.$getter());},
                _=>{}
            }
            json!({"wall_millis":teaql_core::time::Timestamp::now().0,"before":before,"after":view(&entity),"copy":view(&copy),
                "pending":root.changed_field_names(&entity.entity_key())})
        }};
    }
    match (input["model"].as_str().unwrap(),input["field"].as_str().unwrap()) {
        ("invoice",_)=>run!(new_invoice,Invoice,amount,eval_amount,changed_amount,update_amount),
        ("special_invoice","amount")=>run!(new_special_invoice,SpecialInvoice,amount,eval_amount,changed_amount,update_amount),
        ("special_invoice",_)=>run!(new_special_invoice,SpecialInvoice,reference,eval_reference,changed_reference,update_reference),
        ("receipt",_)=>run!(new_receipt,Receipt,status,eval_status,changed_status,update_status),
        ("intermediate_record",_)=>run!(new_intermediate_record,IntermediateRecord,name,eval_name,changed_name,update_name),
        _=>run!(new_document,Document,name,eval_name,changed_name,update_name),
    }
}
fn main() {
    for line in io::stdin().lock().lines() {
        let input:Json=serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}",observe(&input));
    }
}
