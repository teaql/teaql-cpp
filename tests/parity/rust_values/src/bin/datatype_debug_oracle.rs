use serde_json::{json, Value as Json};
use std::io::{self, BufRead};
use teaql_core::{DataType, Value};

fn formatted<T: std::fmt::Debug>(v: &T) -> Json {
    json!({"compact":format!("{v:?}"),"pretty":format!("{v:#?}"),
        "lower_hex":format!("{v:x?}"),"upper_hex":format!("{v:X?}"),
        "pretty_lower_hex":format!("{v:#x?}"),"pretty_upper_hex":format!("{v:#X?}"),
        "width_precision":format!("{v:>40.2?}"),"fancy":format!("{v:🧊^+18.3?}"),
        "zero_pad":format!("{v:+012?}")})
}
fn evaluate(input: Json) -> Json {
    let types = [DataType::Bool, DataType::I64, DataType::U64, DataType::F64,
        DataType::Decimal, DataType::Text, DataType::LargeText, DataType::Json,
        DataType::Date, DataType::Timestamp];
    let value = types[input["type"].as_u64().unwrap() as usize];
    json!({"type":formatted(&value),"typed_null":formatted(&Value::TypedNull(value)),
        "equal":types.iter().map(|other|value==*other).collect::<Vec<_>>()})
}
fn main() {
    for line in io::stdin().lock().lines() {
        println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
