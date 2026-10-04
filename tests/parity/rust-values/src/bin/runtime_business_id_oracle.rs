use std::io::{self,BufRead};
use serde_json::{Value as J,json};
use teaql_core::business_id::*;
use teaql_runtime::{BusinessIdEncodingKey,BusinessIdPermutationV1,DailySequenceBusinessIdProfile,PermutedDailyBusinessIdProfile,StaticBusinessIdKeyProvider,BusinessIdKeyProvider,UserContext};
fn err(e:BusinessIdError)->J{json!({"code":format!("{:?}",e.code),"message":e.message,"display":e.to_string()})}
fn scope(j:&J)->BusinessIdScope{let a=j.as_array().unwrap();BusinessIdScope{domain_root_key:a[0].as_str().unwrap().into(),aggregate_type:a[1].as_str().unwrap().into(),namespace:a[2].as_str().unwrap().into(),period_key:a[3].as_str().unwrap().into()}}
fn definition(j:&J)->BusinessIdDefinition{BusinessIdDefinition{field_name:"business_id".into(),profile:j["profile"].as_str().unwrap().into(),prefix:j["prefix"].as_str().unwrap().into(),namespace:j["namespace"].as_str().unwrap().into(),separator:j["separator"].as_str().unwrap().into(),split_by_date:j["split"].as_bool().unwrap(),preserve_digits:j["digits"].as_u64().unwrap() as u8}}
fn plan(j:&J)->BusinessIdPlan{BusinessIdPlan{scope:scope(&j["scope"]),prefix:j["prefix"].as_str().unwrap().into(),date_text:j["date"].as_str().map(str::to_owned),preserve_digits:j["digits"].as_u64().unwrap() as u8,separator:j["separator"].as_str().unwrap().into(),initial_sequence:j["initial"].as_str().unwrap().parse().unwrap(),max_sequence:j["maximum"].as_str().unwrap().parse().unwrap()}}
fn evaluate(j:J)->J{
    let bytes=j["key"].as_array().unwrap().iter().map(|x|x.as_u64().unwrap() as u8).collect::<Vec<_>>().try_into().unwrap();let key=match BusinessIdEncodingKey::new(j["version"].as_u64().unwrap() as u32,bytes){Ok(k)=>k,Err(e)=>return err(e)};
    let op=j["op"].as_str().unwrap();if op=="key"{let context=UserContext::new();let d=BusinessIdDefinition::daily_sequence("id","P","ns");let provided=StaticBusinessIdKeyProvider::new(key.clone()).current_key(&context,&d).unwrap();return json!({"version":provided.version(),"equal":provided==key});}
    if op=="encode"{return match BusinessIdPermutationV1::encode(j["sequence"].as_str().unwrap().parse().unwrap(),&scope(&j["scope"]),&key){Ok(v)=>json!({"ok":v}),Err(e)=>err(e)}}
    let p:Box<dyn BusinessIdProfile>=if j["implementation"]=="legacy"{Box::new(DailySequenceBusinessIdProfile)}else{Box::new(PermutedDailyBusinessIdProfile::new(key))};
    match op{
        "plan"=>{let d=definition(&j["definition"]);let date=chrono::NaiveDate::parse_from_str(j["date"].as_str().unwrap(),"%Y-%m-%d").unwrap();let request=BusinessIdGenerationRequest{definition:&d,domain_root_key:j["root"].as_str().unwrap(),aggregate_type:j["aggregate"].as_str().unwrap(),business_date:date};match p.plan(&request){Ok(v)=>json!({"scope":[v.scope.domain_root_key,v.scope.aggregate_type,v.scope.namespace,v.scope.period_key],"prefix":v.prefix,"date":v.date_text,"digits":v.preserve_digits,"separator":v.separator,"initial":v.initial_sequence.to_string(),"maximum":v.max_sequence.to_string()}),Err(e)=>err(e)}},
        "format"=>{let plan=plan(&j["plan"]);let allocation=BusinessIdAllocation{scope:scope(&j["allocation_scope"]),sequence:j["sequence"].as_str().unwrap().parse().unwrap()};match p.format(&plan,&allocation){Ok(v)=>json!({"ok":v.0}),Err(e)=>err(e)}},
        "validate"=>match p.validate(&definition(&j["definition"]),j["value"].as_str().unwrap()){Ok(v)=>json!({"ok":v.0}),Err(e)=>err(e)},
        _=>panic!("unknown op"),
    }
}
fn main(){for line in io::stdin().lock().lines(){println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap()));}}
