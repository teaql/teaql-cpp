use serde_json::Value as Json;
use teaql_core::{Value,DataType};
pub fn value(input:&Json)->Value {
    let data=&input["value"];
    match input["kind"].as_str().unwrap() {
        "null"=>Value::Null,"typed_null"=>Value::TypedNull(DataType::Text),
        "text"=>Value::Text(data.as_str().unwrap().into()),"bool"=>Value::Bool(data.as_bool().unwrap()),
        "i64"=>Value::I64(data.as_str().unwrap().parse().unwrap()),"u64"=>Value::U64(data.as_str().unwrap().parse().unwrap()),
        "f64"=>Value::F64(data.as_str().unwrap().parse().unwrap()),"decimal"=>Value::Decimal(data.as_str().unwrap().parse().unwrap()),
        "date"=>Value::Date(Value::Text(data.as_str().unwrap().into()).try_date().unwrap()),
        "timestamp"=>Value::Timestamp(teaql_core::time::Timestamp(data.as_str().unwrap().parse().unwrap())),
        "json"=>Value::Json(data.clone()),
        "list"=>Value::List(data.as_array().unwrap().iter().map(value).collect()),
        "object"=>Value::Object(data.as_object().unwrap().iter().map(|(k,v)|(k.clone(),value(v))).collect()),
        _=>panic!("unknown value fixture")
    }
}
