//! Actual enabled locked driver alternatives, not a copied error implementation.
use serde_json::{json,Value as Json};
use std::io::{self,BufRead};
use rusqlite::{Error,types::Type};

fn formatted<T:std::fmt::Debug>(value:&T)->Json {
    json!({"compact":format!("{value:?}"),"pretty":format!("{value:#?}"),
        "lower_hex":format!("{value:x?}"),"upper_hex":format!("{value:X?}"),
        "pretty_lower_hex":format!("{value:#x?}"),"pretty_upper_hex":format!("{value:#X?}"),
        "width_precision":format!("{value:>40.2?}"),"fancy":format!("{value:🧊^+18.3?}"),
        "zero_pad":format!("{value:+012?}")})
}
fn make_error(input:&Json)->Error {
    let index=||input["index"].as_str().unwrap().parse::<usize>().unwrap();
    match input["kind"].as_u64().unwrap() {
        0=>Error::SqliteSingleThreadedMode,
        1=>Error::IntegralValueOutOfRange(index(),input["value"].as_str().unwrap().parse().unwrap()),
        2=>Error::InvalidParameterName(input["name"].as_str().unwrap().into()),
        3=>Error::QueryReturnedNoRows,
        4=>Error::QueryReturnedMoreThanOneRow,
        5=>Error::InvalidColumnName(input["name"].as_str().unwrap().into()),
        6=>Error::StatementChangedRows(index()),
        7=>Error::InvalidFunctionParameterType(index(),
            [Type::Null,Type::Integer,Type::Real,Type::Text,Type::Blob][input["type"].as_u64().unwrap() as usize]),
        8=>Error::InvalidQuery,
        9=>Error::UnwindingPanic,
        10=>Error::GetAuxWrongType,
        11=>Error::InvalidDatabaseIndex(index()),
        _=>panic!("unknown enabled fixture"),
    }
}
fn evaluate(input:Json)->Json {
    let error=make_error(&input);
    let equal=error==make_error(&input);
    let changed=input.get("changed").map(|other|error==make_error(other));
    json!({"debug":formatted(&error),"display":error.to_string(),
        "self_equal":error==error,"same_payload_equal":equal,"changed_equal":changed,
        "source_none":std::error::Error::source(&error).is_none()})
}
fn main() {
    for line in io::stdin().lock().lines() {
        println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
