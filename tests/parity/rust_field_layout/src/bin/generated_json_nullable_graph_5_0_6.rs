//! Actual generated nullable foreign keys and graph input states.
use cpp_member_aliases_core::{Holder, Record};
use serde_json::{json, Value as J};
use std::io::{self, BufRead};
use teaql_core::Entity;
use teaql_runtime::LedgerEntity;
fn run(input: &J) -> J {
    let context = cpp_member_aliases_core::module().into_context();
    if input["entity"] == "Holder" {
        match context.decode_json_entity::<Holder>(&input["value"]) {
            Err(error) => json!({"ok":false,"error":error.to_string()}),
            Ok(row) => {
                let state=row.entity_runtime_state().unwrap();
                let option=state.resolve_relation_option::<Record>("Holder",row.id(),"record");
                let relation=match option {None=>"not_loaded",Some(None)=>"null",Some(Some(_))=>"some"};
                let same=option.and_then(|o|o.as_ref()).map(|parent|std::ptr::eq(parent,state.resolve_entity::<Record>(parent.id()).unwrap()));
                json!({"ok":true,"json":row.clone().into_json(),"foreign_key":row.record_id(),
                    "foreign_key_loaded":row.is_field_loaded("record_id"),"relation_loaded":row.is_field_loaded("record"),
                    "relation":relation,"same_address":same,"clean":row.dirty_fields().is_none()})
            }
        }
    } else {
        match context.decode_json_entity::<Record>(&input["value"]) {
            Err(error) => json!({"ok":false,"error":error.to_string()}),
            Ok(row) => {
                let state=row.entity_runtime_state().unwrap();
                let children=state.resolve_relation_list::<Holder>("Record",row.id(),"holder_list");
                json!({"ok":true,"json":row.clone().into_json(),"relation_loaded":row.is_field_loaded("holder_list"),
                    "count":children.map(|c|c.len()),"clean":row.dirty_fields().is_none()})
            }
        }
    }
}
fn main() {for line in io::stdin().lock().lines(){let input:J=serde_json::from_str(&line.unwrap()).unwrap();println!("{}",run(&input));}}
