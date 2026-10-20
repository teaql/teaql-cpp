use cpp_business_ids_core::{E, Receipt, ReceiptNumber, Ticket, TicketNumber, ValueExpression};
use serde_json::{Value as Json, json};
use std::{collections::BTreeMap, io::{self, BufRead}, panic::{catch_unwind, AssertUnwindSafe}};
use teaql_core::{CompactRow, Entity, Value};
#[path = "../../rust_values/src/fixture_value.rs"]
mod fixture_value;
fn capture(function: impl FnOnce() -> Json) -> Json {
    match catch_unwind(AssertUnwindSafe(function)) {
        Ok(value) => json!({"ok":value}),
        Err(error) => json!({"panic":error.downcast_ref::<String>().cloned()
            .or_else(||error.downcast_ref::<&str>().map(|s|s.to_string())).unwrap()}),
    }
}
fn summary<T: Clone + Into<Value>>(expression: ValueExpression<'_,T>, fallback: T) -> Json {
    let mut callbacks=0;
    let eval=capture(|| match expression.clone().eval() {
        Some(value)=>json!({"present":true,"value":value.into().to_json_value()}),
        None=>json!({"present":false,"value":Json::Null}),
    });
    let unwrap=capture(||expression.clone().unwrap().into().to_json_value());
    let eager=capture(||expression.clone().or_if_null(fallback.clone()).into().to_json_value());
    let lazy=capture(||expression.or_else_if_null(|| {callbacks+=1;fallback.clone()}).into().to_json_value());
    json!({"eval":eval,"unwrap":unwrap,"eager":eager,"lazy":lazy,"callbacks":callbacks})
}
fn row(input: &Json,name: &str)->CompactRow {
    let mut fields=BTreeMap::from([("id".into(),Value::U64(input["id"].as_u64().unwrap())),("version".into(),Value::I64(1))]);
    if input["loaded"]==true {fields.insert(name.into(),fixture_value::value(&input["value"]));}
    CompactRow::from_map(fields)
}
fn observe(input:&Json)->Json {
    if input["model"]=="Receipt" {
        let entity=match Receipt::from_compact_row(row(input,"receipt_number")) {
            Ok(value)=>value,Err(error)=>return json!({"decode_error":error.to_string()}),
        };
        let expression=E::receipt(&entity).get_receipt_number();
        let mut alias=entity.clone();
        let before=summary(expression.clone(),ReceiptNumber::new("fallback").unwrap());
        alias.update_receipt_number(ReceiptNumber::new("replacement").unwrap());
        return json!({"before":before,"captured":summary(expression,ReceiptNumber::new("fallback").unwrap()),
                     "current":summary(E::receipt(&entity).get_receipt_number(),ReceiptNumber::new("fallback").unwrap())});
    }
    if input["model"]=="Ticket" {
        let entity=match Ticket::from_compact_row(row(input,"ticket_number")) {
            Ok(value)=>value,Err(error)=>return json!({"decode_error":error.to_string()}),
        };
        let expression=E::ticket(&entity).get_ticket_number();
        let mut alias=entity.clone();
        let before=summary(expression.clone(),TicketNumber::new("fallback").unwrap());
        alias.update_ticket_number(TicketNumber::new("replacement").unwrap());
        return json!({"before":before,"captured":summary(expression,TicketNumber::new("fallback").unwrap()),
                     "current":summary(E::ticket(&entity).get_ticket_number(),TicketNumber::new("fallback").unwrap())});
    }
    panic!("Unknown Business ID model");
}
fn main() {
    std::panic::set_hook(Box::new(|_|{}));
    for line in io::stdin().lock().lines() {
        let input:Json=serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}",observe(&input));
    }
}
