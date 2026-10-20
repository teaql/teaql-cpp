//! Executes retained generated inherited Checkers and public audited Save.
use cpp_inheritance_business_ids_core::*;
use serde_json::{json, Value as Json};
use std::{future::Future, io::{self, BufRead}, sync::{Arc, Mutex}, task::{Context, Poll, Wake, Waker}};
use teaql_core::{Entity, TeaqlEntity};
use teaql_runtime::{CheckObjectStatus, CheckResults, InMemoryCheckerRegistry, JsonFieldNamingProfile,
    MutationDecision, MutationPlan, MutationPolicy, MutationPolicyIdentity, ObjectLocation,
    RuntimeError, TypedEntityChecker, UserContext};

struct NoWake;
impl Wake for NoWake { fn wake(self: Arc<Self>) {} }
fn wait<T>(future: impl Future<Output = T>) -> T {
    let mut future = Box::pin(future);
    let waker = Waker::from(Arc::new(NoWake));
    let mut context = Context::from_waker(&waker);
    loop { if let Poll::Ready(value) = future.as_mut().poll(&mut context) { return value; } }
}
struct Settings { mode: String, hooks: Arc<Mutex<Vec<Json>>> }
struct Policy { plans: Arc<Mutex<Vec<Json>>>, allow: bool }
impl MutationPolicy for Policy {
    fn identity(&self) -> MutationPolicyIdentity { MutationPolicyIdentity::new("inherited-extension","5.0.6","fixture") }
    fn review(&self, _: &UserContext, plan: &MutationPlan) -> MutationDecision {
        let operations = plan.operations.iter().map(|operation| {
            let values = operation.changed_values.iter()
                .filter(|(name,_)| !matches!(name.as_str(),"created_at"|"updated_at"))
                .map(|(name,value)|(name.clone(),value.to_json_value())).collect::<serde_json::Map<_,_>>();
            json!({"kind":format!("{:?}",operation.kind),"entity":operation.entity.entity,
                "id":operation.entity.id.to_json_value(),"version":operation.original_version,
                "values":values,"clock_fields":[operation.changed_values.contains_key("created_at"),operation.changed_values.contains_key("updated_at")]})
        }).collect::<Vec<_>>();
        self.plans.lock().unwrap().push(json!({"request":plan.request_key,"root":plan.root_entity_type,
            "reason":plan.audit_reason,"operations":operations}));
        if self.allow { MutationDecision::allow() } else { MutationDecision::deny("EXTENSION-DENIED","business policy denied",["name"]) }
    }
}
fn default_checkers() -> InMemoryCheckerRegistry {
    InMemoryCheckerRegistry::new()
        .with_checker(TypedEntityChecker::<Tenant,_>::new(TenantChecker::default()))
        .with_checker(TypedEntityChecker::<Document,_>::new(DocumentChecker::default()))
        .with_checker(TypedEntityChecker::<IntermediateRecord,_>::new(IntermediateRecordChecker::default()))
        .with_checker(TypedEntityChecker::<Invoice,_>::new(InvoiceChecker::default()))
        .with_checker(TypedEntityChecker::<Receipt,_>::new(ReceiptChecker::default()))
        .with_checker(TypedEntityChecker::<SpecialInvoice,_>::new(SpecialInvoiceChecker::default()))
}
trait OwnFields { fn update_own_fields(&mut self); }
impl OwnFields for Document { fn update_own_fields(&mut self) {} }
impl OwnFields for IntermediateRecord { fn update_own_fields(&mut self) {} }
impl OwnFields for Invoice { fn update_own_fields(&mut self) { self.update_amount("12.50".parse::<teaql_core::Decimal>().unwrap()); } }
impl OwnFields for Receipt { fn update_own_fields(&mut self) { self.update_status("open"); } }
impl OwnFields for SpecialInvoice { fn update_own_fields(&mut self) { self.update_amount("12.50".parse::<teaql_core::Decimal>().unwrap()).update_reference("reference"); } }

