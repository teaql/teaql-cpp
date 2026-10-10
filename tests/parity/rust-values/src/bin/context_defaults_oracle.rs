//! Observe original public Context defaults with independent system-clock brackets.
use serde_json::{json, Value};
use std::{io::{self,BufRead}, time::{SystemTime,UNIX_EPOCH}};
use teaql_runtime::{Language,RuntimeModule,UserContext};
use teaql_core::EntityDescriptor;
fn evaluate(input:Value)->Value {
    let before=SystemTime::now().duration_since(UNIX_EPOCH).unwrap_or_default().as_micros();
    let mut user=UserContext::new();
    let after=SystemTime::now().duration_since(UNIX_EPOCH).unwrap_or_default().as_micros();
    let identifier=user.user_identifier().unwrap();
    let tail=identifier.rsplit_once("@pid-").unwrap().1;
    let (pid,thread)=tail.split_once(".tid-").unwrap();
    let parts=user.trace_id().split('-').collect::<Vec<_>>();
    let counter=u128::from_str_radix(parts[3],16).unwrap();
    let options=user.sql_log_options();
    let mut out=json!({"trace_clock_is_microseconds":counter>=before && counter<=after,
        "trace_shape":parts.len()==4 && parts[0]=="req" && parts[1]==pid && parts[2]==thread,
        "identifier_pid":pid.parse::<u32>().unwrap()==std::process::id(),
        "identifier_thread_numeric":thread.parse::<u64>().is_ok(),
        "timezone":user.timezone(),"language_english":user.language()==Language::English,
        "select_log":options.select,"mutation_log":options.mutation,
        "empty_sql_logs":user.sql_logs().is_empty(),"empty_graphs":user.initial_graphs().is_empty()&&user.root_graphs().is_empty(),
        "empty_entities":user.all_entities().is_empty(),"missing_service":!user.has_entity_data_service("Unknown"),
        "missing_checker":!user.has_checker("Unknown"),"noop_telemetry":user.runtime_telemetry().is_noop(),
        "empty_evidence":user.last_fix_evidence().is_empty(),"missing_local":user.local("absent").is_none()});
    user.set_user_identifier_option(input["identifier"].as_str().map(str::to_owned));
    user.set_timezone(input["timezone"].as_str().unwrap());
    user.set_trace_id(input["trace"].as_str().unwrap());
    out["set_identifier"]=json!(user.user_identifier());
    out["set_timezone"]=json!(user.timezone());
    out["set_trace"]=json!(user.trace_id());
    user=user.with_module(RuntimeModule::new().descriptor(EntityDescriptor::new("ContextAssembly")).language(Language::Japanese));
    out["module_entity"]=json!(user.entity("ContextAssembly").unwrap().name);
    out["module_language"]=json!(user.language()==Language::Japanese);
    out["module_service"]=json!(user.has_entity_data_service("ContextAssembly"));
    out
}
fn main(){for line in io::stdin().lock().lines(){println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap()));}}
