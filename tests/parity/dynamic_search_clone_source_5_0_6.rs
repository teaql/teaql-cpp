use serde_json::{Value as Json, json};
use std::io::{self, BufRead};
use teaql_core::{Expr, SelectQuery};
use teaql_core::dynamic_search::*;

fn snapshot(model: &SearchModel, normalized: &NormalizedDynamicSearch, result: &DynamicSearchResult) -> Json {
    json!({"model": format!("{model:?}"), "normalized": format!("{normalized:?}"), "result": format!("{result:?}")})
}

fn observe(input: Json) -> Json {
    let text = input["text"].as_str().unwrap();
    let flags = input["flags"].as_u64().unwrap();
    let mut model = SearchModel::default();
    if flags & 1 != 0 {
        model.fields.insert(text.into(), "string".into());
        model.relations.insert("child".into(), text.into());
    }
    let warning = DynamicSearchWarning { code: "DYNAMIC_SEARCH_UNKNOWN_FIELD", entity: "School".into(), clause: "FILTER", field_path: text.into() };
    let normalized = NormalizedDynamicSearch {
        filters: if flags & 1 == 0 { vec![] } else { vec![DynamicSearchFilter { field_path: text.into(), operator: "$eq".into(), value: input["value"].clone() }] },
        orders: if flags & 2 == 0 { vec![] } else { vec![DynamicSearchOrder { field_path: text.into(), descending: true }] },
        warnings: if flags == 0 { vec![] } else { vec![warning.clone()] },
    };
    let result = DynamicSearchResult { query: SelectQuery::new("School").filter(Expr::eq("name", text)).comment("source scope"), warnings: vec![warning] };
    let before = snapshot(&model, &normalized, &result);
    let mut model_copy = model.clone();
    let mut normalized_copy = normalized.clone();
    let mut result_copy = result.clone();
    let copied = snapshot(&model_copy, &normalized_copy, &result_copy);
    model_copy.fields.insert("mutated".into(), "number".into());
    normalized_copy.filters.push(DynamicSearchFilter { field_path: "later".into(), operator: "$ne".into(), value: json!("changed") });
    let source_after = snapshot(&model, &normalized, &result);
    drop(model);
    drop(normalized);
    drop(result);
    normalized_copy.orders.push(DynamicSearchOrder { field_path: "later".into(), descending: false });
    normalized_copy.warnings.push(DynamicSearchWarning { code: "DYNAMIC_SEARCH_UNKNOWN_FIELD", entity: "School".into(), clause: "ORDER_BY", field_path: "later".into() });
    result_copy.query = result_copy.query.comment("retained changed");
    result_copy.warnings.push(DynamicSearchWarning { code: "DYNAMIC_SEARCH_UNKNOWN_FIELD", entity: "School".into(), clause: "ORDER_BY", field_path: "later".into() });
    json!({"default": format!("{:?}", SearchModel::default()), "source_before": before, "clone_before": copied, "source_after": source_after, "clone_after_source_drop": snapshot(&model_copy, &normalized_copy, &result_copy)})
}

fn main() {
    for line in io::stdin().lock().lines() {
        println!("{}", observe(serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
