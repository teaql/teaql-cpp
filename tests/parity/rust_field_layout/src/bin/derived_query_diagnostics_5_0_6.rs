use std::{future::Future, io::{self, BufRead}, sync::Arc, task::{Context, Poll, Wake, Waker}};
use serde_json::{Value as Json, json};
use teaql_core::{CompactRow, DataType, EntityDescriptor, Expr, PropertyDescriptor, SelectQuery, Value};
use teaql_core::request::{FacetRequest, QueryOptions, QuerySelection, RelationAggregate};
use teaql_data_service::SchemaProvider;
use teaql_provider_sqlite::{SqliteDialect, SqliteMutationExecutor, SqliteProviderExt};
use teaql_runtime::{InMemoryEntityRegistry, InMemoryMetadataStore, PurposedSelectQuery, UserContext};
use teaql_sql::SqlDataServiceExecutor;
#[path = "../../../../../.reference/teaql-rs/examples/src/lib.rs"]
mod original_models;

fn wait<T>(future: impl Future<Output=T>) -> T {
    struct NoWake;
    impl Wake for NoWake { fn wake(self: Arc<Self>) {} }
    let mut future=Box::pin(future);
    let waker=Waker::from(Arc::new(NoWake));
    let mut context=Context::from_waker(&waker);
    loop { if let Poll::Ready(value)=future.as_mut().poll(&mut context) { return value; } }
}
#[derive(Clone)]
struct Schema(Arc<EntityDescriptor>);
impl SchemaProvider for Schema {
    fn get_entity(&self, name:&str)->Option<Arc<EntityDescriptor>> {
        (name==self.0.name).then(||self.0.clone())
    }
}
type Executor=SqlDataServiceExecutor<SqliteDialect,SqliteMutationExecutor,Schema>;
fn row(value:&CompactRow)->Json {
    json!([value.get("id").and_then(Value::try_u64),value.get("version").and_then(Value::try_u64),value.get("count").and_then(Value::try_u64)])
}
fn logs(user_context:&UserContext)->Json {
    json!(user_context.sql_logs().iter().map(|entry|json!({
        "sql":entry.sql,"comment":entry.comment,"purpose":entry.purpose,"debug_sql":entry.debug_sql,
        "outcome":entry.log_context.execution_outcome.map(|value|format!("{value:?}")),
        "params":entry.params.iter().map(|value|format!("{value:?}")).collect::<Vec<_>>(),
        "trace":entry.trace_path.iter().map(|value|format!("{value:?}")).collect::<Vec<_>>()
    })).collect::<Vec<_>>())
}
fn future_options(secret:&str, shape:&str)->QueryOptions {
    let private=QuerySelection::new(SelectQuery::new("Order").filter(Expr::eq("name",secret)));
    let mut selection=QuerySelection::new(SelectQuery::new("Order"));
    match shape {
        "aggregate"=>selection.query_options.relation_aggregates.push(RelationAggregate::new("unused","private_count",private,true)),
        "nested"=>selection.query_options.facets.push(FacetRequest::new("nested","unused",private,true)),
        _=>selection=private,
    }
    let mut options=QueryOptions::default();
    options.facets.push(FacetRequest::new("future","unused",selection,true));
    options
}
fn observe(input:&Json)->Json {
    let descriptor=EntityDescriptor::new("Order").table_name("example_orders")
        .property(PropertyDescriptor::new("id",DataType::I64).id())
        .property(PropertyDescriptor::new("version",DataType::I64).version())
        .property(PropertyDescriptor::new("name",DataType::Text))
        .property(PropertyDescriptor::new("password",DataType::Text))
        .property(PropertyDescriptor::new("status",DataType::Text)).audit_mask_fields(vec!["name".into()]);
    let transport=SqliteMutationExecutor::from_connection(rusqlite::Connection::open_in_memory().unwrap());
    let mut user_context=UserContext::new()
        .with_metadata(InMemoryMetadataStore::new().with_entity(descriptor.clone()))
        .with_entity_registry(InMemoryEntityRegistry::new().with_entity("Order"));
    user_context.use_sqlite_provider(transport.clone());
    user_context.register_executor(Executor::new(SqliteDialect,transport.clone(),Schema(Arc::new(descriptor))));
    transport.connection().lock().unwrap().execute_batch("CREATE TABLE example_orders(id INTEGER PRIMARY KEY,version INTEGER,name TEXT,password TEXT,status TEXT); INSERT INTO example_orders VALUES(9,1,'first','credential','ACTIVE'),(10,2,'second','credential','ACTIVE'),(11,1,'other','credential','INACTIVE');").unwrap();
    user_context.disable_sql_log();
    if input["logging"]==true { user_context.enable_select_sql_log(); }
    let kind=input["kind"].as_str().unwrap();let secret=input["secret"].as_str().unwrap();
    let mut query=SelectQuery::new("Order").filter(Expr::eq("status","ACTIVE")).project("id").project("version").limit(1);
    match kind {
        "relation"=>query=query.relation_query("unused_relation",SelectQuery::new("Order").filter(Expr::eq("name",secret))),
        "projection"=>query=query.project_expr("discarded",Expr::eq("name",secret)),
        "order"=>query=query.order_expr_asc(Expr::eq("password",secret)),
        _=>(),
    }
    if input["invalid_intent"]!=true { query=query.comment(format!("count ACTIVE including {secret}")); }
    let mut request=PurposedSelectQuery::new(query,format!("page ACTIVE with {secret}"));
    if kind=="facet" {
        let mut options=QueryOptions::default();
        options.facets.push(FacetRequest::new("secret_facet","unused_relation",QuerySelection::new(SelectQuery::new("Order").filter(Expr::eq("name",secret))),true));
        request=request.with_facet_diagnostics(&options);
    } else { request=request.for_exact_count("count"); }
    if let Some(steps)=input.get("diagnostic_steps").and_then(Json::as_array) {
        for step in steps {
            match step["action"].as_str().unwrap() {
                "facet"=>request=request.with_facet_diagnostics(&future_options(step["secret"].as_str().unwrap(),step["shape"].as_str().unwrap())),
                "empty"=>request=request.with_facet_diagnostics(&QueryOptions::default()),
                "count"=>request=request.for_exact_count(step["alias"].as_str().unwrap()),
                "clone"=>request=request.clone(),
                _=>panic!("unknown diagnostic sequence action"),
            }
        }
    }
    let mut output=json!({"wrapper":format!("{request:?}")});
    if input["failure"]==true { transport.connection().lock().unwrap().execute_batch("DROP TABLE example_orders").unwrap(); }
    let service=user_context.entity_data_service::<Executor>("Order").unwrap();
    let mut rows=Vec::new();let mut error=None;
    match input["endpoint"].as_str().unwrap() {
        "entities"=>match wait(service.fetch_entities::<original_models::Order>(&request)) { Ok(values)=>rows=values.iter().map(|value|json!([value.id,value.version,Json::Null])).collect(),Err(value)=>error=Some(value.to_string()) },
        "enhanced"=>match wait(service.fetch_enhanced_entities::<original_models::Order>(&request)) { Ok(values)=>rows=values.iter().map(|value|json!([value.id,value.version,Json::Null])).collect(),Err(value)=>error=Some(value.to_string()) },
        "enhanced_aggregate"=>match wait(service.fetch_enhanced_entities_with_relation_aggregates::<original_models::Order>(&request,&[])) { Ok(values)=>rows=values.iter().map(|value|json!([value.id,value.version,Json::Null])).collect(),Err(value)=>error=Some(value.to_string()) },
        "enhanced_owned"=>match wait(service.fetch_enhanced_entities_with_relation_aggregates_owned::<original_models::Order>(request.clone(),&[])) { Ok(values)=>rows=values.iter().map(|value|json!([value.id,value.version,Json::Null])).collect(),Err(value)=>error=Some(value.to_string()) },
        "owned"=>match wait(service.fetch_all_owned(request.clone())) { Ok(values)=>rows=values.iter().map(row).collect(),Err(value)=>error=Some(value.to_string()) },
        "list"=>match wait(service.fetch_smart_list(&request)) { Ok(values)=>rows=values.iter().map(row).collect(),Err(value)=>error=Some(value.to_string()) },
        "aggregate"=>match wait(service.fetch_smart_list_with_relation_aggregates(&request,&[])) { Ok(values)=>rows=values.iter().map(row).collect(),Err(value)=>error=Some(value.to_string()) },
        "stream"=>match wait(service.fetch_stream(&request)) {
            Ok(mut stream)=>loop {
                match wait(std::future::poll_fn(|context|stream.as_mut().poll_next(context))) {
                    Some(Ok(chunk))=>rows.extend(chunk.rows.iter().map(row)),
                    Some(Err(value))=>{error=Some(value.to_string());break;},
                    None=>break,
                }
            },
            Err(value)=>error=Some(value.to_string()),
        },
        _=>match wait(service.fetch_all(&request)) { Ok(values)=>rows=values.iter().map(row).collect(),Err(value)=>error=Some(value.to_string()) },
    }
    output["rows"]=json!(rows);output["error"]=json!(error);output["logs"]=logs(&user_context);
    if input["failure"]!=true {
        user_context.clear_sql_logs();
        let independent=PurposedSelectQuery::new(SelectQuery::new("Order").project("id").limit(1).comment(secret),"independent query");
        let _=wait(service.fetch_all(&independent));
        output["independent_logs"]=logs(&user_context);
    }
    output
}
fn main() {
    for line in io::stdin().lock().lines() {
        let input:Json=serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}",observe(&input));
    }
}
