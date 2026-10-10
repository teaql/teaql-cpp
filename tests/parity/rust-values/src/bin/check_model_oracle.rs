//! Actual original checker model derives, path methods and wire serialization.
use serde_json::{json,Value as J};
use std::io::{self,BufRead};
use teaql_runtime::*;
#[path="../fixture_value.rs"]mod fixture_value;
fn formatted<T:std::fmt::Debug>(v:&T)->J {json!({"compact":format!("{v:?}"),"pretty":format!("{v:#?}"),"lower_hex":format!("{v:x?}"),"upper_hex":format!("{v:X?}"),"pretty_lower_hex":format!("{v:#x?}"),"pretty_upper_hex":format!("{v:#X?}"),"width_precision":format!("{v:>40.2?}"),"fancy":format!("{v:🧊^+18.3?}"),"zero_pad":format!("{v:+012?}")})}
fn check(input:&J)->CheckResult {
 let mut location=ObjectLocation::root();
 for part in input["path"].as_array().unwrap(){location=if let Some(name)=part.as_str(){location.member(name)}else{location.element(part.as_u64().unwrap() as usize)}}
 let rule=match input["rule"].as_u64().unwrap(){0=>CheckRule::Required,1=>CheckRule::InvalidType,2=>CheckRule::Min,3=>CheckRule::Max,4=>CheckRule::MinStringLength,5=>CheckRule::MaxStringLength,6=>CheckRule::ContextRootMissing,_=>CheckRule::ContextRootMismatch};
 let mut result=CheckResult::new(rule,location);
 if let Some(v)=input.get("input"){result=result.with_input_value(fixture_value::value(v));}
 if let Some(v)=input.get("system"){result=result.with_system_value(fixture_value::value(v));}
 if let Some(v)=input.get("message"){result=result.with_message(v.as_str().unwrap());}
 if let Some(v)=input.get("entity"){result=result.with_entity_type(v.as_str().unwrap());}
 if let Some(v)=input.get("source"){result=result.with_source_instance_path(v.as_str().unwrap());}result
}
fn evaluate(input:J)->J {
 let profile=match JsonFieldNamingProfile::from_model_value(input["profile"].as_str().unwrap()){Ok(v)=>v,Err(error)=>return json!({"error":error})};
 let result=check(&input);let mut other=result.clone();other.message=Some("changed".into());let wire=result.to_wire(profile);let mut other_wire=wire.clone();other_wire.instance_path.push('!');
 json!({"rule":formatted(&result.rule),"profile":formatted(&profile),"default_profile":formatted(&JsonFieldNamingProfile::default()),"status_create":formatted(&CheckObjectStatus::Create),"status_update":formatted(&CheckObjectStatus::Update),"status_unknown":formatted(&CheckObjectStatus::Unknown),"location":formatted(&result.location),"location_clone":formatted(&result.location.clone()),"model":result.location.model_path(),"native":result.location.native_path(),"instance":result.location.instance_path_with(profile),"display":result.location.to_string(),"rule_id":result.rule.wire_id(),"check":formatted(&result),"check_clone":formatted(&result.clone()),"check_clone_equal":result==result.clone(),"check_changed_equal":result==other,"check_display":result.to_string(),"wire":formatted(&wire),"wire_clone":formatted(&wire.clone()),"wire_clone_equal":wire==wire.clone(),"wire_changed_equal":wire==other_wire,"json":serde_json::to_value(&wire).unwrap()})
}
fn main(){for line in io::stdin().lock().lines(){println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap()));}}
