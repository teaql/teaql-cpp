//! Actual runtime Business ID/internal ID models, derives and state transitions.
use chrono::DateTime;
use serde_json::{json,Value};
use std::{io::{self,BufRead},panic::{catch_unwind,AssertUnwindSafe}};
use teaql_core::business_id::{BusinessIdAllocator,BusinessIdPlan,BusinessIdScope};
use teaql_runtime::*;
fn formatted<T:std::fmt::Debug>(value:&T)->Value {
    json!({"compact":format!("{value:?}"),"pretty":format!("{value:#?}"),
        "lower_hex":format!("{value:x?}"),"upper_hex":format!("{value:X?}"),
        "pretty_lower_hex":format!("{value:#x?}"),"pretty_upper_hex":format!("{value:#X?}"),
        "width_precision":format!("{value:>40.2?}"),"fancy":format!("{value:🧊^+18.3?}"),
        "zero_pad":format!("{value:+012?}")})
}
fn number(j:&Value,key:&str)->u64 {j[key].as_str().unwrap().parse().unwrap()}
fn panic_text(value:Box<dyn std::any::Any+Send>)->String {value.downcast_ref::<String>().cloned().or_else(||value.downcast_ref::<&str>().map(|v|v.to_string())).unwrap()}
fn evaluate(j:Value)->Value {
    match j["kind"].as_str().unwrap() {
        "clock"=>{
            let time=DateTime::from_timestamp(j["seconds"].as_str().unwrap().parse().unwrap(),j["nanos"].as_u64().unwrap() as u32).unwrap();
            let system=SystemBusinessClock::default();let fixed=FixedBusinessClock::new(time);let date=BusinessDate(time.date_naive());let profile=DailySequenceBusinessIdProfile::default();
            json!({"system":formatted(&system),"system_clone":formatted(&system.clone()),"fixed":formatted(&fixed),"fixed_clone":formatted(&fixed.clone()),"date":formatted(&date),"date_clone":formatted(&date.clone()),"date_equal":date==date.clone(),"profile":formatted(&profile),"profile_clone":formatted(&profile.clone()),"time":formatted(&fixed.business_time()),"business_date":formatted(&fixed.business_date())})
        },
        "key"=>{
            let first=BusinessIdEncodingKey::new(j["version"].as_u64().unwrap() as u32,[j["byte"].as_u64().unwrap() as u8;32]);
            match first {Ok(key)=>{let other=BusinessIdEncodingKey::new(j["other_version"].as_u64().unwrap() as u32,[j["other_byte"].as_u64().unwrap() as u8;32]).unwrap();json!({"version":key.version(),"clone_equal":key==key.clone(),"other_equal":key==other})},Err(error)=>json!({"error":error.to_string()})}
        },
        "counter"=>{
            let counter=if j["default"]==true {AtomicCounterIdGenerator::default()}else{AtomicCounterIdGenerator::new(number(&j,"start"))};let mut out=vec![json!({"debug":formatted(&counter)})];
            for action in j["steps"].as_array().unwrap(){
                let result=catch_unwind(AssertUnwindSafe(||if action["op"]=="floor" {counter.ensure_floor("Ignored",number(action,"value")).map(|_|json!(null))}else{counter.generate_id("Ignored").map(|v|json!(v.to_string()))}));
                let observed=match result{Ok(Ok(value))=>json!({"ok":value}),Ok(Err(error))=>json!({"error":error.to_string()}),Err(error)=>json!({"panic":panic_text(error)})};out.push(json!({"result":observed,"debug":formatted(&counter)}));
            }json!(out)
        },
        "snowflake"=>{
            match catch_unwind(AssertUnwindSafe(||SnowflakeIdGenerator::new(number(&j,"worker"),number(&j,"datacenter")))){Ok(value)=>json!({"debug":formatted(&value)}),Err(error)=>json!({"panic":panic_text(error)})}
        },
        "allocator"=>{
            let allocator=InMemoryBusinessIdAllocator::default();let mut out=vec![json!({"debug":formatted(&allocator)})];
            for action in j["steps"].as_array().unwrap(){
                let scope=BusinessIdScope{domain_root_key:"Root|%".into(),aggregate_type:"Item".into(),namespace:"sequence".into(),period_key:"20261008".into()};
                let plan=BusinessIdPlan{scope,prefix:"ORD".into(),date_text:None,preserve_digits:6,separator:"-".into(),initial_sequence:number(action,"initial"),max_sequence:number(action,"maximum")};
                let result=match allocator.allocate(&plan){Ok(value)=>json!({"sequence":value.sequence.to_string(),"scope":value.scope.canonical_key()}),Err(error)=>json!({"error":error.to_string()})};out.push(json!({"result":result,"debug":formatted(&allocator)}));
            }json!(out)
        },
        _=>panic!("unknown runtime ID fixture"),
    }
}
fn main(){std::panic::set_hook(Box::new(|_|{}));for line in io::stdin().lock().lines(){println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap()));}}
