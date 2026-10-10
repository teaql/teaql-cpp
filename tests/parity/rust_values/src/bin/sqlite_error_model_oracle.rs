//! Public structured driver errors and the original provider's From/Debug/Error.
use std::io::{self, BufRead};
use serde_json::{json, Value as Json};
use rusqlite::{Error, types::Type};
use teaql_provider_sqlite::MutationExecutorError;
use std::ffi::{CString,OsString};
use std::os::unix::ffi::OsStringExt;
use std::path::PathBuf;

#[derive(Debug)]
struct FixtureCause { message:String, child:Option<Box<FixtureCause>> }
impl std::fmt::Display for FixtureCause {
    fn fmt(&self,writer:&mut std::fmt::Formatter<'_>)->std::fmt::Result { writer.write_str(&self.message) }
}
impl std::error::Error for FixtureCause {
    fn source(&self)->Option<&(dyn std::error::Error+'static)> {
        self.child.as_deref().map(|value|value as &(dyn std::error::Error+'static))
    }
}
fn boxed_cause(input:&Json)->Box<FixtureCause> {
    let mut next=None;
    for message in input["messages"].as_array().unwrap().iter().rev() {
        next=Some(Box::new(FixtureCause {message:message.as_str().unwrap().into(),child:next}));
    }
    next.unwrap()
}

fn formatted<T: std::fmt::Debug>(value: &T) -> Json {
    json!({"compact":format!("{value:?}"),"pretty":format!("{value:#?}"),
        "lower_hex":format!("{value:x?}"),"upper_hex":format!("{value:X?}"),
        "pretty_lower_hex":format!("{value:#x?}"),"pretty_upper_hex":format!("{value:#X?}"),
        "width_precision":format!("{value:>40.2?}"),"fancy":format!("{value:🧊^+18.3?}"),
        "zero_pad":format!("{value:+012?}")})
}

fn make_error(input: &Json) -> Error {
    let integer = |name: &str| input[name].as_str().unwrap().parse::<usize>().unwrap();
    let native = || {
        let mut error=libsqlite3_sys::Error::new(input["code"].as_i64().unwrap() as i32);
        if let Some(category)=input.get("category") {
            error.code=[
                libsqlite3_sys::ErrorCode::InternalMalfunction,
                libsqlite3_sys::ErrorCode::PermissionDenied,
                libsqlite3_sys::ErrorCode::OperationAborted,
                libsqlite3_sys::ErrorCode::DatabaseBusy,
                libsqlite3_sys::ErrorCode::DatabaseLocked,
                libsqlite3_sys::ErrorCode::OutOfMemory,
                libsqlite3_sys::ErrorCode::ReadOnly,
                libsqlite3_sys::ErrorCode::OperationInterrupted,
                libsqlite3_sys::ErrorCode::SystemIoFailure,
                libsqlite3_sys::ErrorCode::DatabaseCorrupt,
                libsqlite3_sys::ErrorCode::NotFound,
                libsqlite3_sys::ErrorCode::DiskFull,
                libsqlite3_sys::ErrorCode::CannotOpen,
                libsqlite3_sys::ErrorCode::FileLockingProtocolFailed,
                libsqlite3_sys::ErrorCode::SchemaChanged,
                libsqlite3_sys::ErrorCode::TooBig,
                libsqlite3_sys::ErrorCode::ConstraintViolation,
                libsqlite3_sys::ErrorCode::TypeMismatch,
                libsqlite3_sys::ErrorCode::ApiMisuse,
                libsqlite3_sys::ErrorCode::NoLargeFileSupport,
                libsqlite3_sys::ErrorCode::AuthorizationForStatementDenied,
                libsqlite3_sys::ErrorCode::ParameterOutOfRange,
                libsqlite3_sys::ErrorCode::NotADatabase,
                libsqlite3_sys::ErrorCode::Unknown,
            ][category.as_u64().unwrap() as usize];
        }
        error
    };
    match input["kind"].as_str().unwrap() {
        "native" => Error::SqliteFailure(native(), input["message"].as_str().map(str::to_owned)),
        "input" => Error::SqlInputError {error:native(),
            msg:input["message"].as_str().unwrap().into(), sql:input["sql"].as_str().unwrap().into(),
            offset:input["offset"].as_i64().unwrap() as i32},
        "count" => Error::InvalidParameterCount(integer("given"),integer("expected")),
        "multiple" => Error::MultipleStatement,
        "returned" => Error::ExecuteReturnedResults,
        "index" => Error::InvalidColumnIndex(integer("index")),
        "type" => Error::InvalidColumnType(integer("index"),input["name"].as_str().unwrap().into(),
            [Type::Null,Type::Integer,Type::Real,Type::Text,Type::Blob][input["type"].as_u64().unwrap() as usize]),
        "utf8" => Error::Utf8Error(integer("index"), std::str::from_utf8(&raw_bytes(input)).unwrap_err()),
        "nul" => Error::NulError(CString::new(raw_bytes(input)).unwrap_err()),
        "path" => Error::InvalidPath(PathBuf::from(OsString::from_vec(raw_bytes(input)))),
        "simple" => match input["variant"].as_u64().unwrap() {
            0=>Error::SqliteSingleThreadedMode,
            1=>Error::IntegralValueOutOfRange(integer("index"),input["value"].as_str().unwrap().parse().unwrap()),
            2=>Error::InvalidParameterName(input["name"].as_str().unwrap().into()),
            3=>Error::QueryReturnedNoRows,
            4=>Error::QueryReturnedMoreThanOneRow,
            5=>Error::InvalidColumnName(input["name"].as_str().unwrap().into()),
            6=>Error::StatementChangedRows(integer("index")),
            7=>Error::InvalidFunctionParameterType(integer("index"),
                [Type::Null,Type::Integer,Type::Real,Type::Text,Type::Blob][input["type"].as_u64().unwrap() as usize]),
            8=>Error::InvalidQuery,
            9=>Error::UnwindingPanic,
            10=>Error::GetAuxWrongType,
            11=>Error::InvalidDatabaseIndex(integer("index")),
            _=>panic!("unknown enabled alternative"),
        },
        "boxed" => match input["category"].as_u64().unwrap() {
            0=>Error::FromSqlConversionFailure(integer("index"),
                [Type::Null,Type::Integer,Type::Real,Type::Text,Type::Blob][input["type"].as_u64().unwrap() as usize], boxed_cause(input)),
            1=>Error::ToSqlConversionFailure(boxed_cause(input)),
            2=>Error::UserFunctionError(boxed_cause(input)),
            _=>panic!("unknown enabled boxed alternative"),
        },
        _ => panic!("unknown structured fixture"),
    }
}

fn raw_bytes(input:&Json)->Vec<u8> {
    input["bytes"].as_array().unwrap().iter().map(|v|v.as_u64().unwrap() as u8).collect()
}

fn evaluate(input: Json) -> Json {
    if let Some(outer) = input["outer"].as_str() {
        let text = input["text"].as_str().unwrap();
        let wrapper = match outer {
            "unsupported" => MutationExecutorError::UnsupportedValue(match text {
                "" => "", "list" => "list", "object" => "object", "unknown" => "unknown",
                _ => panic!("unsupported-value text is a static source payload"),
            }),
            "column" => MutationExecutorError::UnsupportedColumnType(text.into()),
            "bind" => MutationExecutorError::Bind(text.into()),
            "lock" => MutationExecutorError::Lock(text.into()),
            "compile" => MutationExecutorError::from(teaql_sql::SqlCompileError::UnknownEntity(text.into())),
            "shape" => MutationExecutorError::from(teaql_sql::SqlCompileError::InvalidSchemaShape {
                property:text.into(),reason:format!("reason:{text}")}),
            _ => panic!("unknown outer fixture"),
        };
        return json!({"driver":null,"wrapper_debug":formatted(&wrapper),
            "wrapper_display":wrapper.to_string(),
            "wrapper_source_none":std::error::Error::source(&wrapper).is_none()});
    }
    let error = make_error(&input);
    let copied_input = make_error(&input);
    let source = std::error::Error::source(&error);
    let mut driver = json!({"debug":formatted(&error),"display":error.to_string(),
        "self_equal":error == error,"same_payload_equal":error == copied_input,
        "source_some":source.is_some(),"source_display":source.map(ToString::to_string),
        "source_source_none":source.map(|e|e.source().is_none()),
        "native":error.sqlite_error().map(formatted),
        "native_code":error.sqlite_error_code().map(|c|format!("{c:?}")),
        "native_extended":error.sqlite_extended_error_code()});
    if let Some(other)=input.get("changed") {
        driver["changed_equal"]=json!(error==make_error(other));
    }
    if input["kind"].as_str()==Some("boxed") {
        let mut chain=Vec::new();
        let mut cursor=std::error::Error::source(&error);
        while let Some(cause)=cursor {
            chain.push(json!({"debug":formatted(&cause),"display":cause.to_string()}));
            cursor=cause.source();
        }
        driver["cause_chain"]=json!(chain);
    }
    let wrapper = MutationExecutorError::from(error);
    json!({"driver":driver,"wrapper_debug":formatted(&wrapper),
        "wrapper_display":wrapper.to_string(),
        "wrapper_source_none":std::error::Error::source(&wrapper).is_none()})
}

fn main() {
    for line in io::stdin().lock().lines() {
        println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap()));
    }
}
