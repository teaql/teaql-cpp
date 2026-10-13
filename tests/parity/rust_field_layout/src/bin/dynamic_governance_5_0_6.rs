use school_management_service_core::School;
use serde_json::{json, Value as Json};
use std::{
    collections::{BTreeMap, HashMap},
    future::Future,
    io::{self, BufRead},
    sync::{Arc, Mutex},
    task::{Context, Poll, Wake, Waker},
};
use teaql_core::dynamic_fields::{DynamicFieldDefinitions, DynamicFieldValues};
use teaql_core::{CompactRow, DataType, Entity, EntityDescriptor, PropertyDescriptor, Value};
use teaql_provider_sqlite::{SqliteDialect, SqliteMutationExecutor};
use teaql_runtime::dynamic_fields::DatabaseDynamicFieldsProvider;
use teaql_runtime::{
    EntityKey, InMemoryMetadataStore, LedgerEntity, MutationDecision, MutationPlan, MutationPolicy,
    MutationPolicyIdentity, UserContext,
};
use teaql_sql::SqlDataServiceExecutor;
#[path = "../../../rust_values/src/fixture_value.rs"]
mod fixture_value;
struct NoWake;
impl Wake for NoWake {
    fn wake(self: Arc<Self>) {}
}
fn wait<T>(future: impl Future<Output = T>) -> T {
    let mut future = Box::pin(future);
    let waker = Waker::from(Arc::new(NoWake));
    let mut context = Context::from_waker(&waker);
    loop {
        if let Poll::Ready(result) = future.as_mut().poll(&mut context) {
            return result;
        }
    }
}
struct Schema(Arc<EntityDescriptor>);
impl teaql_data_service::SchemaProvider for Schema {
    fn get_entity(&self, name: &str) -> Option<Arc<EntityDescriptor>> {
        (name == "School").then(|| self.0.clone())
    }
}
type Executor = SqlDataServiceExecutor<SqliteDialect, SqliteMutationExecutor, Schema>;
struct Deny(Arc<Mutex<Option<Json>>>);
impl MutationPolicy for Deny {
    fn identity(&self) -> MutationPolicyIdentity {
        MutationPolicyIdentity::new("dynamic", "5.0.6", "fixture")
    }
    fn review(&self, _: &UserContext, plan: &MutationPlan) -> MutationDecision {
        *self.0.lock().unwrap() = Some(
            json!({"request":plan.request_key,"root":plan.root_entity_type,"reason":plan.audit_reason,
            "operations":plan.operations.iter().map(|operation|json!({"kind":format!("{:?}",operation.kind),"entity":operation.entity.entity,"id":operation.entity.id.to_json_value(),"version":operation.original_version,"values":operation.changed_values.iter().map(|(name,value)|(name.clone(),value.to_json_value())).collect::<serde_json::Map<_,_>>()})).collect::<Vec<_>>()}),
        );
        MutationDecision::deny("DYNAMIC-DENIED", "extension denied", ["#title"])
    }
}
fn row(id: u64, definitions: Arc<DynamicFieldDefinitions>) -> School {
    let row = CompactRow::from_map(BTreeMap::from([
        ("id".into(), Value::U64(id)),
        ("name".into(), Value::Text("original".into())),
        ("version".into(), Value::I64(3)),
    ]));
    let mut entity = School::from_compact_row(row).unwrap();
    let fields = DynamicFieldValues::from_values(
        definitions,
        HashMap::from([
            ("title".into(), Value::Text("old".into())),
            ("count".into(), Value::I64(9)),
        ]),
    )
    .unwrap();
    let state = teaql_core::LoadedSnapshot::with_dynamic_fields(
        &entity.loaded_state_snapshot().unwrap(),
        &fields,
    )
    .unwrap();
    entity.install_loaded_dynamic_fields(fields, state).unwrap();
    entity
}
fn observe(input: &Json) -> Json {
    let descriptor = EntityDescriptor::new("School")
        .table_name("school_data")
        .property(PropertyDescriptor::new("id", DataType::U64).id())
        .property(PropertyDescriptor::new("name", DataType::Text))
        .property(PropertyDescriptor::new("version", DataType::I64).version());
    let transport =
        SqliteMutationExecutor::from_connection(rusqlite::Connection::open_in_memory().unwrap());
    let mut context = UserContext::new()
        .with_metadata(InMemoryMetadataStore::new().with_entity(descriptor.clone()));
    context.register_executor(SqlDataServiceExecutor::new(
        SqliteDialect,
        transport,
        Schema(Arc::new(descriptor)),
    ));
    let definitions = DynamicFieldDefinitions::new(
        "School",
        "v1",
        [
            ("title".into(), DataType::Text),
            ("count".into(), DataType::I64),
        ],
    )
    .unwrap();
    context.set_dynamic_fields_provider(Arc::new(
        DatabaseDynamicFieldsProvider::<Executor>::new("profile", [definitions.clone()]).unwrap(),
    ));
    let captured = Arc::new(Mutex::new(None));
    let policy: Arc<dyn MutationPolicy> = Arc::new(Deny(captured.clone()));
    context.set_mutation_policy_registry(move |_: &str| Some(policy.clone()));
    let mut parent = row(7, definitions.clone());
    let root = parent.entity_runtime_state().unwrap();
    if input["native"] == true {
        root.set(
            EntityKey::new("School", 7_u64),
            "name",
            Value::Text("native-pending".into()),
        );
    }
    let mut child = row(if input["other"] == true { 8 } else { 7 }, definitions);
    if input["mode"] == "delete" {
        child.delete_dynamic_field("title").unwrap();
    } else {
        child
            .update_dynamic_field("title", fixture_value::value(&input["value"]))
            .unwrap();
    }
    if input["other"] == true {
        root.adopt_mutations_from(&child.entity_runtime_state().unwrap());
    } else {
        parent = child;
        if input["native"] == true {
            parent.entity_runtime_state().unwrap().set(
                EntityKey::new("School", 7_u64),
                "name",
                Value::Text("native-pending".into()),
            );
        }
    }
    let state = parent.entity_runtime_state().unwrap();
    let error = wait(teaql_runtime::save_audited_ledger_entity(
        parent.audit_as("review dynamic input"),
        &context,
    ))
    .unwrap_err();
    json!({"plan":captured.lock().unwrap().clone(),"error":error.to_string(),"pending":state.has_pending_dynamic_mutations()})
}
fn main() {
    for line in io::stdin().lock().lines() {
        let input: Json = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", observe(&input));
    }
}
