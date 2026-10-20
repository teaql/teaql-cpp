use cpp_dynamic_json_core::{AuditedSave, Q, Product, ServiceRuntimeExecutor};
use serde_json::{json, Value as Json};
use std::{future::Future, io::{self,BufRead}, sync::{Arc,Mutex}, task::{Context,Poll,Wake,Waker}};
use teaql_core::{TeaqlEntity, Entity, DataType, Value, dynamic_fields::{DynamicFieldDefinitions,DynamicFieldSelection}};
use teaql_runtime::{SafeAuditEvent,SafeAuditEventSink,UserContext,RuntimeError,dynamic_fields::{DatabaseDynamicFieldsProvider,DynamicFieldsProvider}};
#[path = "../../../rust_values/src/fixture_value.rs"]
mod fixture_value;
use teaql_runtime::{MutationPolicy,MutationPolicyIdentity,MutationDecision,MutationPlan,MutationPolicyApproval};
use teaql_data_service::dynamic_fields::{DynamicFieldStore,DynamicFieldWrite};
use teaql_sql::{SqlTransaction,SqlTransactionTransport};
fn wait<T>(f: impl Future<Output=T>) -> T { struct NoWake; impl Wake for NoWake { fn wake(self:Arc<Self>){} } let mut f=Box::pin(f); let w=Waker::from(Arc::new(NoWake)); let mut c=Context::from_waker(&w); loop { if let Poll::Ready(v)=f.as_mut().poll(&mut c) { return v; } } }
struct Sink { events:Arc<Mutex<Vec<String>>>, fail:bool }
impl SafeAuditEventSink for Sink { fn on_safe_event(&self,_:&UserContext,event:&SafeAuditEvent)->Result<(),RuntimeError> { self.events.lock().unwrap().push(format!("{event:?}")); if self.fail { Err(RuntimeError::Event("fixture audit failure".into())) } else { Ok(()) } } }
struct Policy { plans:Arc<Mutex<Vec<String>>>, deny:bool }
impl MutationPolicy for Policy {
    fn identity(&self)->MutationPolicyIdentity { MutationPolicyIdentity::new("dynamic-policy","1","fingerprint") }
    fn review(&self,_:&UserContext,plan:&MutationPlan)->MutationDecision {
        self.plans.lock().unwrap().push(format!("{plan:?}"));
        if self.deny { MutationDecision::deny("DYNAMIC-DENIED","fixture policy denial",["#note"]) } else { MutationDecision::allow() }
    }
}
fn datatype(i:u64)->DataType { [DataType::Bool,DataType::I64,DataType::U64,DataType::F64,DataType::Decimal,DataType::Text,DataType::LargeText,DataType::Json,DataType::Date,DataType::Timestamp][i as usize] }
fn observe(input:&Json)->Json {
    let pool=Arc::new(Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));
    let mut user_context=wait(cpp_dynamic_json_core::service_runtime_from_pool(pool.clone())).unwrap().with_active_root("Platform",1);
    { let db=pool.lock().unwrap(); db.execute_batch(include_str!("../../../../support/dynamic_json_schema.sql")).unwrap(); db.execute_batch(include_str!("../../../../support/dynamic_json_dataset.sql")).unwrap(); }
    user_context.set_trace_id("fixture"); user_context.set_user_identifier("fixture");
    let definitions=DynamicFieldDefinitions::new("Product","v1",[("note".into(),datatype(input["type"].as_u64().unwrap_or(5)))]).unwrap();
    let provider=Arc::new(DatabaseDynamicFieldsProvider::<ServiceRuntimeExecutor>::new("profile",[definitions.clone()]).unwrap());
    user_context.set_dynamic_fields_provider(provider.clone()); wait(provider.ensure_schema(&user_context)).unwrap();
    if input.get("stored").is_some() {
        let transport=teaql_provider_sqlite::SqliteMutationExecutor::new(pool.clone());
        let tx=wait(transport.begin_sql()).unwrap();
        let writes=vec![DynamicFieldWrite { namespace:"profile".into(),definitions:definitions.clone(),owner_id:7,changes:std::collections::BTreeMap::from([("note".into(),teaql_core::dynamic_fields::DynamicFieldMutation::Set(fixture_value::value(&input["stored"])))]) }];
        wait(tx.apply_writes(&writes,&teaql_core::MutationIntent::new("seed typed durable extension").unwrap())).unwrap();
        wait(tx.commit_sql()).unwrap();
    } else {
    pool.lock().unwrap().execute_batch(r#"INSERT INTO teaql_dynamic_field_storage_v1 VALUES('profile','Product','7','note','v1','Text',0,'"PRIVATE-OLD"')"#).unwrap();
    }
    let mut entity=wait(Q::products().with_id_is(7).select_self_fields().select_dynamic_fields_with(DynamicFieldSelection::All).comment("load PRIVATE-OLD").purpose("edit durable extensions").execute_for_one(&user_context)).unwrap().unwrap();
    match input["op"].as_str().unwrap() { "delete"=>entity.delete_dynamic_field("note").unwrap(), "null"=>entity.update_dynamic_field("note",Value::Null).unwrap(), _=>entity.update_dynamic_field("note",input.get("value").map(fixture_value::value).unwrap_or_else(||Value::from("PRIVATE-NEW"))).unwrap() };
    if input["trigger"]==true { pool.lock().unwrap().execute_batch("CREATE TRIGGER deny_extension BEFORE INSERT ON teaql_dynamic_field_storage_v1 BEGIN SELECT RAISE(ABORT,'fixture failure'); END").unwrap(); }
    if input["conflict"]==true { pool.lock().unwrap().execute_batch("UPDATE product_data SET version=3 WHERE id=7").unwrap(); }
    let events=Arc::new(Mutex::new(Vec::new())); user_context.set_custom_event_sink(Sink{events:events.clone(),fail:input["audit_fail"].as_bool().unwrap()});
    let plans=Arc::new(Mutex::new(Vec::new()));
    if let Some(mode)=input["policy"].as_str() {
        if mode!="none" {
            let policy:Arc<dyn MutationPolicy>=Arc::new(Policy{plans:plans.clone(),deny:mode=="deny"});
            user_context.set_mutation_policy_registry(move|_:&str|Some(policy.clone()));
            if mode=="approved" { user_context.set_mutation_policy_approval_provider(|identity:&MutationPolicyIdentity|Some(MutationPolicyApproval::new(identity.clone(),"reviewer",std::time::UNIX_EPOCH))); }
        }
    }
    user_context.clear_sql_logs();
    let saved=wait(entity.clone().audit_as("save PRIVATE-OLD PRIVATE-NEW").save(&user_context));
    let logs:Vec<_>=user_context.sql_logs().iter().map(|log|json!({"sql":log.sql,"params":log.params.iter().map(Value::to_json_value).collect::<Vec<_>>(),"comment":log.comment,"purpose":log.purpose})).collect();
    let db=pool.lock().unwrap();
    let version:i64=db.query_row("SELECT version FROM product_data WHERE id=7",[],|r|r.get(0)).unwrap();
    let storage=db.query_row("SELECT is_null,payload FROM teaql_dynamic_field_storage_v1 WHERE owner_id='7' AND code='note'",[],|r|Ok(json!({"is_null":r.get::<_,i64>(0)?,"payload":r.get::<_,Option<String>>(1)?}))).ok();
    let mut output=json!({"error":saved.err().map(|e|e.to_string()),"events":*events.lock().unwrap(),"logs":logs,"version":version,"storage":storage,"pending":entity.has_pending_dynamic_mutations()});
    if input.get("policy").is_some() { output["plans"]=json!(*plans.lock().unwrap()); }
    output
}
fn main() { unsafe { std::env::set_var("TEAQL_LOG_ENDPOINT","off"); std::env::remove_var("TEAQL_ALLOW_SENSITIVE_PLAINTEXT_LOGS"); } for line in io::stdin().lock().lines() { println!("{}",observe(&serde_json::from_str(&line.unwrap()).unwrap())); } }
