use cpp_checker_fixture_core::{Note, NoteChecker, NoteCheckerLogic, Tenant, TenantChecker};
use serde_json::{json, Value as Json};
use std::{collections::BTreeMap, future::Future, io::{self, BufRead}, sync::{Arc, Mutex}, task::{Context, Poll, Wake, Waker}};
use teaql_core::{CompactRow, Entity, TeaqlEntity, Value, eval::LoadState};
use teaql_runtime::{CheckObjectStatus, CheckResults, InMemoryCheckerRegistry, InMemoryMetadataStore,
    JsonFieldNamingProfile, LedgerEntity, MutationDecision, MutationPlan, MutationPolicy, MutationPolicyIdentity,
    ObjectLocation, RuntimeError, TypedChecker, TypedEntityChecker, UserContext};

struct NoWake;
impl Wake for NoWake { fn wake(self: Arc<Self>) {} }
fn wait<T>(future: impl Future<Output=T>) -> T {
    let mut future = Box::pin(future);
    let waker = Waker::from(Arc::new(NoWake));
    let mut context = Context::from_waker(&waker);
    loop { if let Poll::Ready(result) = future.as_mut().poll(&mut context) { return result; } }
}
struct Directive { reject: bool, fix: bool }
fn view(entity: &Note) -> Json {
    json!({"tenant":entity.tenant_id(),"name":entity.name(),
        "created_changed":entity.created_at().0 != 111,"updated_changed":entity.updated_at().0 != 222,
        "loaded":[entity.is_loaded("id"),entity.is_loaded("name"),entity.is_loaded("tenant_id"),entity.is_loaded("created_at"),entity.is_loaded("updated_at"),entity.is_loaded("version")]})
}
struct Logic(Arc<Mutex<Vec<Json>>>);
impl NoteCheckerLogic for Logic {
    fn check_and_fix_note(&self, user_context: &UserContext, entity: &mut Note, _status: CheckObjectStatus,
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
    let commit = input["mode"]=="commit";
    let status = match input["status"].as_str().unwrap() {"create"=>CheckObjectStatus::Create,"update"=>CheckObjectStatus::Update,_=>CheckObjectStatus::Unknown};
    let mut user_context = UserContext::new().with_metadata(InMemoryMetadataStore::new()
        .with_entity(Tenant::entity_descriptor()).with_entity(Note::entity_descriptor()));
    if let Some(root) = input["root"].as_u64() { user_context = user_context.with_active_root("Tenant", root); }
    user_context.insert_resource(Directive {reject:input["reject"]==true,fix:input["fix"]==true});
    let hooks = Arc::new(Mutex::new(Vec::new()));
    let mut entity = Note::from_compact_row(CompactRow::from_map(BTreeMap::from([
        ("id".into(),Value::U64(7)),("name".into(),Value::Text(if commit && status==CheckObjectStatus::Update {"stored".into()} else {input["name"].as_str().unwrap().into()})),
        ("tenant_id".into(),Value::U64(input["tenant"].as_u64().unwrap())),
        ("created_at".into(),Value::Timestamp(teaql_core::time::Timestamp(111))),
        ("updated_at".into(),Value::Timestamp(teaql_core::time::Timestamp(222))),
        ("version".into(),Value::I64(if status==CheckObjectStatus::Create {0} else {3}))
    ]))).unwrap();
    if commit {
        if status==CheckObjectStatus::Create {
            entity=Note::from_compact_row(CompactRow::from_map(BTreeMap::new())).unwrap();
            entity.update_id(7_u64).update_version(0_i64)
                .update_created_at(teaql_core::time::Timestamp(111))
                .update_updated_at(teaql_core::time::Timestamp(222));
        }
        entity.update_name(input["name"].as_str().unwrap());
    }
    let fields: Vec<String> = match input["loaded"].as_str().unwrap() {
        "identity"=>vec!["id".into(),"version".into()],
        "no_times"=>vec!["id".into(),"name".into(),"tenant_id".into(),"version".into()],
        "no_tenant"=>vec!["id".into(),"name".into(),"created_at".into(),"updated_at".into(),"version".into()],
        _=>vec!["id".into(),"name".into(),"tenant_id".into(),"created_at".into(),"updated_at".into(),"version".into()],
    };
    entity.set_load_state(LoadState::Partial(fields.into_iter().collect()));
    let root = entity.entity_runtime_state().unwrap();
    if status==CheckObjectStatus::Create { root.mark_as_new(entity.entity_key()); }
    let captured = Arc::new(Mutex::new(None));
    let mut errors = Vec::new();
    let mut error = None;
    let mut error_kind = None;
    let mut saved = None;
    let mut database = None;
    let mut db_before = None;
    if input["mode"]=="direct" {
        NoteChecker::new(Logic(hooks.clone())).check_and_fix_typed(&user_context, &mut entity, status,
            &ObjectLocation::array_root(2).member("note_info"), &mut errors);
    } else {
        user_context.set_checker_registry(InMemoryCheckerRegistry::new()
            .with_checker(TypedEntityChecker::<Tenant,_>::new(TenantChecker::default()))
            .with_checker(TypedEntityChecker::<Note,_>::new(NoteChecker::new(Logic(hooks.clone())))));
        let transport = teaql_provider_sqlite::SqliteMutationExecutor::from_connection(rusqlite::Connection::open_in_memory().unwrap());
        if commit {
            use teaql_provider_sqlite::SqliteProviderExt;
            user_context.use_sqlite_provider(transport.clone());
        }
        user_context.register_executor(cpp_checker_fixture_core::ServiceRuntimeExecutor::new(transport.clone()));
        if commit {
            wait(user_context.ensure_schema()).unwrap();
            let connection = transport.connection();
            {
                let connection = connection.lock().unwrap();
                connection.execute_batch("INSERT INTO tenant_data(id,name,version) VALUES(42,'trusted',3),(43,'other',3);").unwrap();
                if status==CheckObjectStatus::Update {
                    connection.execute_batch("INSERT INTO note_data(id,name,tenant,created_at,updated_at,version) VALUES(7,'stored',42,111,222,3);").unwrap();
                }
                if input["fault"]=="trigger" {
                    let event = if status==CheckObjectStatus::Create {"INSERT"} else {"UPDATE"};
                    connection.execute_batch(&format!("CREATE TRIGGER fixture_failure BEFORE {event} ON note_data BEGIN SELECT RAISE(ABORT,'fixture write failed'); END;")).unwrap();
                }
                if input["fault"]=="conflict" && status==CheckObjectStatus::Update {
                    connection.execute_batch("UPDATE note_data SET version=4 WHERE id=7;").unwrap();
                }
            }
            db_before = Some(database_view(&connection.lock().unwrap()));
            database = Some(connection);
        }
        let policy: Arc<dyn MutationPolicy> = Arc::new(Deny(captured.clone(),commit));
        user_context.set_mutation_policy_registry(move |_: &str|Some(policy.clone()));
        let result = wait(teaql_runtime::save_audited_ledger_entity(entity.clone().audit_as("Observe generated fixes"), &user_context));
        match result {
            Err(RuntimeError::Check(results)) => {errors=results;error=Some("Check".to_owned());error_kind=Some("Check");},
            Err(other) => {
                error_kind=Some(match &other {
                    RuntimeError::Graph(_)=>"Graph", RuntimeError::OptimisticLockConflict{..}=>"OptimisticLockConflict",
                    RuntimeError::Transaction(_)=>"Transaction", RuntimeError::Schema(_)=>"Schema",
                    RuntimeError::Policy(_)=>"Policy", RuntimeError::Event(_)=>"Event",
                    RuntimeError::SqlCompile(_)=>"SqlCompile", _=>"Other",
                });
                if commit {
                    error=Some(match &other {
                        RuntimeError::OptimisticLockConflict{..}=>"OptimisticLockConflict".into(),
                        _ if other.to_string().contains("fixture write failed")=>"FixtureWriteFailure".into(),
                        _=>other.to_string(),
                    });
                } else {error=Some(other.to_string());}
            },
            Ok(entity) => {
                if !commit {panic!("policy must prevent writes");}
                saved=Some(json!({"view":view(&entity),"version":entity.version(),"common_clock":entity.created_at()==entity.updated_at()}));
            },
        }
    }
    let evidence: Vec<_> = user_context.last_fix_evidence().into_iter().map(|value|
        json!({"entity":value.entity_type,"path":value.model_path,"source":format!("{:?}",value.source),"label":value.source_label})).collect();
    let mut output = json!({"view":view(&entity),"hooks":hooks.lock().unwrap().clone(),"diagnostics":diagnostics(&errors),
        "plan":captured.lock().unwrap().clone(),"error":error,"evidence":evidence});
    if commit {
        let connection=database.unwrap();
        output["saved"]=json!(saved);
        output["error_kind"]=json!(error_kind);
        output["db_before"]=db_before.unwrap();
        output["db_after"]=database_view(&connection.lock().unwrap());
        output["pending_fields"]=json!(root.current_change_set().changes().values().flat_map(|fields|fields.keys().cloned()).collect::<std::collections::BTreeSet<_>>());
        output["new_keys"]=json!(root.new_keys().len());
    }
    output
}
fn database_view(connection: &rusqlite::Connection) -> Json {
    let mut statement=connection.prepare("SELECT id,name,tenant,version,created_at=111,updated_at=222,created_at=updated_at FROM note_data ORDER BY id").unwrap();
    let rows=statement.query_map([],|row|Ok(json!({"id":row.get::<_,i64>(0)?,"name":row.get::<_,String>(1)?,"tenant":row.get::<_,i64>(2)?,"version":row.get::<_,i64>(3)?,"created_old":row.get::<_,bool>(4)?,"updated_old":row.get::<_,bool>(5)?,"common_clock":row.get::<_,bool>(6)?}))).unwrap();
    json!(rows.collect::<Result<Vec<_>,_>>().unwrap())
}
fn main() {
    for line in io::stdin().lock().lines() {
        let input: Json = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", observe(&input));
    }
}
