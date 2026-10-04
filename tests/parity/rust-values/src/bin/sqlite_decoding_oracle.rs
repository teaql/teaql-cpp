use std::io::{self,BufRead};
use rusqlite::Connection;
use serde_json::{json,Value as Json};
use teaql_core::Value;
use teaql_provider_sqlite::{SqliteMutationExecutor,MutationExecutorError};
use teaql_sql::CompiledQuery;
fn value(v:&Value)->Json {
    match v {
        Value::Null=>json!({"kind":"null"}),
        Value::Bool(v)=>json!({"kind":"bool","value":v}),
        Value::I64(v)=>json!({"kind":"i64","value":v.to_string()}),
        Value::F64(v)=>json!({"kind":"f64","value":if v.is_infinite(){json!(if *v<0.0 {"-inf"}else{"inf"})}else{json!(v)}}),
        Value::Text(v)=>json!({"kind":"text","value":v}),
        Value::Decimal(v)=>json!({"kind":"decimal","value":v.to_string()}),
        Value::Date(v)=>json!({"kind":"date","value":v.to_string()}),
        Value::Timestamp(v)=>json!({"kind":"timestamp","value":v.0.to_string()}),
        Value::Json(v)=>json!({"kind":"json","value":v}),
        _=>panic!("unexpected row value"),
    }
}
fn evaluate(input:Json)->Json {
    let connection=Connection::open_in_memory().unwrap();
    let declaration=input["decl"].as_str().map(|s|format!("\"{}\"",s.replace('"',"\"\""))).unwrap_or_default();
    connection.execute_batch(&format!("CREATE TABLE data(v {declaration}); INSERT INTO data VALUES({});",input["sql_value"].as_str().unwrap())).unwrap();
    let provider=SqliteMutationExecutor::from_connection(connection);
    let query=CompiledQuery{log_context:Default::default(),sql:input["query"].as_str().unwrap_or("SELECT v FROM data").into(),params:vec![],comment:None};
    match provider.fetch_all_compact(&query) {
        Ok(rows)=>json!({"rows":rows.iter().map(|row|json!({"keys":row.keys().collect::<Vec<_>>(),"values":row.values().map(value).collect::<Vec<_>>()})).collect::<Vec<_>>()}),
        Err(error)=>json!({"error":error.to_string(),"kind":match error {MutationExecutorError::Bind(_)=>"Bind",MutationExecutorError::UnsupportedColumnType(_)=>"UnsupportedColumnType",_=>"Sqlite"}}),
    }
}
fn main(){for line in io::stdin().lock().lines(){println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap()));}}
