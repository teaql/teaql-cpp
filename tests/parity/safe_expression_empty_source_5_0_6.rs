use serde_json::{json, Value as Json};
use std::io::{self, BufRead};
use std::sync::{Arc, atomic::{AtomicUsize, Ordering}};
use teaql_core::{SafeExpression, SmartList, TeaqlEmpty};
#[path = "rust_values/src/fixture_value.rs"]
mod fixture_value;

fn inspect<T: TeaqlEmpty + Clone + Send + Sync + 'static>(root: T, present: bool) -> Json {
    let direct = root.teaql_is_empty();
    let evaluations = Arc::new(AtomicUsize::new(0));
    let counter = evaluations.clone();
    let expression = SafeExpression::new(root, move |root| {
        counter.fetch_add(1, Ordering::SeqCst);
        present.then(|| root.clone())
    });
    let empty = expression.is_empty();
    let not_empty = expression.is_not_empty();
    let is_null = expression.is_null();
    let not_null = expression.is_not_null();
    let mut empty_callbacks = 0;
    let mut value_callbacks = 0;
    let mut null_callbacks = 0;
    let mut nonnull_callbacks = 0;
    expression.when_is_empty(|| empty_callbacks += 1);
    expression.when_not_empty(|_| value_callbacks += 1);
    expression.when_is_null(|| null_callbacks += 1);
    expression.when_is_not_null(|_| nonnull_callbacks += 1);
    json!({"direct_empty":direct, "empty":empty, "not_empty":not_empty,
        "null":is_null, "not_null":not_null, "evaluations":evaluations.load(Ordering::SeqCst),
        "empty_callbacks":empty_callbacks, "value_callbacks":value_callbacks,
        "null_callbacks":null_callbacks, "nonnull_callbacks":nonnull_callbacks})
}

fn observe(input: &Json) -> Json {
    let present = input["present"].as_bool().unwrap();
    let texts = ["", " ", "\u{2003}", "雪😀", "a\0b", "\n\r\t", "e\u{301}", "%_\\\"'"];
    match input["kind"].as_str().unwrap() {
        "string" => inspect(texts[input["index"].as_u64().unwrap() as usize].to_owned(), present),
        "borrowed" => inspect(texts[input["index"].as_u64().unwrap() as usize], present),
        "vector" => inspect(input["items"].as_array().unwrap().iter()
            .map(|value| value.as_str().unwrap().to_owned()).collect::<Vec<_>>(), present),
        "smart_list" => {
            let mut list = SmartList::new(input["items"].as_array().unwrap().iter()
                .map(fixture_value::value).collect());
            list.is_loaded = input["loaded"].as_bool().unwrap();
            inspect(list, present)
        },
        "option" => inspect(input["some"].as_bool().unwrap()
            .then(|| fixture_value::value(&input["value"])), present),
        "value" => inspect(fixture_value::value(&input["value"]), present),
        _ => panic!("Unknown emptiness fixture kind"),
    }
}

fn main() {
    for line in io::stdin().lock().lines() {
        let input: Json = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", observe(&input));
    }
}
