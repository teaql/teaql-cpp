use school_management_service_core::School;
use serde_json::{Value as Json, json};
use std::{
    collections::{BTreeMap, HashMap},
    future::Future,
    io::{self, BufRead},
    sync::Arc,
    task::{Context, Poll, Wake, Waker},
};
use teaql_core::dynamic_fields::{
    DynamicFieldDefinitions, DynamicFieldMutation, DynamicFieldSelection, DynamicFieldValues,
};
use teaql_core::{
    CompactRow, DataType, Entity, EntityDescriptor, Expr, MutationIntent, SelectQuery, TeaqlEntity,
    Value,
};
use teaql_data_service::dynamic_fields::{DynamicFieldStore, DynamicFieldWrite};
use teaql_provider_sqlite::{SqliteDialect, SqliteMutationExecutor};
use teaql_runtime::dynamic_fields::{DatabaseDynamicFieldsProvider, InMemoryDynamicFieldsProvider};
use teaql_runtime::{
    EntityRuntimeState, InMemoryEntityRegistry, InMemoryMetadataStore, PurposedSelectQuery,
    UserContext,
};
use teaql_sql::{SqlDataServiceExecutor, SqlTransaction, SqlTransactionTransport, SqlTransport};
#[path = "../../../rust_values/src/fixture_value.rs"]
mod fixture_value;
struct NoWake;
impl Wake for NoWake {
    fn wake(self: Arc<Self>) {}
}
fn wait<T>(f: impl Future<Output = T>) -> T {
    let mut f = Box::pin(f);
    let w = Waker::from(Arc::new(NoWake));
    let mut c = Context::from_waker(&w);
    loop {
        if let Poll::Ready(v) = f.as_mut().poll(&mut c) {
            return v;
        }
    }
}
struct Schema(Arc<EntityDescriptor>);
impl teaql_data_service::SchemaProvider for Schema {
    fn get_entity(&self, n: &str) -> Option<Arc<EntityDescriptor>> {
        (n == self.0.name).then(|| self.0.clone())
    }
}
type Executor = SqlDataServiceExecutor<SqliteDialect, SqliteMutationExecutor, Schema>;
fn dt(n: u64) -> DataType {
    [
        DataType::Bool,
        DataType::I64,
        DataType::U64,
        DataType::F64,
        DataType::Decimal,
        DataType::Text,
        DataType::LargeText,
        DataType::Json,
        DataType::Date,
        DataType::Timestamp,
    ][n as usize]
}
fn selection(
    i: &Json,
) -> Result<DynamicFieldSelection, teaql_core::dynamic_fields::DynamicFieldError> {
    if i.is_null() {
        Ok(DynamicFieldSelection::All)
    } else {
        DynamicFieldSelection::fields(
            i.as_array()
                .unwrap()
                .iter()
                .map(|p| (p[0].as_str().unwrap().into(), dt(p[1].as_u64().unwrap()))),
        )
    }
}
fn describe(entities: Vec<School>) -> Json {
    let views:Vec<_>=entities.iter().map(|e|{
 let fields=Entity::dynamic_field_values(e);
 let state=Entity::loaded_state_snapshot(e).unwrap();
 let loaded = ["id","name","version","address","#title","#count"].iter().map(|n|((*n).to_owned(),Json::Bool(Entity::is_field_loaded(e,n)))).collect::<serde_json::Map<String,Json>>();
 json!({"loaded":loaded,
 "values":fields.map(|f|f.values().iter().map(|(n,v)|(n.clone(),v.to_json_value())).collect::<serde_json::Map<_,_>>()),"bits":state.bits(),"dirty":Entity::dirty_fields(e).is_some()})
 }).collect();
    let sharing: Vec<Vec<_>> = entities
        .iter()
        .map(|a| {
            entities
                .iter()
                .map(|b| {
                    Arc::ptr_eq(
                        &Entity::loaded_state_snapshot(a).unwrap(),
                        &Entity::loaded_state_snapshot(b).unwrap(),
                    )
                })
                .collect()
        })
        .collect();
    json!({"views":views,"sharing":sharing})
}
fn describe_row(mut row: CompactRow) -> Json {
    let dynamic = row.take_loaded_dynamic_fields().map(|values| {
        values
            .into_values()
            .into_iter()
            .map(|(name, v)| (name, v.to_json_value()))
            .collect::<serde_json::Map<_, _>>()
    });
    let loaded = row
        .take_loaded_relation("peers")
        .map(|list| list.data.into_iter().map(describe_row).collect::<Vec<_>>());
    json!({"native":row.into_map().iter().map(|(name,v)|(name.clone(),v.to_json_value())).collect::<serde_json::Map<_,_>>(),"dynamic":dynamic,"loaded":loaded})
}
fn observe(input: &Json) -> Json {
    let selected = match selection(&input["selection"]) {
        Ok(v) => v,
        Err(e) => return json!({"error":e.to_string()}),
    };
    let definition = DynamicFieldDefinitions::new(
        "School",
        "v1",
        [
            ("title".into(), DataType::Text),
            ("count".into(), DataType::I64),
        ],
    )
    .unwrap();
    if input["mode"] == "decode" {
        let rows: Vec<_> = input["rows"]
            .as_array()
            .unwrap()
            .iter()
            .map(|item| {
                let mut row = CompactRow::from(BTreeMap::from([
                    ("id".into(), Value::U64(item["id"].as_u64().unwrap())),
                    ("name".into(), Value::Text("school".into())),
                ]));
                let payload = item["values"]
                    .as_object()
                    .unwrap()
                    .iter()
                    .map(|(n, v)| (n.clone(), fixture_value::value(v)))
                    .collect();
                row.set_loaded_dynamic_fields(
                    DynamicFieldValues::from_values(definition.clone(), payload).unwrap(),
                );
                row
            })
            .collect();
        return match teaql_runtime::decode_compact_rows_with_read_metadata::<School>(
            rows,
            &EntityRuntimeState::default(),
        ) {
            Ok(v) => describe(v),
            Err(e) => json!({"error":e.to_string()}),
        };
    }
    let mut descriptor = School::entity_descriptor();
    if input["mode"] == "relation" {
        descriptor.relations.clear();
        let relation = teaql_core::RelationDescriptor::new("peers", "School");
        descriptor.relations.push(if input["many"] == true {
            relation.many()
        } else {
            relation
        });
    }
    let connection = rusqlite::Connection::open_in_memory().unwrap();
    connection
        .execute_batch("CREATE TABLE school_data(id INTEGER PRIMARY KEY,name TEXT,version INTEGER)")
        .unwrap();
    for id in input["ids"].as_array().unwrap() {
        connection
            .execute(
                "INSERT INTO school_data VALUES(?1,'school',1)",
                [id.as_u64().unwrap() as i64],
            )
            .unwrap();
    }
    let transport = SqliteMutationExecutor::from_connection(connection);
    let metadata = InMemoryMetadataStore::new().with_entity(descriptor.clone());
    let registry = InMemoryEntityRegistry::new().with_entity(descriptor.name.clone());
    let mut context = UserContext::new()
        .with_metadata(metadata)
        .with_entity_registry(registry);
    context.insert_resource(SqlDataServiceExecutor::new(
        SqliteDialect,
        transport.clone(),
        Schema(Arc::new(descriptor)),
    ));
    let stored: HashMap<u64, HashMap<String, Value>> = input["stored"]
        .as_object()
        .unwrap()
        .iter()
        .map(|(id, p)| {
            (
                id.parse().unwrap(),
                p.as_object()
                    .unwrap()
                    .iter()
                    .map(|(n, v)| (n.clone(), fixture_value::value(v)))
                    .collect(),
            )
        })
        .collect();
    if input["provider"] == "database" {
        wait(DynamicFieldStore::ensure_schema(&transport)).unwrap();
        let tx = wait(transport.begin_sql()).unwrap();
        let writes: Vec<_> = stored
            .iter()
            .map(|(id, p)| DynamicFieldWrite {
                namespace: "profile".into(),
                definitions: definition.clone(),
                owner_id: *id,
                changes: p
                    .iter()
                    .map(|(n, v)| (n.clone(), DynamicFieldMutation::Set(v.clone())))
                    .collect(),
            })
            .collect();
        wait(
            tx.dynamic_field_store()
                .unwrap()
                .apply_writes(&writes, &MutationIntent::new("seed extensions").unwrap()),
        )
        .unwrap();
        wait(tx.commit_sql()).unwrap();
        context.set_dynamic_fields_provider(Arc::new(
            DatabaseDynamicFieldsProvider::<Executor>::new("profile", [definition]).unwrap(),
        ));
    } else if input["provider"] == "fixture" {
        match InMemoryDynamicFieldsProvider::from_owners([(definition, stored)]) {
            Ok(v) => context.set_dynamic_fields_provider(Arc::new(v)),
            Err(e) => return json!({"error":e.to_string()}),
        };
    }
    let mut query = SelectQuery::new("School")
        .project("name")
        .order_asc("id")
        .select_dynamic_fields(selected.clone());
    if input["filter"].is_u64() {
        query = query.filter(Expr::eq(
            "id",
            Value::U64(input["filter"].as_u64().unwrap()),
        ));
    }
    query.comment = Some("inspect dynamic schools".into());
    if input["mode"] == "child" {
        query.dynamic_field_selection = None;
        query = query.project("id");
        for child in input["children"].as_array().unwrap() {
            let selected = match selection(&child["selection"]) {
                Ok(v) => v,
                Err(e) => return json!({"error":e.to_string()}),
            };
            let mut enhancement = SelectQuery::new("School")
                .project("id")
                .project_expr(
                    "name",
                    Expr::Value(Value::Text(child["name"].as_str().unwrap().into())),
                )
                .select_dynamic_fields(selected);
            if child["filter"].is_u64() {
                enhancement = enhancement.filter(Expr::eq(
                    "id",
                    Value::U64(child["filter"].as_u64().unwrap()),
                ));
            }
            query.child_enhancements.push(enhancement);
        }
    }
    if input["mode"] == "relation" {
        query.dynamic_field_selection = None;
        query = query.project("id");
        let mut relation_query = SelectQuery::new("School")
            .projects(["id", "name"])
            .order_asc("id")
            .select_dynamic_fields(selected);
        if input["child_filter"].is_u64() {
            relation_query = relation_query.filter(Expr::eq(
                "id",
                Value::U64(input["child_filter"].as_u64().unwrap()),
            ));
        }
        if let Some(depth) = input["depth"].as_u64() {
            for _ in 0..depth {
                relation_query = SelectQuery::new("School")
                    .projects(["id", "name"])
                    .relation_query("peers", relation_query);
            }
        }
        query = query.relation_query("peers", relation_query);
    }
    let query = PurposedSelectQuery::new(query, "hydrate schools");
    let service = context.entity_data_service::<Executor>("School").unwrap();
    if input["mode"] == "child" || input["mode"] == "relation" {
        return match wait(service.fetch_all(&query)) {
            Err(e) => json!({"error":e.to_string()}),
            Ok(rows) => json!({"rows":rows.into_iter().map(describe_row).collect::<Vec<_>>()}),
        };
    }
    if input["mode"] == "stream" {
        let mut query = query.into_query();
        if input["missing_id"] != true {
            query = query.project("id");
        }
        query.stream_config = Some(teaql_core::StreamConfig {
            chunk_size: input["chunk"].as_u64().unwrap() as usize,
        });
        let query = PurposedSelectQuery::new(query, "hydrate schools");
        let mut stream = match wait(service.fetch_stream(&query)) {
            Ok(v) => v,
            Err(e) => return json!({"error":e.to_string()}),
        };
        let mut chunks = Vec::new();
        loop {
            let next = wait(std::future::poll_fn(|cx| stream.as_mut().poll_next(cx)));
            match next {
                None => break,
                Some(Err(e)) => {
                    chunks.push(json!({"error":e.to_string()}));
                    break;
                }
                Some(Ok(mut chunk)) => {
                    chunks.push(json!({"index":chunk.chunk_index,"last":chunk.is_last,"rows":chunk.rows.iter_mut().map(|row|json!({"id":row.get("id").unwrap().to_json_value(),"values":row.take_loaded_dynamic_fields().unwrap().values().iter().map(|(n,v)|(n.clone(),v.to_json_value())).collect::<serde_json::Map<_,_>>()})).collect::<Vec<_>>()}));
                }
            }
        }
        return json!({"chunks":chunks});
    }
    let loaded = if input["mode"] == "enhanced" {
        wait(service.fetch_enhanced_entities::<School>(&query))
    } else {
        wait(service.fetch_entities::<School>(&query))
    };
    match loaded {
        Ok(v) => describe(v.data),
        Err(e) => json!({"error":e.to_string()}),
    }
}
fn main() {
    for line in io::stdin().lock().lines() {
        let input: Json = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", observe(&input));
    }
}
