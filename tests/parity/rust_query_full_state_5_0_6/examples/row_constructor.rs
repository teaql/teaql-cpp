use serde_json::{Value as Json,json};
use std::{io::{self,BufRead},sync::Arc};
use teaql_core::{CompactRow,CompactRowLayout,Value};
fn observe(input:Json)->Json {
    let names:Arc<[String]>=(0..input["columns"].as_u64().unwrap()).map(|i|format!("n{i}")).collect::<Vec<_>>().into();
    let values=vec![Value::Null;input["values"].as_u64().unwrap() as usize];
    let outcome=std::panic::catch_unwind(||{
        if input["entry"]=="new" {CompactRow::new(names,values)}
        else{CompactRow::with_layout(CompactRowLayout::new(names),values)}
    });
    match outcome {
        Ok(row)=>json!({"error":null,"row":format!("{row:?}")}),
        Err(error)=>json!({"error":error.downcast_ref::<String>().cloned().or_else(||error.downcast_ref::<&str>().map(|s|s.to_string())).unwrap(),"row":null}),
    }
}
fn main(){std::panic::set_hook(Box::new(|_|{}));for line in io::stdin().lock().lines(){println!("{}",observe(serde_json::from_str(&line.unwrap()).unwrap()));}}
