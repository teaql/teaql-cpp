use std::io::{self,BufRead};
use serde_json::{Value as J,json};
use teaql_runtime::{CheckResult,CheckRule,ObjectLocation,JsonFieldNamingProfile,RuntimeError,DataServiceError};
use teaql_core::{EntityError,DataType};
use teaql_sql::SqlCompileError;
#[path="../fixture_value.rs"]mod fixture_value;
fn check(input:&J)->CheckResult {
    let mut location=ObjectLocation::root();
    for part in input["path"].as_array().unwrap(){location=if let Some(name)=part.as_str(){location.member(name)}else{location.element(part.as_u64().unwrap() as usize)}}
    let rule=match input["rule"].as_u64().unwrap(){0=>CheckRule::Required,1=>CheckRule::InvalidType,2=>CheckRule::Min,3=>CheckRule::Max,4=>CheckRule::MinStringLength,5=>CheckRule::MaxStringLength,6=>CheckRule::ContextRootMissing,_=>CheckRule::ContextRootMismatch};
    let mut result=match input["constructor"].as_str().unwrap_or(""){
        "required"=>CheckResult::required(location),
        "min"=>CheckResult::min(location,fixture_value::value(&input["system"]),fixture_value::value(&input["input"])),
        "max"=>CheckResult::max(location,fixture_value::value(&input["system"]),fixture_value::value(&input["input"])),
        "min_str"=>CheckResult::min_str(location,input["length"].as_u64().unwrap(),fixture_value::value(&input["input"])),
        "max_str"=>CheckResult::max_str(location,input["length"].as_u64().unwrap(),fixture_value::value(&input["input"])),
        _=>CheckResult::new(rule,location),
    };
    if let Some(v)=input.get("input"){result=result.with_input_value(fixture_value::value(v));}
    if let Some(v)=input.get("system"){result=result.with_system_value(fixture_value::value(v));}
    if let Some(v)=input.get("message"){result=result.with_message(v.as_str().unwrap());}
    if let Some(v)=input.get("entity"){result=result.with_entity_type(v.as_str().unwrap());}
    if let Some(v)=input.get("source"){result=result.with_source_instance_path(v.as_str().unwrap());}result
}
fn error(input:&J)->RuntimeError {
    let text=input["text"].as_str().unwrap_or("").to_owned();let other=input["other"].as_str().unwrap_or("").to_owned();
    match input["kind"].as_u64().unwrap(){
        0=>RuntimeError::MissingEntity(text),1=>RuntimeError::SqlCompile(match input["sql"].as_u64().unwrap(){0=>SqlCompileError::UnknownEntity(text),1=>SqlCompileError::UnknownField(text),2=>SqlCompileError::EmptyInList,3=>SqlCompileError::MissingIdProperty(text),4=>SqlCompileError::MissingVersionProperty(text),5=>SqlCompileError::EmptyMutation(text),6=>SqlCompileError::InvalidRecoverVersion(-9),7=>SqlCompileError::UnsupportedSchemaType(DataType::Text),8=>SqlCompileError::InvalidSchemaShape{property:text,reason:other},9=>SqlCompileError::SchemaIdentifierTooLong{object:text,identifier:other,actual_bytes:99,max_bytes:64},10=>SqlCompileError::InvalidFunctionArguments(text),_=>SqlCompileError::InvalidSubQueryOperator(text)}),
        2=>RuntimeError::Behavior(text),3=>RuntimeError::Event(text),4=>RuntimeError::Policy(text),5=>RuntimeError::Check(input["checks"].as_array().unwrap().iter().map(check).collect()),6=>RuntimeError::Graph(text),7=>RuntimeError::IdGeneration(text),8=>RuntimeError::Language(text),9=>RuntimeError::UnsupportedLocale(text),10=>RuntimeError::Schema(text),11=>RuntimeError::Transaction(text),12=>RuntimeError::MissingRelation{entity:text,relation:other},_=>RuntimeError::OptimisticLockConflict{entity:text,id:other},
    }
}
fn evaluate(input:J)->J {
    match input["op"].as_str().unwrap(){
        "check"=>{let profile=match JsonFieldNamingProfile::from_model_value(input["profile"].as_str().unwrap()){Ok(v)=>v,Err(e)=>return json!({"error":e})};let result=check(&input);json!({"root":result.location.is_root(),"level":result.location.level(),"model":result.location.model_path(),"native":result.location.native_path(),"display":result.location.to_string(),"defaultInstance":result.location.instance_path(),"instance":result.location.instance_path_with(profile),"message":result.to_string(),"inputPresent":result.input_value.is_some(),"systemPresent":result.system_value.is_some(),"wire":result.to_wire(profile)})},
        "error"=>{let e=error(&input);let text=e.to_string();let checks=if let RuntimeError::Check(ref results)=e {json!(results.iter().map(|c|c.to_wire(JsonFieldNamingProfile::CamelCase)).collect::<Vec<_>>())}else{J::Null};json!({"display":text,"wrapped":DataServiceError::<String>::Runtime(e).to_string(),"checks":checks})},
        "entity_error"=>json!({"display":DataServiceError::<String>::Entity(EntityError::new(input["entity"].as_str().unwrap(),input["text"].as_str().unwrap())).to_string()}),
        "executor_error"=>json!({"display":DataServiceError::Executor(input["text"].as_str().unwrap().to_owned()).to_string()}),
        _=>panic!("unknown operation"),
    }
}
fn main(){for line in io::stdin().lock().lines(){println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap()));}}
