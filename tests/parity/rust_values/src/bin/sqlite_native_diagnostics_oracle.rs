//! Actual standard-library payloads held by the locked SQLite driver's errors.
use serde_json::{json,Value as Json};
use std::ffi::{CString,OsString};
use std::io::{self,BufRead};
use std::os::unix::ffi::{OsStringExt,OsStrExt};
use std::path::PathBuf;

fn formatted<T:std::fmt::Debug>(value:&T)->Json {
    json!({"compact":format!("{value:?}"),"pretty":format!("{value:#?}"),
        "lower_hex":format!("{value:x?}"),"upper_hex":format!("{value:X?}"),
        "pretty_lower_hex":format!("{value:#x?}"),"pretty_upper_hex":format!("{value:#X?}"),
        "width_precision":format!("{value:>40.2?}"),"fancy":format!("{value:🧊^+18.3?}"),
        "zero_pad":format!("{value:+012?}")})
}
fn bytes(input:&Json)->Vec<u8> {
    input.as_array().unwrap().iter().map(|v|v.as_u64().unwrap() as u8).collect()
}
fn evaluate(input:Json)->Json {
    let raw=bytes(&input["bytes"]);
    let utf8=std::str::from_utf8(&raw).err().map(|error| json!({
        "debug":formatted(&error),"display":error.to_string(),
        "valid_up_to":error.valid_up_to(),"error_len":error.error_len(),
        "source_none":std::error::Error::source(&error).is_none(),
        "clone_equal":error==error.clone()}));
    let nul=CString::new(raw.clone()).err().map(|error| json!({
        "debug":formatted(&error),"display":error.to_string(),"position":error.nul_position(),
        "clone_equal":error==error.clone(),"source_none":std::error::Error::source(&error).is_none(),
        "into_vec":error.clone().into_vec()}));
    let path=PathBuf::from(OsString::from_vec(raw));
    let other=PathBuf::from(OsString::from_vec(bytes(&input["other"])));
    json!({"utf8":utf8,"nul":nul,"path":{
        "debug":formatted(&path),"lossy":path.to_string_lossy(),
        "bytes":path.as_os_str().as_bytes(),"other_equal":path==other}})
}
fn main() {
    for line in io::stdin().lock().lines() {
        println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
