use std::io::{self, BufRead};
use teaql_core::{Value, Decimal, DataType};
use serde_json::{json, Value as Json};
#[derive(Clone)]
struct IdEntity(Value);
impl teaql_core::TeaqlEntity for IdEntity {
    const ENTITY_NAME: &'static str = "IdEntity";
    fn entity_descriptor() -> teaql_core::EntityDescriptor { teaql_core::EntityDescriptor::new(Self::ENTITY_NAME) }
}
impl teaql_core::Entity for IdEntity {
    fn from_compact_row(_: teaql_core::CompactRow) -> Result<Self,teaql_core::EntityError> { unreachable!() }
    fn into_values(self) -> teaql_core::MutationValues { Default::default() }
}
impl teaql_core::IdentifiableEntity for IdEntity {
    fn id_value(&self) -> Value { self.0.clone() }
}
fn evaluate(input: Json) -> Json {
    let kind = input["kind"].as_str().unwrap();
    let data = &input["value"];
    let value = match kind {
        "text" => Value::Text(data.as_str().unwrap().to_owned()),
        "i64" => Value::I64(data.as_str().unwrap().parse().unwrap()),
        "u64" => Value::U64(data.as_str().unwrap().parse().unwrap()),
        "f64" => Value::F64(data.as_str().unwrap().parse().unwrap()),
        "f64_bits" => Value::F64(f64::from_bits(u64::from_str_radix(data.as_str().unwrap(),16).unwrap())),
        "json_bits" => Value::Json(Json::from(f64::from_bits(u64::from_str_radix(data.as_str().unwrap(),16).unwrap()))),
        "decimal" => match data.as_str().unwrap().parse::<Decimal>() { Ok(d) => Value::Decimal(d), Err(_) => return json!({"error":"decimal"}) },
        "json" => match serde_json::from_str::<Json>(data.as_str().unwrap()) { Ok(v) => Value::Json(v), Err(_) => return json!({"error":"json"}) },
        "date" => match Value::Text(data.as_str().unwrap().to_owned()).try_date() { Some(v) => Value::Date(v), None => return json!({"error":"date"}) },
        "timestamp" => Value::Timestamp(teaql_core::time::Timestamp(data.as_str().unwrap().parse().unwrap())),
        "null" => Value::Null,
        "typed_null" => Value::TypedNull(DataType::Text),
        "bool" => Value::Bool(data.as_bool().unwrap()),
        "object" => Value::Object(Default::default()),
        "list" => Value::List(Default::default()),
        _ => panic!("unknown fixture kind"),
    };
    let json_value = value.to_json_value();
    if input["format"] == true {
        let list = teaql_core::SmartList::new(vec![IdEntity(value)]);
        return json!({"id_key":list.map_by_id().into_keys().next().unwrap(),"json_text":json_value.to_string()});
    }
    json!({"i64":value.try_i64().map(|v|v.to_string()), "u64":value.try_u64().map(|v|v.to_string()),
        "decimal":value.try_decimal().map(|v|v.to_string()), "f64":value.try_f64().map(Json::from),
        "date":value.try_date().map(|v|v.to_string()), "timestamp":value.try_timestamp().map(|v|v.0.to_string()),
        "text":value.try_text(), "bool":value.try_bool(), "json":json_value})
}
fn main() {
    for line in io::stdin().lock().lines() {
        let input: Json = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", evaluate(input));
    }
}
