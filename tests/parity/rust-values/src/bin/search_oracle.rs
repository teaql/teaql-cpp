use std::io::{self,BufRead};
use serde_json::{json,Value};
use teaql_core::dynamic_search::*;
fn evaluate(input:Value)->Value {
    let mut models=SearchModels::new();
    for (name,model) in input["models"].as_object().unwrap() {
        let mut entry=SearchModel::default();
        for (field,kind) in model["fields"].as_object().unwrap() {entry.fields.insert(field.clone(),kind.as_str().unwrap().into());}
        for (relation,target) in model["relations"].as_object().unwrap() {entry.relations.insert(relation.clone(),target.as_str().unwrap().into());}
        models.insert(name.clone(),entry);
    }
    let mut emitted=vec![];
    let output=normalize_dynamic_search(input["source"].as_str().unwrap(),input["entity"].as_str().unwrap(),&models,input["maxClauses"].as_u64().unwrap() as usize,Some(&mut |w|emitted.push(w.clone())));
    match output {
        Err(error)=>json!({"error":error.to_string(),"emitted":emitted}),
        Ok(result)=>json!({"result":{
            "filters":result.filters.into_iter().map(|f|json!({"fieldPath":f.field_path,"operator":f.operator,"value":f.value})).collect::<Vec<_>>(),
            "orders":result.orders.into_iter().map(|o|json!({"fieldPath":o.field_path,"descending":o.descending})).collect::<Vec<_>>(),
            "warnings":result.warnings},"emitted":emitted}),
    }
}
fn main(){for line in io::stdin().lock().lines(){let input:Value=serde_json::from_str(&line.unwrap()).unwrap();println!("{}",evaluate(input));}}
