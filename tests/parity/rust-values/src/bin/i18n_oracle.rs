//! Observe original public language/catalog/Context/module calls only.
use serde_json::{Value as J, json};
use std::io::{self, BufRead};
use std::sync::Arc;
use teaql_runtime::{
    BuiltinTranslator, CheckResult, CheckRule, I18nCatalog, Language, MessageTranslator,
    ObjectLocation, RuntimeModule, UserContext,
};
#[path = "../fixture_value.rs"]
mod fixture_value;

fn language(input: &J) -> Language {
    Language::from_code(input["language"].as_str().unwrap_or("en")).unwrap()
}
fn location(input: &J) -> ObjectLocation {
    let mut out = ObjectLocation::root();
    for part in input["path"].as_array().unwrap() {
        out = if let Some(name) = part.as_str() {
            out.member(name)
        } else {
            out.element(part.as_u64().unwrap() as usize)
        };
    }
    out
}
fn check(input: &J) -> CheckResult {
    let rules = [
        CheckRule::Required,
        CheckRule::InvalidType,
        CheckRule::Min,
        CheckRule::Max,
        CheckRule::MinStringLength,
        CheckRule::MaxStringLength,
        CheckRule::ContextRootMissing,
        CheckRule::ContextRootMismatch,
    ];
    let mut out = CheckResult::new(
        rules[input["rule"].as_u64().unwrap() as usize],
        location(input),
    );
    if let Some(v) = input.get("input") {
        out = out.with_input_value(fixture_value::value(v));
    }
    if let Some(v) = input.get("system") {
        out = out.with_system_value(fixture_value::value(v));
    }
    if let Some(v) = input.get("message") {
        out = out.with_message(v.as_str().unwrap());
    }
    out
}
fn evaluate(input: J) -> J {
    let op = input["op"].as_str().unwrap();
    if op == "language" {
        let code = Language::from_code(input["code"].as_str().unwrap()).map(|v| v.code());
        return json!({"code":code,"all":Language::ALL.map(|v|v.code())});
    }
    if op == "path" {
        return json!({"text":teaql_runtime::translate_location(language(&input), &location(&input))});
    }
    let catalog = if let Some(source) = input["source"].as_str() {
        match I18nCatalog::from_json(source) {
            Ok(value) => Arc::new(value),
            Err(error) => return json!({"error":error.to_string()}),
        }
    } else {
        I18nCatalog::builtin().clone()
    };
    match op {
        "catalog" => json!({
            "messages":input["messages"].as_array().unwrap().iter().map(|v|catalog.message(language(&input),v.as_str().unwrap())).collect::<Vec<_>>(),
            "vocabulary":input["vocabulary"].as_array().unwrap().iter().map(|v|catalog.vocabulary(language(&input),v.as_str().unwrap())).collect::<Vec<_>>()
        }),
        "translate" => {
            let result = check(&input);
            let translator = BuiltinTranslator::new(language(&input));
            json!({"text":catalog.translate_check_result(language(&input),&result),
                   "builtin":translator.translate_check_result(&result),
                   "language":translator.language().code()})
        }
        "context" => {
            let mut context = UserContext::new()
                .with_language(language(&input))
                .with_i18n_catalog(catalog);
            let mut error = None;
            if let Some(code) = input["set"].as_str() {
                let outcome = if input["alias"].as_bool().unwrap_or(false) {
                    context.set_locale_code(code)
                } else {
                    context.set_language_code(code)
                };
                error = outcome.err().map(|v| v.to_string());
            }
            if let Some(module_language) = input["module"].as_str() {
                let mut module = RuntimeModule::new();
                if !module_language.is_empty() {
                    module = module.language(Language::from_code(module_language).unwrap());
                }
                context = context.with_module(module);
            }
            let mut checks: Vec<_> = input["checks"]
                .as_array()
                .unwrap()
                .iter()
                .map(check)
                .collect();
            context.translate_check_results(&mut checks);
            context.translate_check_results(&mut checks);
            json!({"language":context.language().code(),"error":error,
                   "messages":checks.iter().map(|v|v.message.clone()).collect::<Vec<_>>()})
        }
        _ => panic!("unknown operation"),
    }
}
fn main() {
    for line in io::stdin().lock().lines() {
        println!(
            "{}",
            evaluate(serde_json::from_str(&line.unwrap()).unwrap())
        );
    }
}
