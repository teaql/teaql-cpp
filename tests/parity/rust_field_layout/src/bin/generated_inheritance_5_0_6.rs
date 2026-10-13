use cpp_inheritance_core::{School, Academy, ResearchAcademy};
use serde_json::{json, Value as Json};
use std::{collections::BTreeMap, io::{self,BufRead}};
use teaql_core::{CompactRow, Entity, TeaqlEntity, Value, eval::{LoadState, EvalResult}};

fn evaluation(result: EvalResult<String>) -> Json {
    match result {
        EvalResult::Value(v)=>json!({"state":"Value","value":v}),
        EvalResult::Null=>json!({"state":"Null"}),
        EvalResult::NotLoaded{failed_node,attempted_path}=>json!({"state":"NotLoaded","node":failed_node,"path":attempted_path}),
    }
}
fn observe(input: &Json) -> Json {
    macro_rules! run {
        ($model:ty) => {{
            let descriptor=<$model>::entity_descriptor();
            let mut row=BTreeMap::from([("id".into(),Value::U64(7)),("name".into(),Value::Text("original".into())),
                ("rank".into(),Value::I64(1)),("platform_id".into(),Value::U64(42)),("version".into(),Value::I64(3))]);
            if descriptor.properties.iter().any(|p|p.name=="campus_code") {row.insert("campus_code".into(),Value::Text("north".into()));}
            if descriptor.properties.iter().any(|p|p.name=="budget") {row.insert("budget".into(),Value::Decimal("12.50".parse().unwrap()));}
            let mut entity=<$model>::from_compact_row(CompactRow::from_map(row)).unwrap();
            if input["loaded"]==false {entity.set_load_state(LoadState::Partial(["id".into(),"version".into()].into_iter().collect()));}
            let copy=entity.clone();
            let before=evaluation(entity.eval_name());
            if input["update"]==true {entity.update_name(input["text"].as_str().unwrap());entity.update_rank(-17_i64);}
            let values=entity.clone().into_values();
            let native=values.iter().filter(|(name,_)|!name.starts_with('_')).map(|(name,value)|(name.clone(),value.to_json_value())).collect::<serde_json::Map<_,_>>();
            json!({"revision":<$model>::__TEAQL_FIELD_LAYOUT_REVISION,
                "indexes":<$model>::__TEAQL_FIXED_FIELD_INDEXES,"mappings":<$model>::__TEAQL_FIXED_FIELD_MAPPINGS,
                "descriptor":descriptor.properties.iter().map(|p|json!({"name":p.name,"column":p.column_name,"nullable":p.nullable,"id":p.is_id,"version":p.is_version})).collect::<Vec<_>>(),
                "table":descriptor.table_name,"masks":descriptor.audit_mask_fields,"mask_declared":descriptor.audit_mask_fields_declared,
                "before":before,"after":evaluation(entity.eval_name()),"copy":evaluation(copy.eval_name()),
                "name":entity.name(),"rank":entity.rank(),"platform":entity.platform_id(),"native":native})
        }};
    }
    match input["entity"].as_str().unwrap() {
        "school"=>run!(School),"academy"=>run!(Academy),_=>run!(ResearchAcademy)
    }
}
fn main() {
    for line in io::stdin().lock().lines() {
        let input:Json=serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}",observe(&input));
    }
}
