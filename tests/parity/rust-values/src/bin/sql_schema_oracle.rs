use std::io::{self,BufRead};
use teaql_sql::*;
use serde_json::{Value as J,json};
#[allow(dead_code)]
#[path="../fixture_value.rs"] mod fixture_value;
#[allow(dead_code)]
#[path="../fixture_sql.rs"] mod fixture_sql;
use fixture_sql::{FixtureDialect,entity,expression};
fn formatted<T:std::fmt::Debug>(value:&T)->J{
    json!({"compact":format!("{value:?}"),"pretty":format!("{value:#?}"),
        "lower_hex":format!("{value:x?}"),"upper_hex":format!("{value:X?}"),
        "pretty_lower_hex":format!("{value:#x?}"),"pretty_upper_hex":format!("{value:#X?}"),
        "width_precision":format!("{value:>40.2?}"),"fancy":format!("{value:🧊^+18.3?}"),"zero_pad":format!("{value:+012?}")})
}
fn evaluate(j:J)->J{
    match j["mode"].as_str().unwrap(){
        "in_direct"=>{
            let dialect=FixtureDialect{numbered:j["numbered"].as_bool().unwrap_or(false)};
            let e=entity(&j["entity"]);let left=expression(&j["left"]);let right=expression(&j["right"]);
            let op=[teaql_core::BinaryOp::Eq,teaql_core::BinaryOp::Ne,teaql_core::BinaryOp::Gt,teaql_core::BinaryOp::Gte,teaql_core::BinaryOp::Lt,teaql_core::BinaryOp::Lte,teaql_core::BinaryOp::Like,teaql_core::BinaryOp::NotLike,teaql_core::BinaryOp::In,teaql_core::BinaryOp::NotIn,teaql_core::BinaryOp::InLarge,teaql_core::BinaryOp::NotInLarge][j["op"].as_u64().unwrap() as usize];
            let mut params=SqlBindings::new();
            let result=std::panic::catch_unwind(std::panic::AssertUnwindSafe(||dialect.compile_in(&e,&left,op,&right,&mut params)));
            let mut output=match result{
                Ok(Ok(sql))=>json!({"sql":sql}),Ok(Err(error))=>json!({"error":error.to_string()}),
                Err(p)=>{let message=p.downcast_ref::<String>().map(String::as_str).or_else(||p.downcast_ref::<&str>().copied()).unwrap_or("");assert!(message.contains("unreachable"));json!({"panic":"operator"})}
            };
            output["values"]=json!(params.iter().map(teaql_core::Value::to_json_value).collect::<Vec<_>>());
            output["policies"]=json!(params.log_context(dialect.kind()).parameter_policies.iter().map(|policy|match policy{teaql_data_service::SqlParameterLogPolicy::Unknown=>0,teaql_data_service::SqlParameterLogPolicy::Plain=>1,teaql_data_service::SqlParameterLogPolicy::Masked=>2,teaql_data_service::SqlParameterLogPolicy::Credential=>3}).collect::<Vec<_>>());
            output
        },
        "quote"=>json!({"quoted":quote_identifier_if_needed(j["identifier"].as_str().unwrap(),char::from_u32(j["quote_code"].as_u64().unwrap() as u32).unwrap())}),
        "storage"=>json!({
            "length":storage_length_covers(j["expected_length"].as_u64().unwrap() as u32,j["actual_length"].as_u64().map(|v|v as u32)),
            "numeric":storage_numeric_covers(j["expected_precision"].as_u64().unwrap() as u32,j["expected_scale"].as_u64().unwrap() as u32,j["actual_precision"].as_u64().map(|v|v as u32),j["actual_scale"].as_u64().map(|v|v as u32))
        }),
        "foreign_key"=>json!({"name":schema_foreign_key_name(j["source_table"].as_str().unwrap(),j["source_column"].as_str().unwrap(),j["target_table"].as_str().unwrap(),j["target_column"].as_str().unwrap(),j["max"].as_u64().unwrap() as usize)}),
        "indexes"=>{
            let e=entity(&j["entity"]);let specs=schema_index_specs(&e,j["max"].as_u64().map(|v|v as usize));let d=FixtureDialect{numbered:false};
            json!({"specs":specs.iter().map(|s|json!({"name":s.name,"table":s.table,"columns":s.columns,"unique":s.unique,"debug":formatted(s),"clone_equal":*s==s.clone()})).collect::<Vec<_>>(),"sql":d.schema_indexes_sqls(&e).unwrap()})
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
