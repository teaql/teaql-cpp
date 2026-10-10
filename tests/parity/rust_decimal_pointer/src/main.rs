//! Actual pointer-width Decimal conversions; no conversion algorithms are copied.
use rust_decimal::Decimal;
use rust_decimal::prelude::{FromPrimitive, ToPrimitive};
use serde_json::{Value, json};
use std::io::{self, BufRead};

// Compile the unchanged source const-fn entries on both actual word widths.
const CONSTANT_DECIMAL: Decimal = Decimal::from_parts(123, 0, 0, true, 2);
const CONSTANT_BYTES: [u8; 16] = CONSTANT_DECIMAL.serialize();
#[allow(deprecated)]
const CONSTANT_BOUNDS: (Decimal, Decimal) = (Decimal::min_value(), Decimal::max_value());
const _: () = {
    let parts = CONSTANT_DECIMAL.unpack();
    assert!(CONSTANT_DECIMAL.scale() == 2);
    assert!(CONSTANT_DECIMAL.is_sign_negative());
    assert!(!CONSTANT_DECIMAL.is_sign_positive());
    assert!(parts.lo == 123);
    assert!(CONSTANT_DECIMAL.mantissa() == -123);
    assert!(CONSTANT_BYTES[2] == 2 && CONSTANT_BYTES[3] == 128 && CONSTANT_BYTES[4] == 123);
    assert!(Decimal::ZERO.is_zero());
    let minimum = CONSTANT_BOUNDS.0.unpack();
    let maximum = CONSTANT_BOUNDS.1.unpack();
    assert!(minimum.negative && minimum.lo == u32::MAX && minimum.mid == u32::MAX && minimum.hi == u32::MAX);
    assert!(!maximum.negative && maximum.lo == u32::MAX && maximum.mid == u32::MAX && maximum.hi == u32::MAX);
};

fn model(value: Decimal) -> Value {
    let parts = value.unpack();
    let text = std::panic::catch_unwind(|| value.to_string());
    let mut output = json!({"text": text.as_ref().ok(), "scale": value.scale(),
                           "negative": value.is_sign_negative(),
                           "coefficient": [parts.lo, parts.mid, parts.hi]});
    if text.is_err() {
        output["text_panic"] = json!(true);
    }
    output
}

fn error(error: rust_decimal::Error) -> Value {
    json!({"message": error.to_string(), "debug": format!("{error:?}")})
}

fn evaluate(input: Value) -> Value {
    let mut value: Decimal = input["text"].as_str().unwrap().parse().unwrap();
    value.set_sign_negative(input["negative"].as_bool().unwrap());
    value.rescale(input["scale"].as_u64().unwrap() as u32);
    let signed = isize::try_from(value).map(|value| json!({"value": value}))
        .unwrap_or_else(error);
    let unsigned = usize::try_from(value).map(|value| json!({"value": value}))
        .unwrap_or_else(error);
    let mut output = json!({
        "bits": usize::BITS,
        "input": model(value),
        "to_isize": value.to_isize(),
        "to_usize": value.to_usize(),
        "try_isize": signed,
        "try_usize": unsigned,
    });
    let signed_text = input["signed"].as_str().unwrap();
    let unsigned_text = input["unsigned"].as_str().unwrap();
    output["from_isize"] = match signed_text.parse::<isize>() {
        Ok(value) => json!({"optional": Decimal::from_isize(value).map(model),
                            "infallible": model(Decimal::from(value))}),
        Err(_) => json!({"input_rejected": true}),
    };
    output["from_usize"] = match unsigned_text.parse::<usize>() {
        Ok(value) => json!({"optional": Decimal::from_usize(value).map(model),
                            "infallible": model(Decimal::from(value))}),
        Err(_) => json!({"input_rejected": true}),
    };
    output["unchanged"] = model(value);
    output
}

fn main() {
    std::panic::set_hook(Box::new(|_| {}));
    if std::env::args().nth(1).as_deref() == Some("--pointer-bits") {
        println!("{}", usize::BITS);
        return;
    }
    for line in io::stdin().lock().lines() {
        let input = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", evaluate(input));
    }
}
