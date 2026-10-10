use std::io::{self,BufRead};
use serde_json::{Value as J,json};
use teaql_runtime::{UserContext,ContextError};
#[path="../fixture_value.rs"]mod fixture_value;
fn evaluate(input:J)->J{let mut context=UserContext::new();let mut outputs=vec![];
    for action in input["actions"].as_array().unwrap(){let op=action["op"].as_str().unwrap();let name=action["name"].as_str().unwrap_or("");let kind=action["type"].as_str().unwrap_or("");outputs.push(match op{
        "typed_insert"|"named_insert"=>{if kind=="text"{let value=action["value"].as_str().unwrap().to_owned();if op=="typed_insert"{context.insert_resource(value)}else{context.insert_named_resource(name,value)}}else{let value=action["value"].as_i64().unwrap();if op=="typed_insert"{context.insert_resource(value)}else{context.insert_named_resource(name,value)}}json!({"ok":true})},
        "typed_get"|"named_get"=>if kind=="text"{json!({"value":if op=="typed_get"{context.get_resource::<String>()}else{context.get_named_resource::<String>(name)}})}else{json!({"value":if op=="typed_get"{context.get_resource::<i64>()}else{context.get_named_resource::<i64>(name)}})},
        "named_require"=>if kind=="text"{match context.require_named_resource::<String>(name){Ok(value)=>json!({"value":value}),Err(e)=>json!({"error":e.to_string()})}}else{match context.require_named_resource::<i64>(name){Ok(value)=>json!({"value":value}),Err(e)=>json!({"error":e.to_string()})}},
        "put_local"=>{context.put_local(name,fixture_value::value(&action["value"]));json!({"ok":true})},
        "local"=>{let value=context.local(name);json!({"present":value.is_some(),"value":value.map(|v|v.to_json_value())})},
        "remove_local"=>{let value=context.remove_local(name);json!({"present":value.is_some(),"value":value.map(|v|v.to_json_value())})},
        "error"=>{let error=match kind{"typed"=>ContextError::MissingTypedResource("fixture::Resource"),"named"=>ContextError::MissingResource(name.into()),_=>ContextError::MissingEntityDataService(name.into())};json!({"error":error.to_string()})},
        _=>panic!("unknown fixture operation"),
    });}json!(outputs)
}
fn main(){for line in io::stdin().lock().lines(){println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap()));}}