fn observe(input: &Json) -> Json {
    let pool = Arc::new(Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));
    let mut user_context = wait(service_runtime_from_pool(pool.clone())).unwrap();
    user_context.disable_sql_log();
    user_context.set_business_clock(teaql_runtime::FixedBusinessClock::new(
        chrono::DateTime::from_timestamp_millis(1_700_000_000_000).unwrap()));
    wait(user_context.ensure_schema()).unwrap();
    let hooks = Arc::new(Mutex::new(Vec::new()));
    let plans = Arc::new(Mutex::new(Vec::new()));
    macro_rules! run {
        ($request:expr, $model:ty, $checker:ident, $logic:ident, $callback:ident) => {{
            struct Logic;
            impl $logic for Logic {
                fn $callback(&self, user_context: &UserContext, entity: &mut $model,
                    status: CheckObjectStatus, location: &ObjectLocation, results: &mut CheckResults) {
                    let settings = user_context.require_resource::<Settings>().unwrap();
                    settings.hooks.lock().unwrap().push(json!({"status":format!("{status:?}"),"before":results.len(),
                        "name":entity.name(),"number":entity.document_number().as_str(),"tenant":entity.tenant_id()}));
                    self.required_text(&entity.name(),"name",location,results);
                    self.min_string_length(&entity.name(),"name",2,location,results);
                    self.max_string_length(&entity.name(),"name",8,location,results);
                    if settings.mode == "deny" { self.required(false,"custom_field",location,results); }
                    if settings.mode == "fix" { entity.update_name("fixed"); }
                }
            }
            let mut model = $request.comment("Prepare an inherited business extension")
                .purpose("Qualify custom Checker and governed Save").new_entity(&user_context);
            model.update_name("seed").update_active(false).update_note(teaql_core::Value::Null);
            model.update_own_fields();
            let update = input["operation"] == "update";
            if update {
                model = wait(teaql_runtime::save_audited_ledger_entity(model.audit_as("Seed inherited business extension"),&user_context)).unwrap();
            }
            let created_before = model.created_at();
            let number_before = model.document_number().as_str().to_owned();
            model.update_name(input["name"].as_str().unwrap());
            user_context.insert_resource(Settings { mode:input["logic"].as_str().unwrap().into(), hooks:hooks.clone() });
            user_context.set_checker_registry(default_checkers().with_checker(TypedEntityChecker::<$model,_>::new($checker::new(Logic))));
            let policy: Arc<dyn MutationPolicy> = Arc::new(Policy { plans:plans.clone(), allow:input["allow"]==true });
            user_context.set_mutation_policy_registry(move |_: &str|Some(policy.clone()));
            let table = <$model>::entity_descriptor().table_name;
            if input["fault"] == "trigger" {
                let event = if update { "UPDATE" } else { "INSERT" };
                pool.lock().unwrap().execute_batch(&format!("CREATE TRIGGER fixture_failure BEFORE {event} ON {table} BEGIN SELECT RAISE(ABORT,'fixture write failed'); END;")).unwrap();
            } else if input["fault"] == "conflict" {
                pool.lock().unwrap().execute_batch(&format!("UPDATE {table} SET version=2")).unwrap();
            }
            let result = wait(teaql_runtime::save_audited_ledger_entity(model.audit_as("Apply inherited business extension"),&user_context));
            let (saved,error,diagnostics) = match result {
                Ok(model)=>(Some(json!({"name":model.name(),"number":model.document_number().as_str(),"tenant":model.tenant_id(),"version":model.version(),
                    "created_retained":if update {Some(model.created_at()==created_before)}else{None},
                    "number_retained":if update {Some(model.document_number().as_str()==number_before)}else{None}})),None,vec![]),
                Err(RuntimeError::Check(results))=>(None,Some("Check".to_owned()),results.iter().map(|value|value.to_wire(JsonFieldNamingProfile::SnakeCase)).collect::<Vec<_>>()),
                Err(error)=>(None,Some(error.to_string()),vec![]),
            };
            let connection = pool.lock().unwrap();
            let mut statement = connection.prepare(&format!("SELECT name,document_number,tenant,version FROM {table} ORDER BY id")).unwrap();
            let rows = statement.query_map([],|row|Ok(json!({"name":row.get::<_,String>(0)?,"number":row.get::<_,String>(1)?,"tenant":row.get::<_,i64>(2)?,"version":row.get::<_,i64>(3)?})))
                .unwrap().map(Result::unwrap).collect::<Vec<_>>();
            json!({"model":stringify!($model),"saved":saved,"error":error,"diagnostics":diagnostics,"rows":rows,
                "hooks":hooks.lock().unwrap().clone(),"plans":plans.lock().unwrap().clone(),
                "fixes":user_context.last_fix_evidence().iter().map(|fix|json!({"entity":fix.entity_type,"field":fix.model_path,"source":format!("{:?}",fix.source),"label":fix.source_label})).collect::<Vec<_>>()})
        }};
    }
    match input["model"].as_str().unwrap() {
        "document"=>run!(Q::documents(),Document,DocumentChecker,DocumentCheckerLogic,check_and_fix_document),
        "intermediate_record"=>run!(Q::intermediate_records(),IntermediateRecord,IntermediateRecordChecker,IntermediateRecordCheckerLogic,check_and_fix_intermediate_record),
        "invoice"=>run!(Q::invoices(),Invoice,InvoiceChecker,InvoiceCheckerLogic,check_and_fix_invoice),
        "receipt"=>run!(Q::receipts(),Receipt,ReceiptChecker,ReceiptCheckerLogic,check_and_fix_receipt),
        _=>run!(Q::special_invoices(),SpecialInvoice,SpecialInvoiceChecker,SpecialInvoiceCheckerLogic,check_and_fix_special_invoice),
    }
}
fn main() {
    for line in io::stdin().lock().lines() {
        let input:Json=serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}",observe(&input));
    }
}
