use std::io::{self,BufRead};
use serde_json::{json,Value as Json};
use rust_decimal::{Decimal,RoundingStrategy};
fn model(value:Decimal)->Json {
    let unpacked=value.unpack();
    let text=std::panic::catch_unwind(||value.to_string());
    let mut result=json!({"text":text.as_ref().ok(),"scale":value.scale(),"negative":value.is_sign_negative(),"coefficient":[unpacked.lo,unpacked.mid,unpacked.hi]});
    if text.is_err() {result["text_panic"]=json!(true);}result
}
fn evaluate(input:Json)->Json {
    if input["kind"]=="codec" {
        let bytes:Vec<u8>=input["bytes"].as_array().unwrap().iter().map(|v|v.as_u64().unwrap() as u8).collect();
        let value=Decimal::deserialize(bytes.try_into().unwrap());return json!({"value":model(value),"bytes":value.serialize()});
    }
    let mut a:Decimal=input["a"].as_str().unwrap().parse().unwrap();
    if input["negative_zero"]==true {a.set_sign_negative(true);}
    if input["kind"]=="scale" {
        let scale=input["dp"].as_u64().unwrap() as u32;let mut changed=a;let error=changed.set_scale(scale).err().map(|e|e.to_string());
        let mut rescaled=a;rescaled.rescale(scale);
        let mut result=json!({"set":model(changed),"error":error,"rescale":model(rescaled),"trunc_scale":model(a.trunc_with_scale(scale)),"bytes":a.serialize()});
        match std::panic::catch_unwind(||rescaled.as_f64().to_bits()) {Ok(bits)=>result["f64_bits"]=json!(bits),Err(_)=>result["f64_panic"]=json!(true)};
        return result;
    }
    if input["kind"]=="binary" {
        let b:Decimal=input["b"].as_str().unwrap().parse().unwrap();
        return json!({"add":a.checked_add(b).map(model),"sub":a.checked_sub(b).map(model),"mul":a.checked_mul(b).map(model),"div":a.checked_div(b).map(model),"rem":a.checked_rem(b).map(model),
            "saturating_add":model(a.saturating_add(b)),"saturating_sub":model(a.saturating_sub(b)),"saturating_mul":model(a.saturating_mul(b)),"compare":match a.cmp(&b) {std::cmp::Ordering::Less=>-1,std::cmp::Ordering::Equal=>0,std::cmp::Ordering::Greater=>1}});
    }
    #[allow(deprecated)]
    let strategies=[RoundingStrategy::MidpointNearestEven,RoundingStrategy::MidpointAwayFromZero,RoundingStrategy::MidpointTowardZero,RoundingStrategy::ToZero,RoundingStrategy::AwayFromZero,RoundingStrategy::ToNegativeInfinity,RoundingStrategy::ToPositiveInfinity,RoundingStrategy::BankersRounding,RoundingStrategy::RoundHalfUp,RoundingStrategy::RoundHalfDown,RoundingStrategy::RoundDown,RoundingStrategy::RoundUp];
    let dp=input["dp"].as_u64().unwrap() as u32;
    if input["kind"]=="significant" {
        let results=strategies.iter().map(|s|match std::panic::catch_unwind(||a.round_sf_with_strategy(dp,*s)) {Ok(v)=>json!({"value":v.map(model)}),Err(_)=>json!({"panic":true})}).collect::<Vec<_>>();
        return json!({"strategies":results});
    }
    json!({"abs":model(a.abs()),"neg":model(-a),"trunc":model(a.trunc()),"fract":model(a.fract()),"floor":model(a.floor()),"ceil":model(a.ceil()),"normalize":model(a.normalize()),"round":model(a.round()),"round_dp":model(a.round_dp(dp)),"strategies":strategies.iter().map(|s|model(a.round_dp_with_strategy(dp,*s))).collect::<Vec<_>>()})
}
fn main() {std::panic::set_hook(Box::new(|_|{}));for line in io::stdin().lock().lines() {println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap()));}}
