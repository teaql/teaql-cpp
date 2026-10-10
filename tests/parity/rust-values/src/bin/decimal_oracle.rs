use std::io::{self,BufRead};
use serde_json::{json,Value as Json};
use rust_decimal::{Decimal,RoundingStrategy};
use rust_decimal::prelude::{FromPrimitive,ToPrimitive};
use std::hash::{Hash,Hasher};
use rust_decimal::prelude::{Signed,Zero,One};
#[derive(Default)]
struct HashWords(Vec<u32>);
impl Hasher for HashWords {
    fn finish(&self)->u64 {0}
    fn write(&mut self,_:&[u8]) {panic!("unexpected byte hashing")}
    fn write_u32(&mut self,value:u32) {self.0.push(value);}
}
fn words(value:u128)->[u32;4] { [value as u32,(value>>32) as u32,(value>>64) as u32,(value>>96) as u32] }
fn error_code(error:&rust_decimal::Error)->u32 {
    match error {
        rust_decimal::Error::ErrorString(_)=>0,
        rust_decimal::Error::ExceedsMaximumPossibleValue=>1,
        rust_decimal::Error::LessThanMinimumPossibleValue=>2,
        rust_decimal::Error::Underflow=>3,
        rust_decimal::Error::ScaleExceedsMaximumPrecision(_)=>4,
        rust_decimal::Error::ConversionTo(_)=>5,
    }
}
fn error_model(error:rust_decimal::Error)->Json {json!({"code":error_code(&error),"error":error.to_string()})}
fn model(value:Decimal)->Json {
    let unpacked=value.unpack();
    let text=std::panic::catch_unwind(||value.to_string());
    let mut result=json!({"text":text.as_ref().ok(),"scale":value.scale(),"negative":value.is_sign_negative(),"coefficient":[unpacked.lo,unpacked.mid,unpacked.hi]});
    if text.is_err() {result["text_panic"]=json!(true);}result
}
fn evaluate(input:Json)->Json {
    if input["kind"] == "ord_defaults" {
        let value = |name: &str| {
            let input = &input[name];
            let mut value: Decimal = input["text"].as_str().unwrap().parse().unwrap();
            value.set_sign_negative(input["negative"].as_bool().unwrap());
            value.rescale(input["scale"].as_u64().unwrap() as u32);
            value
        };
        let left = value("left");
        let right = value("right");
        let minimum = value("minimum");
        let maximum = value("maximum");
        let mut output = json!({
            "left": model(left), "right": model(right),
            "minimum": model(minimum), "maximum": model(maximum),
            "inherent_min": model(Decimal::min(left, right)),
            "inherent_max": model(Decimal::max(left, right)),
            "ord_min": model(Ord::min(left, right)),
            "ord_max": model(Ord::max(left, right)),
        });
        output["clamp"] = match std::panic::catch_unwind(|| Ord::clamp(left, minimum, maximum)) {
            Ok(value) => json!({"value": model(value)}),
            Err(_) => json!({"panic": true}),
        };
        output["unchanged"] = json!([model(left), model(right), model(minimum), model(maximum)]);
        return output;
    }
    if input["kind"]=="identities" {
        let mut value:Decimal=input["text"].as_str().unwrap().parse().unwrap();
        value.set_sign_negative(input["negative"].as_bool().unwrap());
        value.rescale(input["scale"].as_u64().unwrap() as u32);
        let mut result=json!({"input":model(value),"is_one":value.is_one()});
        value.set_zero();
        result["zero"]=model(value);result["zero_is_one"]=json!(value.is_one());
        value.set_one();
        result["one"]=model(value);result["one_is_one"]=json!(value.is_one());
        return result;
    }
    if input["kind"]=="reciprocal" {
        let mut value:Decimal=input["text"].as_str().unwrap().parse().unwrap();
        value.set_sign_negative(input["negative"].as_bool().unwrap());
        value.rescale(input["scale"].as_u64().unwrap() as u32);
        let mut result=json!({"input":model(value)});
        match std::panic::catch_unwind(||num_traits::Inv::inv(value)) {
            Ok(v)=>result["inverse"]=model(v), Err(_)=>result["panic"]=json!(true)
        }
        result["after"]=model(value);
        return result;
    }
    if input["kind"]=="decimal_json" {
        let raw:Json=serde_json::from_str(input["json"].as_str().unwrap()).unwrap();
        return match serde_json::from_value::<Decimal>(raw) {
            Ok(v)=>json!({"value":model(v),"serialized":serde_json::to_value(v).unwrap()}),
            Err(e)=>json!({"error":e.to_string()})
        };
    }
    if input["kind"]=="decimal_format" {
        let mut value:Decimal=input["text"].as_str().unwrap().parse().unwrap();
        value.set_sign_negative(input["negative"].as_bool().unwrap());
        value.rescale(input["scale"].as_u64().unwrap() as u32);
        let width=input["width"].as_u64().unwrap() as usize;
        let precision=input["precision"].as_u64().map(|v|v as usize);
        let capture=|f:&dyn Fn()->String|match std::panic::catch_unwind(std::panic::AssertUnwindSafe(f)) {Ok(v)=>json!(v),Err(_)=>json!({"panic":true})};
        let mut result=json!({});
        macro_rules! mode {($name:literal,$tag:literal)=>{{
            let mut fields=json!({});
            fields["plain"]=capture(&||format!(concat!("{:",$tag,"}"),value));
            fields["width"]=capture(&||format!(concat!("{:width$",$tag,"}"),value,width=width));
            macro_rules! option {($name2:literal,$start:literal,$with:literal)=>{
                fields[$name2]=capture(&||match precision {
                    Some(p)=>format!(concat!("{:",$start,$with,".precision$",$tag,"}"),value,width=width,precision=p),
                    None=>format!(concat!("{:",$start,$with,$tag,"}"),value,width=width)
                });
            }}
            fields["precision"]=capture(&||match precision {
                Some(p)=>format!(concat!("{:.precision$",$tag,"}"),value,precision=p),
                None=>format!(concat!("{:",$tag,"}"),value)
            });
            option!("padded","","width$");option!("plus","+","width$");
            option!("zero","+0","width$");option!("left","_<+","width$");
            option!("right","_>+","width$");option!("center","_^+","width$");
            option!("unicode","🦀^+","width$");
            result[$name]=fields;
        }}}
        mode!("display","");mode!("debug","?");mode!("lower","e");mode!("upper","E");
        match std::panic::catch_unwind(||serde_json::to_value(value).unwrap()) {
            Ok(v)=>result["serialized"]=v,Err(_)=>result["serialize_panic"]=json!(true)
        }
        return result;
    }
    if input["kind"]=="scientific" {
        let text=input["text"].as_str().unwrap();
        let show=|value:Result<Decimal,rust_decimal::Error>|match value {Ok(v)=>model(v),Err(e)=>error_model(e)};
        return json!({"strict":show(Decimal::from_scientific(text)),"lossy":show(Decimal::from_scientific_lossy(text)),"normal":show(text.parse())});
    }
    if input["kind"]=="constants" {
        let values=[Decimal::MIN,Decimal::MAX,Decimal::ZERO,Decimal::ONE,Decimal::NEGATIVE_ONE,Decimal::TWO,Decimal::TEN,Decimal::ONE_HUNDRED,Decimal::ONE_THOUSAND,Decimal::zero(),Decimal::one()];
        return json!({"values":values.into_iter().map(model).collect::<Vec<_>>(),"max_scale":Decimal::MAX_SCALE});
    }
    if input["kind"]=="traits" {
        let mut value:Decimal=input["text"].as_str().unwrap().parse().unwrap();
        value.set_sign_negative(input["negative"].as_bool().unwrap());
        value.rescale(input["scale"].as_u64().unwrap() as u32);
        let other:Decimal=input["other"].as_str().unwrap().parse().unwrap();
        let mut changed=value;
        #[allow(deprecated)] {changed.set_sign(input["positive"].as_bool().unwrap());}
        let u=value.unpack();
        let mut words=HashWords::default();value.hash(&mut words);
        #[allow(deprecated)]
        let mut result=json!({"signum":model(value.signum()),"positive":value.is_positive(),"negative":value.is_negative(),"set_sign":model(changed),
            "unpacked":{"negative":u.negative,"scale":u.scale,"hi":u.hi,"mid":u.mid,"lo":u.lo},"hash_words":words.0});
        match std::panic::catch_unwind(||Decimal::from(u)) {Ok(v)=>result["repacked"]=model(v),Err(_)=>result["repack_panic"]=json!(true)};
        match std::panic::catch_unwind(||value.abs_sub(&other)) {Ok(v)=>result["abs_sub"]=model(v),Err(_)=>result["abs_sub_panic"]=json!(true)};
        return result;
    }
    if input["kind"]=="fold" {
        let values:Vec<Decimal>=input["values"].as_array().unwrap().iter().map(|v|v.as_str().unwrap().parse().unwrap()).collect();
        let mut result=json!({});
        match std::panic::catch_unwind(||values.iter().sum::<Decimal>()) {Ok(v)=>result["sum"]=model(v),Err(_)=>result["sum_panic"]=json!(true)};
        match std::panic::catch_unwind(||values.iter().product::<Decimal>()) {Ok(v)=>result["product"]=model(v),Err(_)=>result["product_panic"]=json!(true)};
        return result;
    }
    if input["kind"]=="numeric_float" {
        let bits=input["bits"].as_u64().unwrap();
        let (value,retained)=if input["width"]==32 {
            let floating=f32::from_bits(bits as u32);
            (Decimal::from_f32(floating),Decimal::from_f32_retain(floating))
        } else {
            let floating=f64::from_bits(bits);
            (Decimal::from_f64(floating),Decimal::from_f64_retain(floating))
        };
        let checked=if input["width"]==32 {
            Decimal::try_from(f32::from_bits(bits as u32))
        } else {Decimal::try_from(f64::from_bits(bits))};
        return json!({"value":value.map(model),"retained":retained.map(model),"conversion":match checked {Ok(v)=>model(v),Err(e)=>error_model(e)}});
    }
    if input["kind"]=="numeric_wide" {
        let raw=input["words"].as_array().unwrap().iter().enumerate().fold(0u128,|value,(i,word)|value|((word.as_u64().unwrap() as u128)<<(i*32)));
        let scale=input["scale"].as_u64().unwrap() as u32;
        let mut result=json!({"unsigned":Decimal::from_u128(raw).map(model),"signed":Decimal::from_i128(raw as i128).map(model)});
        result["from_signed"]=match std::panic::catch_unwind(||Decimal::from(raw as i128)) {Ok(v)=>model(v),Err(_)=>json!({"panic":true})};
        result["from_unsigned"]=match std::panic::catch_unwind(||Decimal::from(raw)) {Ok(v)=>model(v),Err(_)=>json!({"panic":true})};
        match Decimal::try_from_i128_with_scale(raw as i128,scale) {
            Ok(value)=>result["scaled"]=model(value),
            Err(error)=>{result["code"]=json!(error_code(&error));result["error"]=json!(error.to_string());}
        }
        match std::panic::catch_unwind(||Decimal::from_i128_with_scale(raw as i128,scale)) {
            Ok(value)=>result["throwing"]=model(value),Err(_)=>result["panic"]=json!(true)
        }
        return result;
    }
    if input["kind"]=="numeric_decimal" {
        let mut value:Decimal=input["text"].as_str().unwrap().parse().unwrap();
        if input["negative"]==true {value.set_sign_negative(true);}
        value.rescale(input["scale"].as_u64().unwrap() as u32);
        let mut result=json!({"i8":value.to_i8(),"i16":value.to_i16(),"i32":value.to_i32(),"i64":value.to_i64(),
            "u8":value.to_u8(),"u16":value.to_u16(),"u32":value.to_u32(),"u64":value.to_u64(),
            "i128":words(value.to_i128().unwrap() as u128),"u128":value.to_u128().map(words),"mantissa":words(value.mantissa() as u128)});
        let mut checked=json!({"i128":words(i128::try_from(value).unwrap() as u128),"u128":match u128::try_from(value) {Ok(v)=>json!(words(v)),Err(e)=>error_model(e)}});
        macro_rules! check_type {($t:ty)=> {checked[stringify!($t)]=match <$t>::try_from(value) {Ok(v)=>json!(v),Err(e)=>error_model(e)};}}
        check_type!(i8);check_type!(i16);check_type!(i32);check_type!(i64);
        check_type!(u8);check_type!(u16);check_type!(u32);check_type!(u64);
        result["checked"]=checked;
        match std::panic::catch_unwind(||(value.to_f32().unwrap().to_bits(),value.to_f64().unwrap().to_bits(),f32::try_from(value).unwrap().to_bits(),f64::try_from(value).unwrap().to_bits())) {
            Ok((f32,f64,f32_result,f64_result))=>{result["f32"]=json!(f32);result["f64"]=json!(f64);result["f32_result"]=json!(f32_result);result["f64_result"]=json!(f64_result);},
            Err(_)=>result["float_panic"]=json!(true)
        }
        return result;
    }
    if input["kind"]=="numeric_i64" {
        let raw=input["text"].as_str().unwrap().parse::<i64>().unwrap();
        let scale=input["scale"].as_u64().unwrap() as u32;
        let mut result=json!({"value":model(Decimal::from_i64(raw).unwrap())});
        match Decimal::try_new(raw,scale) {
            Ok(value)=>result["scaled"]=model(value),
            Err(error)=>{result["code"]=json!(error_code(&error));result["error"]=json!(error.to_string());}
        }
        match std::panic::catch_unwind(||Decimal::new(raw,scale)) {
            Ok(value)=>result["throwing"]=model(value),Err(_)=>result["panic"]=json!(true)
        }
        return result;
    }
    if input["kind"]=="parse" {
        let text=input["text"].as_str().unwrap();
        let parsed=std::panic::catch_unwind(||match input["method"].as_str().unwrap() {
            "exact"=>Decimal::from_str_exact(text),
            "radix"=>Decimal::from_str_radix(text,input["radix"].as_u64().unwrap() as u32),
            _=>text.parse::<Decimal>()
        });
        return match parsed {
            Ok(Ok(value))=>json!({"value":model(value)}),
            Ok(Err(error))=>{
                let code=match &error {
                    rust_decimal::Error::ErrorString(_)=>0,
                    rust_decimal::Error::ExceedsMaximumPossibleValue=>1,
                    rust_decimal::Error::LessThanMinimumPossibleValue=>2,
                    rust_decimal::Error::Underflow=>3,
                    rust_decimal::Error::ScaleExceedsMaximumPrecision(_)=>4,
                    rust_decimal::Error::ConversionTo(_)=>5,
                };
                json!({"error":error.to_string(),"code":code})
            },
            Err(_)=>json!({"panic":true})
        };
    }
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
    if input["kind"]=="extended_binary" {
        let mut b:Decimal=input["b"].as_str().unwrap().parse().unwrap();
        a.set_sign_negative(input["a_negative"].as_bool().unwrap());
        b.set_sign_negative(input["b_negative"].as_bool().unwrap());
        a.rescale(input["a_scale"].as_u64().unwrap() as u32);
        b.rescale(input["b_scale"].as_u64().unwrap() as u32);
        let mut result=json!({"a":model(a),"b":model(b)});
        macro_rules! checked {($name:literal,$method:ident)=>{
            result[$name]=match std::panic::catch_unwind(||a.$method(b)) {
                Ok(v)=>json!(v.map(model)),Err(_)=>json!({"panic":true})
            };
        }}
        macro_rules! saturated {($name:literal,$method:ident)=>{
            result[$name]=match std::panic::catch_unwind(||a.$method(b)) {
                Ok(v)=>model(v),Err(_)=>json!({"panic":true})
            };
        }}
        checked!("add",checked_add);checked!("sub",checked_sub);
        checked!("mul",checked_mul);checked!("div",checked_div);checked!("rem",checked_rem);
        saturated!("saturating_add",saturating_add);saturated!("saturating_sub",saturating_sub);
        saturated!("saturating_mul",saturating_mul);
        result["compare"]=match std::panic::catch_unwind(||a.cmp(&b)) {
            Ok(v)=>json!(match v {std::cmp::Ordering::Less=>-1,std::cmp::Ordering::Equal=>0,std::cmp::Ordering::Greater=>1}),
            Err(_)=>json!({"panic":true})
        };
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
