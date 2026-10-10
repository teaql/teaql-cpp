use serde_json::{json, Value as Json};
use std::io::{self, BufRead};
use teaql_core::Decimal;

fn evaluate(input: Json) -> Json {
    let words = input["words"].as_array().unwrap();
    let mut value = Decimal::from_parts(
        words[0].as_u64().unwrap() as u32,
        words[1].as_u64().unwrap() as u32,
        words[2].as_u64().unwrap() as u32,
        input["negative"].as_bool().unwrap(),
        input["scale"].as_u64().unwrap() as u32,
    );
    if let Some(scale) = input["rescale"].as_u64() {
        value.rescale(scale as u32);
    }
    // Observe the original method, including its fixed-buffer panic boundary.
    let result = std::panic::catch_unwind(|| value.array_string().as_ref().to_owned());
    let raw = value.unpack();
    json!({"words":[raw.lo, raw.mid, raw.hi],"negative":raw.negative,"scale":raw.scale,
        "inline":match result {Ok(text)=>json!({"ok":text}),Err(_)=>json!({"panic":true})}})
}

fn main() {
    std::panic::set_hook(Box::new(|_| {}));
    for line in io::stdin().lock().lines() {
        println!("{}", evaluate(serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
