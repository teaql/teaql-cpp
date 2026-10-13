use serde_json::{json, Value as J};
use std::io::{self, BufRead};
use teaql_core::*;
use teaql_data_service::*;
use teaql_runtime::{EntityDataService, RelationLoadPlan, RuntimeModule};

struct Unused;
impl DataServiceExecutor for Unused {
    type Error = io::Error;
    fn capabilities(&self) -> DataServiceCapabilities { panic!("planning queried capabilities") }
}
impl QueryExecutor for Unused {
    async fn query(&self, _: QueryRequest) -> Result<QueryResult, Self::Error> {
        panic!("planning performed query I/O")
    }
}
impl MutationExecutor for Unused {
    async fn mutate(&self, _: MutationRequest) -> Result<MutationResult, Self::Error> {
        panic!("planning performed mutation I/O")
    }
}
fn snapshot(q: &SelectQuery) -> J {
    json!({"entity":q.entity, "projection":q.projection, "groups":q.group_by,
        "aggregates":q.aggregates.iter().map(|a|json!([a.function as u8,a.field,a.alias])).collect::<Vec<_>>(),
        "facets":q.facets.len(), "relations":q.relations.len(),
        "partition":q.partition_by, "filtered":q.filter.is_some(),
        "slice":q.slice.as_ref().map(|s|json!([s.offset,s.limit])),
        "order":q.order_by.iter().map(|o|json!([o.field,o.direction as u8,o.expr.is_some()])).collect::<Vec<_>>()})
}
fn evaluate(input: &J) -> J {
    let mut descriptor = EntityDescriptor::new("Child");
    if input["identity"].as_bool().unwrap() {
        let mut id = PropertyDescriptor::new("child_id", DataType::U64);
        id.is_id = true;
        descriptor.properties.push(id);
    }
    let context = RuntimeModule::new().descriptor(descriptor).into_context();
    let executor = Unused;
    let repo = EntityDataService::for_executor(&context, "Root", &executor);
    let mut q = SelectQuery::new("OriginalTarget").project("score");
    for field in input["groups"].as_array().unwrap() {
        q.group_by.push(field.as_str().unwrap().to_owned());
    }
    if input["aggregate"].as_bool().unwrap() { q = q.count("total"); }
    if input["slice"].as_bool().unwrap() { q = q.offset(1).limit(2); }
    q.partition_by = Some("original_partition".into());
    match input["order"].as_u64().unwrap() {
        1 => q = q.order_desc("owner"),
        2 => q = q.order_expr_asc(Expr::column("owner")),
        3 => q = q.order_desc("child_id"),
        4 => q = q.order_desc("score"),
        5 => q = q.order_desc("owner").order_desc("score"),
        _ => {}
    }
    q.relations.push(RelationLoad::new("nested"));
    if input["facets"].as_bool().unwrap() {
        q.facets.push(teaql_core::request::FacetRequest::new("facet", "nested", SelectQuery::new("FacetTarget"), false));
    }
    let plan = RelationLoadPlan {
        parent_entity:"Root".into(), relation_name:"children".into(), path:"children".into(),
        target_entity:"Child".into(), local_key:"id".into(), foreign_key:"owner".into(),
        many:true, query:Some(q), children:vec![]
    };
    let mut parents = vec![];
    if input["parents"].as_u64().unwrap() == 1 {
        parents.push(CompactRow::from_map([("id".into(),Value::U64(7))].into()));
    }
    if input["parents"].as_u64().unwrap() != 0 {
        parents.push(CompactRow::from_map([("id".into(),Value::Null)].into()));
        parents.push(CompactRow::from_map([("id".into(),Value::TypedNull(DataType::U64))].into()));
    }
    json!({"normal":snapshot(&repo.__cpp_relation_plan_query(&plan,&parents,false)),
        "compact":snapshot(&repo.__cpp_relation_plan_query(&plan,&parents,true)),
        "base":snapshot(&repo.__cpp_relation_base_query(&plan)),
        "caller":snapshot(plan.query.as_ref().unwrap())})
}
fn main() {
    for line in io::stdin().lock().lines() {
        let input: J = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", evaluate(&input));
    }
}
