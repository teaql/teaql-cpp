//! Actual pinned SQL model calls and unchanged private-scope binding source.
use serde_json::{Value as Json, json};
use std::io::{self, BufRead};
use teaql_core::{DataType, EntityDescriptor, Expr, PropertyDescriptor, SelectQuery, Value};
use teaql_data_service::{SqlExecutionOutcome, SqlIntentRedactions, SqlParameterLogPolicy as Policy,
    SqlProjectionState};
use teaql_sql::{CompiledQuery, DatabaseKind, SqlCompileError};

#[path = "../fixture_value.rs"]
mod fixture_value;
// Keep compiler-only scope access while compiling the complete unchanged source.
#[path = "../../../../../.reference/teaql-rs/teaql-sql/src/bindings.rs"]
mod original_bindings;

fn formatted<T: std::fmt::Debug>(value: &T) -> Json {
    json!({"compact":format!("{value:?}"),"pretty":format!("{value:#?}"),
        "lower_hex":format!("{value:x?}"),"upper_hex":format!("{value:X?}"),
        "pretty_lower_hex":format!("{value:#x?}"),"pretty_upper_hex":format!("{value:#X?}"),
        "width_precision":format!("{value:>40.2?}"),"fancy":format!("{value:🧊^+18.3?}"),
        "zero_pad":format!("{value:+012?}")})
}

fn policy(index: u64) -> Policy {
    [Policy::Unknown, Policy::Plain, Policy::Masked, Policy::Credential][index as usize]
}

fn errors(input: &Json) -> Vec<SqlCompileError> {
    let name = input["text"].as_str().unwrap();
    let version = input["version"].as_str().unwrap().parse().unwrap();
    let types = [DataType::Bool, DataType::I64, DataType::U64, DataType::F64, DataType::Decimal,
        DataType::Text, DataType::LargeText, DataType::Json, DataType::Date, DataType::Timestamp];
    vec![SqlCompileError::UnknownEntity(name.into()), SqlCompileError::UnknownField(name.into()),
        SqlCompileError::EmptyInList, SqlCompileError::MissingIdProperty(name.into()),
        SqlCompileError::MissingVersionProperty(name.into()), SqlCompileError::EmptyMutation(name.into()),
        SqlCompileError::InvalidRecoverVersion(version),
        SqlCompileError::UnsupportedSchemaType(types[input["type"].as_u64().unwrap() as usize]),
        SqlCompileError::InvalidSchemaShape {property:name.into(), reason:format!("reason-{name}")},
        SqlCompileError::SchemaIdentifierTooLong {object:name.into(),identifier:format!("id-{name}"),
            actual_bytes:input["bytes"].as_u64().unwrap() as usize,max_bytes:63},
        SqlCompileError::InvalidFunctionArguments(name.into()),
        SqlCompileError::InvalidSubQueryOperator(name.into())]
}

fn evaluate(input: Json) -> Json {
    let mut bindings = original_bindings::SqlBindings::new();
    let initial = formatted(&bindings);
    bindings.scope = input["scope"].as_u64().map(policy);
    let entity = EntityDescriptor::new("Device")
        .audit_mask_fields(vec!["masked".into()])
        .property(PropertyDescriptor::new("plain", DataType::Text))
        .property(PropertyDescriptor::new("masked", DataType::Text))
        .property(PropertyDescriptor::new("token", DataType::Text).column_name("access_token"));
    for item in input["params"].as_array().unwrap() {
        let value = fixture_value::value(item);
        if let Some(field) = item["field"].as_str() {
            bindings.push_field(&entity, field, value);
        } else {
            bindings.push(value);
        }
    }
    if input["untrusted"].as_bool().unwrap() { bindings.mark_untrusted_sql(); }
    let binding_debug = formatted(&bindings);
    let deref_debug: Vec<_> = bindings.iter().map(formatted).collect();
    let mut cloned = bindings.clone();
    cloned.push(Value::Text("clone-only".into()));
    let clone_debug = formatted(&cloned);
    let context = bindings.log_context(DatabaseKind::Sqlite);
    let mut query = CompiledQuery {log_context:context,sql:input["sql"].as_str().unwrap().into(),
        params:bindings.into_values(),comment:input["comment"].as_str().map(str::to_owned)};
    let before = formatted(&query);
    query.log_context.execution_outcome = input["outcome"].as_u64().map(|v|
        [SqlExecutionOutcome::Success,SqlExecutionOutcome::Failure,SqlExecutionOutcome::Cancelled][v as usize]);
    query.log_context.mode = input["mode"].as_str().map(str::to_owned);
    query.log_context.omission_reason = input["omission"].as_str().map(str::to_owned);
    query.log_context.masked_parameters = input["masked"].as_array().unwrap().iter().map(|v|v.as_bool().unwrap()).collect();
    let intent = SelectQuery::new("Device").comment("intent-comment")
        .filter(Expr::eq("token", "private intent must not appear in diagnostics"));
    query.log_context.intent_redactions = SqlIntentRedactions::from_unclassified_query(&intent);
    query.log_context.projection_state = SqlProjectionState::new("private projection payload");
    let clone_equal = query == query.clone();
    let debug_sql = query.debug_sql(DatabaseKind::Sqlite);
    let sql_with_comment = query.sql_with_comment();
    let error_values = errors(&input);
    json!({"initial":initial,"bindings":binding_debug,"clone":clone_debug,"deref":deref_debug,
        "before":before,"query":formatted(&query),"clone_equal":clone_equal,
        "sql_with_comment":sql_with_comment,"debug_sql":debug_sql,
        "errors":error_values.iter().map(formatted).collect::<Vec<_>>(),
        "error_display":error_values.iter().map(ToString::to_string).collect::<Vec<_>>(),
        "error_source_none":error_values.iter().map(|v|std::error::Error::source(v).is_none()).collect::<Vec<_>>(),
        "error_clone_equal":error_values.iter().map(|v|*v==v.clone()).collect::<Vec<_>>(),
        "error_matrix":error_values.iter().map(|a|error_values.iter().map(|b|a==b).collect::<Vec<_>>()).collect::<Vec<_>>(),
        "kinds":[formatted(&DatabaseKind::PostgreSql),formatted(&DatabaseKind::Sqlite),formatted(&DatabaseKind::MySql)]})
}

fn main() {
    for line in io::stdin().lock().lines() {
        println!("{}", evaluate(serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
