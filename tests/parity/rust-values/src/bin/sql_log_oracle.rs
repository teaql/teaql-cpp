use std::io::{self,BufRead};
use serde_json::{json,Value as Json};
use teaql_core::Value;
use teaql_data_service::{SqlLogContext,SqlIntentRedactions,SqlParameterLogPolicy as Policy,is_credential_log_name};
#[path="../fixture_value.rs"] mod fixture_value;
use fixture_value::value;
fn binding(input:&Json)->SqlIntentRedactions {
    let params:Vec<Value>=input["params"].as_array().unwrap().iter().map(value).collect();
    let mut source=SqlLogContext{generated_sql:input["generated"].as_bool().unwrap(),parameter_policies:input["policies"].as_array().unwrap().iter().map(|v|match v.as_u64().unwrap(){0=>Policy::Unknown,1=>Policy::Plain,2=>Policy::Masked,3=>Policy::Credential,_=>panic!()}).collect(),..Default::default()};
    if input.get("ancestor").is_some(){source.intent_redactions=binding(&input["ancestor"]);}
    SqlIntentRedactions::from_bindings(&source,&params,input["sql"].as_str().unwrap())
}
fn evaluate(input:Json)->Json {
    let mut collected=binding(&input);
    if let Some(extra)=input["extend"].as_array(){for item in extra{collected.extend(&binding(item));}}
    if let Some(ids)=input["ids"].as_array(){for id in ids{collected.capture_target_id(&value(id));}}
    let mut safe=Vec::new();let mut debug=Vec::new();collected.extend_secrets(false,&mut safe);collected.extend_secrets(true,&mut debug);
    let names:Vec<bool>=input["names"].as_array().unwrap().iter().map(|v|is_credential_log_name(v.as_str().unwrap())).collect();
    json!({"safe":safe,"debug":debug,"empty":collected.is_empty(),"description":format!("{collected:?}"),"names":names})
}
fn main(){for line in io::stdin().lock().lines(){println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap()));}}
