//! Original derived language, translator and private catalog diagnostics.
use serde_json::{json, Value as Json};
use std::{fmt::Debug, io::{self, BufRead}};
use teaql_runtime::{BuiltinTranslator, I18nCatalog, Language};

fn formatted<T: Debug>(value: &T) -> Json {
    json!({"compact":format!("{value:?}"),"pretty":format!("{value:#?}"),
        "lower_hex":format!("{value:x?}"),"upper_hex":format!("{value:X?}"),
        "pretty_lower_hex":format!("{value:#x?}"),"pretty_upper_hex":format!("{value:#X?}"),
        "width_precision":format!("{value:>40.2?}"),"fancy":format!("{value:🧊^+18.3?}"),
        "zero_pad":format!("{value:+012?}")})
}

fn evaluate(input: Json) -> Json {
    let language = Language::ALL[input["language"].as_u64().unwrap() as usize];
    let translator = BuiltinTranslator::new(language);
    let catalog = match input["source"].as_str() {
        Some(source) => I18nCatalog::from_json(source).unwrap(),
        None => I18nCatalog::builtin().as_ref().clone(),
    };
    json!({"language":formatted(&language),"default":formatted(&Language::default()),
        "translator":formatted(&translator),"translator_clone":formatted(&translator.clone()),
        "catalog":formatted(&catalog),"catalog_clone":formatted(&catalog.clone())})
}
fn main() {
    for line in io::stdin().lock().lines() {
        println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
