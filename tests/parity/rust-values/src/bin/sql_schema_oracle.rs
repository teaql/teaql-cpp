use std::io::{self,BufRead};
use teaql_sql::*;
use serde_json::{Value as J,json};
#[allow(dead_code)]
#[path="../fixture_value.rs"] mod fixture_value;
#[allow(dead_code)]
#[path="../fixture_sql.rs"] mod fixture_sql;
use fixture_sql::{FixtureDialect,entity};
fn evaluate(j:J)->J{
    match j["mode"].as_str().unwrap(){
        "foreign_key"=>json!({"name":schema_foreign_key_name(j["source_table"].as_str().unwrap(),j["source_column"].as_str().unwrap(),j["target_table"].as_str().unwrap(),j["target_column"].as_str().unwrap(),j["max"].as_u64().unwrap() as usize)}),
        "indexes"=>{
            let e=entity(&j["entity"]);let specs=schema_index_specs(&e,j["max"].as_u64().map(|v|v as usize));let d=FixtureDialect{numbered:false};
            json!({"specs":specs.iter().map(|s|json!({"name":s.name,"table":s.table,"columns":s.columns,"unique":s.unique})).collect::<Vec<_>>(),"sql":d.schema_indexes_sqls(&e).unwrap()})
        },
        "validate"=>{
            let entities:Vec<_>=j["entities"].as_array().unwrap().iter().map(entity).collect();let refs:Vec<_>=entities.iter().collect();
            match validate_schema_identifier_lengths(&refs,j["max"].as_u64().unwrap() as usize){
                Ok(())=>json!({"valid":true}),Err(e)=>{let text=e.to_string();if let SqlCompileError::SchemaIdentifierTooLong{object,identifier,actual_bytes,max_bytes}=e{json!({"error":text,"object":object,"identifier":identifier,"actual":actual_bytes,"max":max_bytes})}else{panic!("unexpected error")}}
            }
        },_=>panic!("unknown fixture")
    }
}
fn main(){std::panic::set_hook(Box::new(|_|{}));for line in io::stdin().lock().lines(){let j=serde_json::from_str(&line.unwrap()).unwrap();let out=match std::panic::catch_unwind(||evaluate(j)){Ok(out)=>out,Err(p)=>{let text=p.downcast_ref::<String>().map(String::as_str).or_else(||p.downcast_ref::<&str>().copied()).unwrap_or("");assert!(text.contains("SQL identifier byte limit is too small"));json!({"panic":"identifier limit"})}};println!("{}",out);}}
