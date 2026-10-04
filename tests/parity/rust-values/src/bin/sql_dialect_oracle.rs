use std::io::{self,BufRead};
use teaql_core::*;
use teaql_sql::*;
use serde_json::{Value as J,json};
#[path="../fixture_value.rs"] mod fixture_value;
#[path="../fixture_sql.rs"] mod fixture_sql;
use fixture_sql::{FixtureDialect,entity,expression,query,uint};
fn evaluate(j:J)->J{
    let mode=j["mode"].as_str().unwrap();let d=FixtureDialect{numbered:j["numbered"].as_bool().unwrap_or(false)};
    if mode=="identifier"{let name=j["name"].as_str().unwrap();return json!({"quoted":quote_identifier_if_needed(name,j["quote"].as_str().unwrap().chars().next().unwrap()),"bounded":bounded_sql_identifier(name,j["max"].as_u64().unwrap() as usize)});}
    if mode=="coverage"{return json!({"length":storage_length_covers(j["length"].as_u64().unwrap() as u32,uint(&j,"actual_length")),"numeric":storage_numeric_covers(j["precision"].as_u64().unwrap() as u32,j["scale"].as_u64().unwrap() as u32,uint(&j,"actual_precision"),uint(&j,"actual_scale"))});}
    let e=entity(&j["entity"]);let mut bindings=SqlBindings::new();let mut out=json!({});
    let result=match mode{
        "expr"=>d.compile_expr(&e,&expression(&j["expr"]),&mut bindings),
        "select"=>{let q=query(&j["query"]);let result=d.compile_select_sql(&e,&q,&mut bindings);if result.is_ok(){let full=d.compile_select(&e,&q).unwrap();out["with_comment"]=json!(full.sql_with_comment());out["comment"]=json!(full.comment);assert_eq!(&full.sql,result.as_ref().unwrap());}result},
        "create"=>d.compile_create_table(&e),"add"=>d.compile_add_column(&e,&e.properties[0]),_=>panic!("unknown mode")
    };
    match result{Ok(sql)=>out["sql"]=json!(sql),Err(err)=>out["error"]=json!(err.to_string())};
    if let Some(expr)=j.get("after"){out["after"]=json!(d.compile_expr(&e,&expression(expr),&mut bindings).unwrap());}
    let log=bindings.log_context(d.kind());out["policies"]=json!(log.parameter_policies.iter().map(|p|match p{teaql_data_service::SqlParameterLogPolicy::Unknown=>0,teaql_data_service::SqlParameterLogPolicy::Plain=>1,teaql_data_service::SqlParameterLogPolicy::Masked=>2,teaql_data_service::SqlParameterLogPolicy::Credential=>3}).collect::<Vec<_>>());out["generated"]=json!(log.generated_sql);out["values"]=json!(bindings.into_values().iter().map(Value::to_json_value).collect::<Vec<_>>());out
}
fn main(){for line in io::stdin().lock().lines(){println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap()));}}
