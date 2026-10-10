//! Exercise the original public enhanced-entity entries with handwritten models.
use serde_json::{Value as J, json};
use std::{
    collections::BTreeMap,
    future::Future,
    io::{self, BufRead},
    sync::{Arc, Mutex},
    task::{Context, Poll, Wake, Waker},
};
use teaql_core::*;
use teaql_data_service::*;
use teaql_runtime::{
    EntityDataService, EntityDataServiceBehavior, EntityRuntimeState, InMemoryAggregationCache,
    PurposedSelectQuery, RuntimeModule, UserContext,
};
#[derive(Clone)]
struct Model<const K: usize> {
    row: CompactRow,
    state: Option<EntityRuntimeState>,
}
impl<const K: usize> TeaqlEntity for Model<K> {
    const ENTITY_NAME: &'static str = if K == 0 {
        "Parent"
    } else if K == 1 {
        "Child"
    } else {
        "Leaf"
    };
    fn entity_descriptor() -> EntityDescriptor {
        EntityDescriptor::new(Self::ENTITY_NAME)
    }
}
impl<const K: usize> Entity for Model<K> {
    fn from_compact_row(row: CompactRow) -> Result<Self, EntityError> {
        if row.get("fail").and_then(Value::try_bool).unwrap_or(false) {
            return Err(EntityError::new(Self::ENTITY_NAME, "model decode failed"));
        }
        Ok(Self { row, state: None })
    }
    fn into_values(self) -> MutationValues {
        self.row.into_map().into()
    }
    fn on_loaded(&mut self, context: &dyn std::any::Any) {
        if let Some(root) = context.downcast_ref::<EntityRuntimeState>() {
            let mut state = root.clone();
            state.set_original_compact_row(Self::ENTITY_NAME, self.row.clone());
            self.state = Some(state);
        }
    }
}
impl<const K: usize> IdentifiableEntity for Model<K> {
    fn id_value(&self) -> Value {
        self.row.get("id").cloned().unwrap_or(Value::Null)
    }
}
struct Behavior {
    calls: Arc<Mutex<usize>>,
    loads: bool,
}
impl EntityDataServiceBehavior for Behavior {
    fn before_select(
        &self,
        _: &UserContext,
        _: &mut SelectQuery,
    ) -> Result<(), teaql_runtime::RuntimeError> {
        *self.calls.lock().unwrap() += 1;
        Ok(())
    }
    fn relation_loads(&self, _: &UserContext) -> Vec<String> {
        if self.loads {
            vec!["children".into()]
        } else {
            vec![]
        }
    }
}
fn record(fields: &[(&str, Value)]) -> CompactRow {
    CompactRow::from_map(
        fields
            .iter()
            .map(|(name, value)| (name.to_string(), value.clone()))
            .collect(),
    )
}
struct Mock {
    requests: Mutex<Vec<String>>,
    failure: String,
    fail_model: String,
}
impl DataServiceExecutor for Mock {
    type Error = io::Error;
    fn capabilities(&self) -> DataServiceCapabilities {
        DataServiceCapabilities::default()
    }
}
impl QueryExecutor for Mock {
    async fn query(&self, request: QueryRequest) -> Result<QueryResult, Self::Error> {
        self.requests
            .lock()
            .unwrap()
            .push(format!("{:?}", request.query));
        let name = request.query.entity.as_str();
        if self.failure == name {
            return Err(io::Error::other("provider failed"));
        }
        let mut rows = match name {
            "Parent" => vec![
                record(&[("id", Value::U64(1)), ("ref", Value::U64(10))]),
                record(&[("id", Value::U64(2)), ("ref", Value::Null)]),
            ],
            "Child" => vec![
                record(&[
                    ("id", Value::U64(10)),
                    ("parent_id", Value::U64(1)),
                    ("leaf_ref", Value::U64(100)),
                ]),
                record(&[
                    ("id", Value::U64(11)),
                    ("parent_id", Value::U64(1)),
                    ("leaf_ref", Value::U64(100)),
                ]),
            ],
            "Leaf" => vec![record(&[("id", Value::U64(100))])],
            _ => panic!("fixture entity"),
        };
        if self.fail_model == name {
            rows[0].insert("fail".into(), Value::Bool(true));
        }
        Ok(QueryResult {
            metadata: ExecutionMetadata::unrecorded_query(rows.len()),
            rows,
        })
    }
}
impl MutationExecutor for Mock {
    async fn mutate(&self, _: MutationRequest) -> Result<MutationResult, Self::Error> {
        panic!("unexpected mutation")
    }
}
struct NoWake;
impl Wake for NoWake {
    fn wake(self: Arc<Self>) {}
}
fn wait<F: Future>(future: F) -> F::Output {
    let mut future = std::pin::pin!(future);
    let waker = Waker::from(Arc::new(NoWake));
    let mut cx = Context::from_waker(&waker);
    loop {
        if let Poll::Ready(value) = future.as_mut().poll(&mut cx) {
            return value;
        }
    }
}
fn module(input: &J) -> RuntimeModule {
    let mut parent = EntityDescriptor::new("Parent");
    let mut id = PropertyDescriptor::new("id", DataType::U64);
    id.is_id = true;
    parent.properties.push(id.clone());
    parent
        .properties
        .push(PropertyDescriptor::new("ref", DataType::U64));
    let mut children = RelationDescriptor::new("children", "Child");
    children.local_key = "id".into();
    children.foreign_key = "parent_id".into();
    children.many = true;
    parent.relations.push(children);
    let mut selected = RelationDescriptor::new("selected", "Child");
    selected.local_key = "ref".into();
    selected.foreign_key = "id".into();
    parent.relations.push(selected);
    let mut child = EntityDescriptor::new("Child");
    child.properties.push(id.clone());
    child
        .properties
        .push(PropertyDescriptor::new("parent_id", DataType::U64));
    child
        .properties
        .push(PropertyDescriptor::new("leaf_ref", DataType::U64));
    let mut inverse = RelationDescriptor::new("parent", "Parent");
    inverse.local_key = "parent_id".into();
    inverse.foreign_key = "id".into();
    child.relations.push(inverse);
    let mut product = RelationDescriptor::new("product", "Leaf");
    product.local_key = "leaf_ref".into();
    product.foreign_key = "id".into();
    child.relations.push(product);
    let mut leaf = EntityDescriptor::new("Leaf");
    leaf.properties.push(id);
    let mut module = RuntimeModule::new().entity::<Model<0>>();
    if input["child_decoder"] == true {
        module = module.entity::<Model<1>>();
    }
    if input["leaf_decoder"] == true {
        module = module.entity::<Model<2>>();
    }
    module.descriptor(parent).descriptor(child).descriptor(leaf)
}
fn snapshot(value: &Model<0>) -> J {
    let state = value.state.as_ref().unwrap();
    let id = value.id_value().try_u64().unwrap();
    let children = state.relation_list::<Model<1>>("Parent", id, "children");
    let related = children.value().map(|list| list.data.iter().map(|c| json!({
        "id": c.id_value().try_u64(), "product_embedded": c.row.get("product").is_some(),
        "leaf": c.state.as_ref().unwrap().resolve_entity::<Model<2>>(100).is_some(),
        "parent_view": c.state.as_ref().unwrap().has_relation_view("Child", c.id_value().try_u64().unwrap(), "parent")
    })).collect::<Vec<_>>());
    json!({"row": value.row.clone().into_map().iter().map(|(k,v)| (k.clone(), format!("{v:?}"))).collect::<BTreeMap<_,_>>(),
        "children_state": format!("{:?}", children.state()), "children": related,
        "selected": state.resolve_entity::<Model<1>>(10).is_some(),
        "has_original": state.original_snapshot().is_some()})
}
fn evaluate(input: &J) -> J {
    let calls = Arc::new(Mutex::new(0));
    let mut context = module(input)
        .behavior(
            "Parent",
            Behavior {
                calls: calls.clone(),
                loads: input["behavior"] == true,
            },
        )
        .into_context()
        .with_user_identifier("fixture");
    if input["capture"] == false {
        context.disable_sql_log();
    }
    context.insert_resource(InMemoryAggregationCache::with_namespace("enhanced"));
    if input["sqlite"] == true {
        struct Schema(BTreeMap<String, Arc<EntityDescriptor>>);
        impl SchemaProvider for Schema {
            fn get_entity(&self, name: &str) -> Option<Arc<EntityDescriptor>> {
                self.0.get(name).cloned()
            }
        }
        let db = rusqlite::Connection::open_in_memory().unwrap();
        db.execute_batch("CREATE TABLE parent_data(id INTEGER PRIMARY KEY,ref INTEGER); CREATE TABLE child_data(id INTEGER PRIMARY KEY,parent_id INTEGER,leaf_ref INTEGER); CREATE TABLE leaf_data(id INTEGER PRIMARY KEY); INSERT INTO parent_data VALUES(1,10),(2,NULL); INSERT INTO child_data VALUES(10,1,100),(11,1,100); INSERT INTO leaf_data VALUES(100);").unwrap();
        let schema = Schema(
            context
                .all_entities()
                .into_iter()
                .map(|e| (e.name.clone(), Arc::new(e.clone())))
                .collect(),
        );
        let executor = teaql_sql::SqlDataServiceExecutor::new(
            teaql_provider_sqlite::SqliteDialect,
            teaql_provider_sqlite::SqliteMutationExecutor::from_connection(db),
            schema,
        );
        return run(&context, &executor, input, &calls);
    }
    let executor = Mock {
        requests: Mutex::new(vec![]),
        failure: input["failure"].as_str().unwrap().into(),
        fail_model: input["fail_model"].as_str().unwrap().into(),
    };
    let mut output = run(&context, &executor, input, &calls);
    output["requests"] = json!(*executor.requests.lock().unwrap());
    output
}
fn run<E: QueryExecutor + MutationExecutor + Send + Sync>(
    context: &UserContext,
    executor: &E,
    input: &J,
    calls: &Mutex<usize>,
) -> J {
    let repo = EntityDataService::for_executor(context, "Parent", executor);
    let mut query = repo.select().order_asc("id");
    if input["cache"] == true {
        query = query.enable_aggregation_cache();
    }
    if input["idset"] == true {
        query = query
            .page(0, 1)
            .optimize_pagination_with_id_set_config("fixture", 600, 2);
    }
    if input["continuous"] == true {
        query = query.page(0, 1).optimize_for_continuous_page_fetch();
    }
    if input["relations"] == true {
        let mut child = SelectQuery::new("Child");
        if input["explicit_child_projection"] == true {
            child = child.projects(["id", "parent_id", "leaf_ref"]);
        }
        if input["nested"] == true {
            child = child.relation("product");
        }
        query = query.relation_query("children", child).relation("selected");
    }
    if input["enhancement"] == true {
        query = query.child_enhancement(SelectQuery::new("Parent"));
    }
    let query = PurposedSelectQuery::new(query, "Inspect enhanced entities");
    let result = if input["owned"] == true {
        wait(repo.fetch_enhanced_entities_with_relation_aggregates_owned::<Model<0>>(query, &[]))
    } else {
        wait(repo.fetch_enhanced_entities::<Model<0>>(&query))
    };
    let mut output = match result {
        Ok(list) => json!({"loaded": list.is_loaded, "total": list.total_count,
                          "data": list.data.iter().map(snapshot).collect::<Vec<_>>()}),
        Err(error) => json!({"error": error.to_string()}),
    };
    output["hooks"] = json!(*calls.lock().unwrap());
    output["id_plan"] = json!(context.id_set_plan());
    output
}
fn main() {
    for line in io::stdin().lock().lines() {
        println!(
            "{}",
            evaluate(&serde_json::from_str::<J>(&line.unwrap()).unwrap())
        );
    }
}
