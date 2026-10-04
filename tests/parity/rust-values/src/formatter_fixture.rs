#![allow(unused_imports)]
include!(concat!(env!("CARGO_MANIFEST_DIR"),"/../../../.reference/teaql-rs/teaql-runtime/src/log_formatter.rs"));
pub fn bounds(content:&str,max:usize)->(String,String){(truncate_sensitive_sql_log(content,max),truncate_sensitive_audit_log(content,max))}

pub fn sensitive_sql(trace:&[TraceNode],entry:&SqlLogEntry){LogManager::write_sensitive_sql_log(trace,entry);}
