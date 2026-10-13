use std::io::{self,BufRead};
use serde_json::{json,Value as Json};
use teaql_core::{DataType,EntityDescriptor,PropertyDescriptor};
use teaql_sql::{CompiledQuery,SqlBindings,DatabaseKind,SqlCompileError,render_sql_with,render_sql_value};
#[path="../../../rust_values/src/fixture_value.rs"] mod fixture_value;
use fixture_value::value;
fn datatype(index:u64)->DataType{[DataType::Bool,DataType::I64,DataType::U64,DataType::F64,DataType::Decimal,DataType::Text,DataType::LargeText,DataType::Json,DataType::Date,DataType::Timestamp][index as usize]}
fn outcome(value:Result<String,&str>)->Json {match value {Ok(v)=>json!({"ok":v}),Err(e)=>json!({"error":e})}}
fn evaluate(input:Json)->Json {
    match input["kind"].as_str().unwrap() {
        "value"=>outcome(render_sql_value(&value(&input["value"]),DatabaseKind::Sqlite)),
        "render"=>{
            let query=CompiledQuery{log_context:Default::default(),sql:input["sql"].as_str().unwrap().into(),params:input["params"].as_array().unwrap().iter().map(value).collect(),comment:input["comment"].as_str().map(str::to_owned)};
            let mut called=Vec::new();let rendered=render_sql_with(&query.sql_with_comment(),DatabaseKind::Sqlite,query.params.len(),|i| {called.push(i);if input["literal_failure"].as_u64()==Some(i as u64){Err("redacted literal omitted")}else{render_sql_value(&query.params[i],DatabaseKind::Sqlite)}});
            json!({"with_comment":query.sql_with_comment(),"debug":query.debug_sql(DatabaseKind::Sqlite),"render":outcome(rendered),"called":called})
        },
        "bindings"=>{
            let mut entity=EntityDescriptor::new("Record");
            for property in input["properties"].as_array().unwrap(){entity=entity.property(PropertyDescriptor::new(property["name"].as_str().unwrap(),DataType::Text).column_name(property["column"].as_str().unwrap()));}
            entity.audit_mask_fields_declared=input["declared"].as_bool().unwrap();entity.audit_mask_fields=input["masked"].as_array().unwrap().iter().map(|v|v.as_str().unwrap().to_owned()).collect();
            let mut bindings=SqlBindings::new();for item in input["values"].as_array().unwrap(){if let Some(field)=item["field"].as_str(){bindings.push_field(&entity,field,value(&item["value"]));}else{bindings.push(value(&item["value"]));}}
            if input["untrusted"]==true {bindings.mark_untrusted_sql();}
            let log=bindings.log_context(DatabaseKind::Sqlite);let policies:Vec<_>=log.parameter_policies.iter().map(|p|match p{teaql_data_service::SqlParameterLogPolicy::Unknown=>0,teaql_data_service::SqlParameterLogPolicy::Plain=>1,teaql_data_service::SqlParameterLogPolicy::Masked=>2,teaql_data_service::SqlParameterLogPolicy::Credential=>3}).collect();
            json!({"policies":policies,"generated":log.generated_sql,"database":log.database_kind,"values":bindings.into_values().iter().map(|v|v.to_json_value()).collect::<Vec<_>>()})
        },
        "error"=>{
            let name=input["name"].as_str().unwrap_or("").to_owned();let e=match input["code"].as_u64().unwrap(){
                0=>SqlCompileError::UnknownEntity(name),1=>SqlCompileError::UnknownField(name),2=>SqlCompileError::EmptyInList,3=>SqlCompileError::MissingIdProperty(name),4=>SqlCompileError::MissingVersionProperty(name),5=>SqlCompileError::EmptyMutation(name),6=>SqlCompileError::InvalidRecoverVersion(input["version"].as_str().unwrap().parse().unwrap()),7=>SqlCompileError::UnsupportedSchemaType(datatype(input["data_type"].as_u64().unwrap())),8=>SqlCompileError::InvalidSchemaShape{property:name,reason:input["reason"].as_str().unwrap().into()},9=>SqlCompileError::SchemaIdentifierTooLong{object:name,identifier:input["identifier"].as_str().unwrap().into(),actual_bytes:input["actual"].as_u64().unwrap() as usize,max_bytes:input["max"].as_u64().unwrap() as usize},10=>SqlCompileError::InvalidFunctionArguments(name),11=>SqlCompileError::InvalidSubQueryOperator(name),_=>panic!()};json!({"text":e.to_string()})
        },_=>panic!("unknown fixture kind")
    }
}
fn main(){for line in io::stdin().lock().lines(){println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap()));}}
