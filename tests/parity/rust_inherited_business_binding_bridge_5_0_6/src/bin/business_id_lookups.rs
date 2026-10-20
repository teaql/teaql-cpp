//! Existing qualified model-binding bridge; original q.rs lookup body is untouched.
use cpp_inheritance_business_ids_core::{Q,DocumentNumber};
use cpp_inheritance_business_ids_core::teaql_core::SelectQuery;
use serde_json::{json,Value as Json};
use std::io::{self,BufRead};
fn observe(input:&Json)->Json {
    let number=match DocumentNumber::new(input["text"].as_str().unwrap()) {
        Ok(value)=>value,Err(error)=>return json!({"error":error}),
    };
    let value=number.as_str().to_owned();
    let manual:SelectQuery=Q::documents().with_document_number_is(number.clone()).limit(2).into();
    let mut query=Q::document_by_document_number(number);
    let lookup:SelectQuery=query.clone().into();
    query=match input["operation"].as_str().unwrap() {
        "larger"=>query.limit(10),"offset"=>query.skip(2).limit(10),
        "deleted"=>query.with_deleted_rows().limit(10),_=>query,
    };
    let after:SelectQuery=query.into();
    json!({"value":value,"manual":format!("{manual:?}"),"lookup":format!("{lookup:?}"),
        "after":format!("{after:?}"),"rows":[],"scalar":null,"error":null,"logs":[]})
}
fn main(){for line in io::stdin().lock().lines(){let input:Json=serde_json::from_str(&line.unwrap()).unwrap();println!("{}",observe(&input));}}
