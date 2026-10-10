use serde_json::{json, Value as Json};
use std::io::{self, BufRead};
use teaql_core::*;
use teaql_core::request as request;
#[path = "../fixture_value.rs"]
mod fixture_value;

fn outcome<T: std::fmt::Debug>(operation: impl FnOnce() -> T + std::panic::UnwindSafe) -> Json {
    match std::panic::catch_unwind(operation) {
        Ok(value) => json!({"ok":format!("{value:?}")}),
        Err(error) => {
            let message = error.downcast_ref::<String>().cloned()
                .or_else(||error.downcast_ref::<&str>().map(|s|s.to_string())).unwrap();
            json!({"error":message})
        }
    }
}

fn evaluate(input: Json) -> Json {
    match input["kind"].as_str().unwrap() {
        "operators" => {
            use request::FieldOperator::*;
            let operator=[Equal,NotEqual,GreaterThan,GreaterThanOrEqual,LessThan,LessThanOrEqual,
                Between,In,NotIn,Contain,NotContain,BeginWith,NotBeginWith,EndWith,NotEndWith,SoundsLike,IsNull,IsNotNull]
                [input["operator"].as_u64().unwrap() as usize];
            let values:Vec<Value>=input["values"].as_array().unwrap().iter().map(fixture_value::value).collect();
            let index=input["index"].as_u64().unwrap() as usize;
            json!({"expression":outcome(||request::field_operator_expr("field",operator,values.clone())),
                "column":outcome(||request::field_operator_column_expr("field",operator,"other")),
                "value":outcome(||request::required_value(operator,&values,index)),
                "text":outcome(||request::required_text(operator,&values,index))})
        },
        "dynamic" => {
            let data=&input["json"];
            let object=serde_json::Map::from_iter([(String::from("field"),data.clone())]);
            json!({"value":format!("{:?}",request::dynamic_json_value_to_teaql_value(data)),
                "values":format!("{:?}",request::dynamic_json_values(data)),
                "operator":format!("{:?}",request::dynamic_json_operator(data)),
                "expression":outcome(||request::dynamic_json_filter_expr("field",data)),
                "u64":request::dynamic_json_u64_field(&object,"field").map(|n|n.to_string()),
                "missing_u64":request::dynamic_json_u64_field(&object,"absent").map(|n|n.to_string())})
        },
        "filters" => {
            let payload=fixture_value::value(&input["value"]);
            let target=Expr::eq("target",payload);
            let live=Expr::gt("version",0i64);
            let expression=match input["shape"].as_u64().unwrap() {
                0=>target.clone(),1=>Expr::And(vec![live.clone(),target.clone()]),
                2=>Expr::Or(vec![live.clone(),target.clone()]),3=>Expr::Not(Box::new(live.clone())),
                4=>Expr::And(Vec::new()),5=>Expr::And(vec![Expr::And(vec![live.clone(),live.clone()]),target.clone()]),
                _=>live.clone(),
            };
            json!({"original":format!("{expression:?}"),
                "without_default":format!("{:?}",request::remove_default_live_filter(Some(expression.clone()))),
                "without_target":format!("{:?}",request::remove_filter_expr(expression,&target)),
                "absent":format!("{:?}",request::remove_default_live_filter(None))})
        },
        "facets" => {
            let value=fixture_value::value(&input["value"]);
            let mut rows=SmartList::new(vec![value]);
            rows.total_count=Some(17);
            let mut fields=Record::new();fields.insert("count".into(),Value::I64(1));
            let facet=SmartList::new(vec![CompactRow::from_map(fields)]).with_total_count(9);
            rows.add_facet("replace",SmartList::empty());
            request::attach_facets(&mut rows,std::collections::BTreeMap::from([
                ("replace".into(),facet),("雪".into(),SmartList::empty())]));
            json!({"list":format!("{rows:?}")})
        },
        _=>unreachable!(),
    }
}

fn main() {
    std::panic::set_hook(Box::new(|_|{}));
    for line in io::stdin().lock().lines() {
        println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
