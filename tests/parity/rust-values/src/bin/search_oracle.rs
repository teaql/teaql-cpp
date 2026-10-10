use std::io::{self,BufRead};
use serde_json::{json,Value};
use teaql_core::dynamic_search::*;
fn evaluate(input:Value)->Value {
    if let Some(model)=input["field_contract"].as_str() {
        let field=input["field"].as_str().unwrap();
        macro_rules! observed {
            ($before:expr,$after:expr)=>{json!({"equal":$before==$after,
                "before":format!("{:?}",$before),"after":format!("{:?}",$after)})};
        }
        match model {
            "warning"=> {
                let before=DynamicSearchWarning{code:"CODE",entity:"Entity".into(),clause:"FILTER",field_path:"field".into()};
                let mut after=before.clone();
                match field {
                    "same"=>(),"code"=>after.code="OTHER","entity"=>after.entity="Changed".into(),
                    "clause"=>after.clause="ORDER_BY","field_path"=>after.field_path="changed".into(),
                    _=>panic!("Unknown warning field"),
                }
                let mut result=observed!(before,after);
                result["before_json"]=json!(before);result["after_json"]=json!(after);
                return result;
            },
            "filter"=> {
                let before=DynamicSearchFilter{field_path:"field".into(),operator:"$eq".into(),value:json!("value")};
                let mut after=before.clone();
                match field {
                    "same"=>(),"field_path"=>after.field_path="changed".into(),
                    "operator"=>after.operator="$ne".into(),"value"=>after.value=json!(false),
                    _=>panic!("Unknown filter field"),
                }
                return observed!(before,after);
            },
            "order"=> {
                let before=DynamicSearchOrder{field_path:"field".into(),descending:false};
                let mut after=before.clone();
                match field {
                    "same"=>(),"field_path"=>after.field_path="changed".into(),"descending"=>after.descending=true,
                    _=>panic!("Unknown order field"),
                }
                return observed!(before,after);
            },
            "error"=> {
                let before=DynamicSearchError("error");
                let after=DynamicSearchError(if field=="message" {"changed"} else {"error"});
                assert!(field=="same"||field=="message");
                return observed!(before,after);
            },
            _=>panic!("Unknown search model"),
        }
    }
    if let Some(index)=input["error_contract"].as_u64() {
        let messages=["Invalid trusted search setup","Invalid value for known search field",
            "binding failure","callback failure"];
        let error=DynamicSearchError(messages[index as usize]);
        return json!({"display":error.to_string(),"debug":format!("{error:?}"),
            "equal_clone":error==error.clone(),"source_none":std::error::Error::source(&error).is_none()});
    }
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
