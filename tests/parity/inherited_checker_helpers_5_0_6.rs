use serde_json::{json, Value as Json};
use std::{collections::BTreeMap, future::Future, io::{self, BufRead}, sync::{Arc, Mutex}, task::{Context, Poll, Wake, Waker}};
use teaql_core::{CompactRow, Entity, TeaqlEntity, Value, eval::LoadState};
use teaql_runtime::{CheckObjectStatus, CheckResults, InMemoryCheckerRegistry, InMemoryMetadataStore,
    JsonFieldNamingProfile, LedgerEntity, MutationDecision, MutationPlan, MutationPolicy, MutationPolicyIdentity,
    ObjectLocation, RuntimeError, TypedChecker, TypedEntityChecker, UserContext};


macro_rules! define_observer { ($function:ident, $model:ident, $checker:ident, $logic:ident, $callback:ident) => {
fn $function(input: &Json) -> Json {
use cpp_inheritance_variants_core::{$model as Note,$checker as NoteChecker,$logic as NoteCheckerLogic,Tenant,TenantChecker};
struct NoWake;
impl Wake for NoWake { fn wake(self: Arc<Self>) {} }
fn wait<T>(future: impl Future<Output=T>) -> T {
    let mut future = Box::pin(future);
    let waker = Waker::from(Arc::new(NoWake));
    let mut context = Context::from_waker(&waker);
    loop { if let Poll::Ready(result) = future.as_mut().poll(&mut context) { return result; } }
}
struct Directive { reject: bool, fix: bool }
fn changed(value: Option<Value>) -> Json {
    json!({"present":value.is_some(),"value":value.map(|v|v.to_json_value())})
}
fn state<T>(result: teaql_core::eval::EvalResult<T>, encode: impl FnOnce(T)->Json) -> Json {
    match result {
        teaql_core::eval::EvalResult::Value(value)=>json!({"state":"Value","value":encode(value)}),
        teaql_core::eval::EvalResult::Null=>json!({"state":"Null"}),
        teaql_core::eval::EvalResult::NotLoaded{failed_node,attempted_path}=>json!({"state":"NotLoaded","node":failed_node,"path":attempted_path}),
    }
}
fn view(entity: &Note) -> Json {
    json!({"tenant":entity.tenant_id(),"name":entity.name(),
        "created":entity.created_at().0,"updated":entity.updated_at().0,
        "active":entity.active(),"note":entity.note(),
        "changed_name":changed(entity.changed_name()),
        "changed_note":changed(entity.changed_note()),
        "changed_tenant":changed(entity.changed_tenant_id()),
        "eval_name":state(entity.eval_name(), |v|json!(v)),"eval_note":state(entity.eval_note(), |v|json!(v)),
        "eval_tenant":state(entity.eval_tenant_id(), |v|json!(v)),
        "loaded":[entity.is_loaded("id"),entity.is_loaded("name"),entity.is_loaded("tenant_id"),entity.is_loaded("created_at"),entity.is_loaded("updated_at"),entity.is_loaded("version"),entity.is_loaded("active"),entity.is_loaded("note"),entity.is_loaded("amount"),entity.is_loaded("status"),entity.is_loaded("reference")]})
}
struct Logic(Arc<Mutex<Vec<Json>>>);
impl NoteCheckerLogic for Logic {
    fn $callback(&self, user_context: &UserContext, entity: &mut Note, _status: CheckObjectStatus,
                         location: &ObjectLocation, results: &mut CheckResults) {
        self.0.lock().unwrap().push(json!({"view":view(entity),"before_logic":results.len(),"location":location.model_path()}));
        let directive = user_context.require_resource::<Directive>().unwrap();
        self.required_text(&entity.name(), "name", location, results);
        if directive.reject { self.required(false, "custom_field", location, results); }
        if directive.fix { entity.update_name("custom fixed"); }
    }
}
struct Deny(Arc<Mutex<Option<Json>>>, bool);
impl MutationPolicy for Deny {
    fn identity(&self) -> MutationPolicyIdentity { MutationPolicyIdentity::new("checker-flow","5.0.6","fixture") }
    fn review(&self, _: &UserContext, plan: &MutationPlan) -> MutationDecision {
        let operations: Vec<_> = plan.operations.iter().map(|operation| {
            let values = operation.changed_values.iter().filter(|(name,_)| !matches!(name.as_str(),"created_at"|"updated_at"))
                .map(|(name,value)|(name.clone(),value.to_json_value())).collect::<serde_json::Map<_,_>>();
            let clock = ["created_at","updated_at"].map(|name|operation.changed_values.contains_key(name));
            let common_clock = operation.changed_values.get("created_at").zip(operation.changed_values.get("updated_at"))
                .map(|(a,b)|a==b);
            json!({"kind":format!("{:?}",operation.kind),"entity":operation.entity.entity,
                "id":operation.entity.id.to_json_value(),"version":operation.original_version,
                "values":values,"clock_fields":clock,"common_clock":common_clock})
        }).collect();
        *self.0.lock().unwrap() = Some(json!({"request":plan.request_key,"root":plan.root_entity_type,"reason":plan.audit_reason,"operations":operations}));
        if self.1 { MutationDecision::allow() } else { MutationDecision::deny("CHECKER-DENIED","preflight observed", ["name"]) }
    }
}
fn diagnostics(results: &[teaql_runtime::CheckResult]) -> Json {
    json!([JsonFieldNamingProfile::CamelCase, JsonFieldNamingProfile::SnakeCase, JsonFieldNamingProfile::PascalCase]
        .map(|profile|results.iter().map(|value|value.to_wire(profile)).collect::<Vec<_>>()))
}

fn observe(input: &Json) -> Json {
    let status = match input["status"].as_str().unwrap() {"create"=>CheckObjectStatus::Create,"update"=>CheckObjectStatus::Update,_=>CheckObjectStatus::Unknown};
    let mut user_context = UserContext::new().with_metadata(InMemoryMetadataStore::new()
        .with_entity(Tenant::entity_descriptor()).with_entity(Note::entity_descriptor()));
    if let Some(root) = input["root"].as_u64() { user_context = user_context.with_active_root("Tenant", root); }
    user_context.insert_resource(Directive {reject:input["reject"]==true,fix:input["fix"]==true});
    let hooks = Arc::new(Mutex::new(Vec::new()));
    let mut entity = Note::from_compact_row(CompactRow::from_map(BTreeMap::from([
        ("id".into(),Value::U64(7)),("name".into(),Value::Text(input["name"].as_str().unwrap().into())),
        ("tenant_id".into(),Value::U64(input["tenant"].as_u64().unwrap())),
        ("created_at".into(),Value::Timestamp(teaql_core::time::Timestamp(111))),
        ("updated_at".into(),Value::Timestamp(teaql_core::time::Timestamp(222))),
        ("active".into(),Value::Bool(true)),("note".into(),Value::Text("stored note".into())),
        ("amount".into(),Value::Decimal("12.50".parse().unwrap())),
        ("status".into(),Value::Text("open".into())),("reference".into(),Value::Text("ref".into())),
        ("version".into(),Value::I64(if status==CheckObjectStatus::Create {0} else {3}))
    ]))).unwrap();
    let mut fields: Vec<String> = match input["loaded"].as_str().unwrap() {
        "identity"=>vec!["id".into(),"version".into()],
        "no_times"=>vec!["id".into(),"name".into(),"tenant_id".into(),"version".into()],
        "no_tenant"=>vec!["id".into(),"name".into(),"created_at".into(),"updated_at".into(),"version".into()],
        _=>vec!["id".into(),"name".into(),"tenant_id".into(),"created_at".into(),"updated_at".into(),"version".into()],
    };
    if input["loaded"] != "identity" {
        for name in ["active", "note", "amount", "status", "reference"] {
            if Note::entity_descriptor().properties.iter().any(|p|p.name==name) {fields.push(name.into());}
        }
    }
    entity.set_load_state(LoadState::Partial(fields.into_iter().collect()));
    let root = entity.entity_runtime_state().unwrap();
    if status==CheckObjectStatus::Create { root.mark_as_new(entity.entity_key()); }
    let captured = Arc::new(Mutex::new(None));
    let mut errors = Vec::new();
    let mut error = None;
    let before = view(&entity);
    let copy = entity.clone();
    match input["op"].as_str().unwrap() {
        "update" => { entity.update_name("  updated  ").update_note(Value::Null).update_active(false); }
        "overlay" => { root.set(entity.entity_key(), "name", Value::Text("  overlay  ".into()));
                       root.set(entity.entity_key(), "note", Value::Null); }
        "self" => { entity.update_name(entity.name());
                    entity.update_note(entity.note().map(Value::Text).unwrap_or(Value::Null)); }
        _ => {}
    }
    if input["mode"]=="direct" {
        NoteChecker::new(Logic(hooks.clone())).check_and_fix_typed(&user_context, &mut entity, status,
            &ObjectLocation::array_root(2).member("note_info"), &mut errors);
    } else {
        user_context.set_checker_registry(InMemoryCheckerRegistry::new()
            .with_checker(TypedEntityChecker::<Tenant,_>::new(TenantChecker::default()))
            .with_checker(TypedEntityChecker::<Note,_>::new(NoteChecker::new(Logic(hooks.clone())))));
        let transport = teaql_provider_sqlite::SqliteMutationExecutor::from_connection(rusqlite::Connection::open_in_memory().unwrap());
        user_context.register_executor(cpp_inheritance_variants_core::ServiceRuntimeExecutor::new(transport));
        let policy: Arc<dyn MutationPolicy> = Arc::new(Deny(captured.clone(),false));
        user_context.set_mutation_policy_registry(move |_: &str|Some(policy.clone()));
        match wait(teaql_runtime::save_audited_ledger_entity(entity.clone().audit_as("Observe inherited fixes"), &user_context)) {
            Err(RuntimeError::Check(results))=> {errors=results;error=Some("Check".to_owned());}
            Err(other)=>error=Some(other.to_string()),
            Ok(_)=>panic!("policy must prevent writes"),
        }
    }
    let evidence: Vec<_> = user_context.last_fix_evidence().into_iter().map(|value|
        json!({"entity":value.entity_type,"path":value.model_path,"source":format!("{:?}",value.source),"label":value.source_label})).collect();
    json!({"wall_millis":teaql_core::time::Timestamp::now().0,"before":before,
        "view":view(&entity),"copy":view(&copy),"hooks":hooks.lock().unwrap().clone(),
        "diagnostics":diagnostics(&errors),"plan":captured.lock().unwrap().clone(),"error":error,"evidence":evidence})
}
observe(input)
}
}; }
define_observer!(observe_document,Document,DocumentChecker,DocumentCheckerLogic,check_and_fix_document);
define_observer!(observe_intermediate_record,IntermediateRecord,IntermediateRecordChecker,IntermediateRecordCheckerLogic,check_and_fix_intermediate_record);
define_observer!(observe_invoice,Invoice,InvoiceChecker,InvoiceCheckerLogic,check_and_fix_invoice);
define_observer!(observe_receipt,Receipt,ReceiptChecker,ReceiptCheckerLogic,check_and_fix_receipt);
define_observer!(observe_special_invoice,SpecialInvoice,SpecialInvoiceChecker,SpecialInvoiceCheckerLogic,check_and_fix_special_invoice);
fn main() {
    for line in io::stdin().lock().lines() {
        let input: Json=serde_json::from_str(&line.unwrap()).unwrap();
        let value=match input["model"].as_str().unwrap() {
"document"=>observe_document(&input),
"intermediate_record"=>observe_intermediate_record(&input),
"invoice"=>observe_invoice(&input),
"receipt"=>observe_receipt(&input),
"special_invoice"=>observe_special_invoice(&input),
_=>panic!("unknown inherited model"),
        };
        println!("{}",value);
    }
}
