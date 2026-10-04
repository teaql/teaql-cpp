use std::io::{self,BufRead};
use serde_json::{json,Value as Json};
use teaql_core::business_id::*;
fn scope(input:&Json)->BusinessIdScope {
    BusinessIdScope { domain_root_key:input[0].as_str().unwrap().into(),aggregate_type:input[1].as_str().unwrap().into(),namespace:input[2].as_str().unwrap().into(),period_key:input[3].as_str().unwrap().into() }
}
fn error(e:BusinessIdError)->Json { json!({"code":format!("{:?}",e.code),"message":e.message,"display":e.to_string()}) }
fn evaluate(input:Json)->Json {
    match input["kind"].as_str().unwrap() {
        "definition"=>{
            let mut definition=if input["legacy"]==true { BusinessIdDefinition::legacy_daily_sequence("business_id","订单","orders") } else { BusinessIdDefinition::daily_sequence("business_id","订单","orders") };
            if let Some(profile)=input["profile"].as_str() {definition.profile=profile.into();}
            if let Some(digits)=input["digits"].as_u64() {definition.preserve_digits=u8::try_from(digits).unwrap();}
            let range=match definition.max_sequence() {Ok(n)=>json!({"ok":n.to_string()}),Err(e)=>error(e)};
            json!({"field_name":definition.field_name,"profile":definition.profile,"prefix":definition.prefix,"namespace":definition.namespace,"split_by_date":definition.split_by_date,"preserve_digits":definition.preserve_digits,"separator":definition.separator,"range":range})
        },
        "scope"=>json!({"canonical":scope(&input["fields"]).canonical_key()}),
        "range"=>{
            let plan=BusinessIdPlan {scope:scope(&json!(["","","",""])),prefix:"".into(),date_text:None,preserve_digits:0,separator:"".into(),initial_sequence:input["initial"].as_str().unwrap().parse().unwrap(),max_sequence:input["maximum"].as_str().unwrap().parse().unwrap()};
            match plan.validate_allocation_range() {Ok(())=>json!({"ok":true}),Err(e)=>error(e)}
        },
        "error"=>{
            let codes=[BusinessIdErrorCode::InvalidDefinition,BusinessIdErrorCode::InvalidFormat,BusinessIdErrorCode::Immutable,BusinessIdErrorCode::Exhausted,BusinessIdErrorCode::Allocation,BusinessIdErrorCode::KeyNotFound,BusinessIdErrorCode::Encoding];
            error(BusinessIdError::new(codes[input["code"].as_u64().unwrap() as usize],input["message"].as_str().unwrap()))
        },
        _=>panic!("unknown business ID fixture"),
    }
}
fn main() { for line in io::stdin().lock().lines() { println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap())); } }
