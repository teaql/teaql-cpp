use serde_json::{json, Value as Json};
use std::io::{self, BufRead};
use teaql_core::Expr;

fn formatted(value: &Expr) -> Json {
    json!({"compact": format!("{value:?}"), "pretty": format!("{value:#?}"),
        "lower_hex": format!("{value:x?}"), "upper_hex": format!("{value:X?}"),
        "pretty_lower_hex": format!("{value:#x?}"), "pretty_upper_hex": format!("{value:#X?}"),
        "width_precision": format!("{value:>40.2?}"), "fancy": format!("{value:🧊^+18.3?}"),
        "zero_pad": format!("{value:+012?}")})
}

fn observe(input: &Json) -> Json {
    let column = input["column"].as_str().unwrap();
    let text = input["text"].as_str().unwrap();
    let expressions = [Expr::contain(column, text), Expr::not_contain(column, text),
        Expr::begin_with(column, text), Expr::not_begin_with(column, text),
        Expr::end_with(column, text), Expr::not_end_with(column, text)];
    let observations: Vec<_> = expressions.iter().enumerate().map(|(index, expr)| {
        let Expr::Binary {right, ..} = expr else { panic!("typed helper is not binary") };
        let Expr::LikePattern {pattern, original} = &**right else { panic!("typed helper lost provenance") };
        let raw = if index % 2 == 0 {Expr::like(column, pattern)} else {Expr::not_like(column, pattern)};
        let changed_original = Expr::LikePattern {pattern:pattern.clone(), original:format!("{original}x")};
        let changed_pattern = Expr::LikePattern {pattern:format!("{pattern}x"), original:original.clone()};
        json!({"formatted": formatted(expr), "leaf": formatted(right), "pattern": pattern,
            "original": original, "clone_equal": *expr == expr.clone(), "raw_equal": *expr == raw,
            "raw": formatted(&raw), "changed_original_equal": **right == changed_original,
            "changed_pattern_equal": **right == changed_pattern})
    }).collect();
    json!(observations)
}

fn main() {
    for line in io::stdin().lock().lines() {
        let input: Json = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", observe(&input));
    }
}
