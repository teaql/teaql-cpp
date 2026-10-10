//! Actual FromSqlError values and the locked driver's From conversion.
use serde_json::{json,Value as Json};
use std::io::{self,BufRead};
use rusqlite::{Error,types::{FromSqlError,Type}};

fn formatted<T:std::fmt::Debug>(value:&T)->Json {
    json!({"compact":format!("{value:?}"),"pretty":format!("{value:#?}"),
        "lower_hex":format!("{value:x?}"),"upper_hex":format!("{value:X?}"),
        "pretty_lower_hex":format!("{value:#x?}"),"pretty_upper_hex":format!("{value:#X?}"),
        "width_precision":format!("{value:>40.2?}"),"fancy":format!("{value:🧊^+18.3?}"),
        "zero_pad":format!("{value:+012?}")})
}
fn make(input:&Json)->FromSqlError {
    let bytes:Vec<u8>=input["bytes"].as_array().map(|v|v.iter().map(|x|x.as_u64().unwrap() as u8).collect()).unwrap_or_default();
    match input["kind"].as_str().unwrap() {
        "invalid"=>FromSqlError::InvalidType,
        "range"=>FromSqlError::OutOfRange(input["value"].as_str().unwrap().parse().unwrap()),
        "utf8"=>FromSqlError::from(std::str::from_utf8(&bytes).unwrap_err()),
        "blob"=>FromSqlError::InvalidBlobSize {
            expected_size:input["expected"].as_str().unwrap().parse().unwrap(),
            blob_size:input["actual"].as_str().unwrap().parse().unwrap()},
        "other"=>FromSqlError::other(std::str::from_utf8(&bytes).unwrap_err()),
        _=>panic!("unknown conversion fixture"),
    }
}
fn chain(error:&dyn std::error::Error)->Json {
    let mut values=Vec::new();
    let mut next=error.source();
    while let Some(cause)=next {
        values.push(json!({"debug":formatted(&cause),"display":cause.to_string()}));
        next=cause.source();
    }
    json!(values)
}
fn evaluate(input:Json,raw_debug:bool)->Json {
    let model=make(&input);
    let before=json!({"debug":formatted(&model),"display":model.to_string(),
        "self_equal":model==model,"fresh_equal":model==make(&input),"chain":chain(&model)});
    let driver=Error::from(model);
    let (kind,unknown,type_name)=match &driver {
        Error::IntegralValueOutOfRange(index,_)=> ("IntegralValueOutOfRange",*index==usize::MAX,None),
        Error::FromSqlConversionFailure(index,kind,_)=> ("FromSqlConversionFailure",*index==usize::MAX,Some(format!("{kind:?}"))),
        _=>panic!("unexpected original conversion"),
    };
    let type_is_blob=matches!(&driver,Error::FromSqlConversionFailure(_,Type::Blob,_));
    let mut result=json!({"model":before,"converted":{"kind":kind,"unknown_index":unknown,"type":type_name,
        "blob_type":type_is_blob,"display":driver.to_string(),"chain":chain(&driver),
        "self_equal":driver==driver,"native_none":driver.sqlite_error().is_none()}});
    if raw_debug { result["converted_raw_debug"]=formatted(&driver); }
    result
}
fn main() {
    let raw_debug=std::env::args().skip(1).any(|value|value=="--raw-debug");
    for line in io::stdin().lock().lines() { println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap(),raw_debug)); }
}
