use serde_json::{Value as J, json};
use std::{collections::BTreeMap, io::{self, BufRead}, sync::{Arc,Mutex}, future::Future, task::{Context,Poll,Wake,Waker}};
use teaql_core::*;
use teaql_data_service::*;
use teaql_runtime::{RuntimeModule, EntityDataService};
struct Mock { requests: Mutex<Vec<J>> }
impl DataServiceExecutor for Mock {
    type Error=io::Error;
    fn capabilities(&self)->DataServiceCapabilities { DataServiceCapabilities::default() }
}
impl QueryExecutor for Mock {
    async fn query(&self, request:QueryRequest)->Result<QueryResult,Self::Error> {
        self.requests.lock().unwrap().push(json!({"entity":request.query.entity,
            "filter":request.query.filter.as_ref().map(|e|format!("{e:?}"))}));
        let rows = if request.query.entity == "Parent" { vec![] } else {
            (1..=2).map(|i|CompactRow::from_map(BTreeMap::from([
                ("id".into(),Value::U64(i*10)), ("owner".into(),Value::U64(i)),
                ("link".into(),Value::U64(i)), ("label".into(),Value::Text(format!("child{i}")))
            ]))).collect()
        };
        Ok(QueryResult{rows, metadata:ExecutionMetadata::unrecorded_query(2)})
    }
}
impl MutationExecutor for Mock {
    async fn mutate(&self,_:MutationRequest)->Result<MutationResult,Self::Error>{panic!("unexpected mutation")}
}
struct Noop;impl Wake for Noop { fn wake(self:Arc<Self>){} }
fn wait<T>(future:impl Future<Output=T>)->T {
    let mut future=Box::pin(future);let waker=Waker::from(Arc::new(Noop));let mut cx=Context::from_waker(&waker);
    loop {if let Poll::Ready(result)=future.as_mut().poll(&mut cx){return result}}
}
fn descriptor(name:&str)->EntityDescriptor {
    let mut result=EntityDescriptor::new(name);
    for field in ["id","owner","link"] {
        let mut property=PropertyDescriptor::new(field,DataType::U64);property.is_id=field=="id";result.properties.push(property);
    }
    result.properties.push(PropertyDescriptor::new("label",DataType::Text));result
}
fn relation(name:&str,target:&str,local:&str,foreign:&str)->RelationDescriptor {
    let mut result=RelationDescriptor::new(name,target);result.local_key=local.into();result.foreign_key=foreign.into();result.many=true;result
}
fn evaluate(j:&J)->J {
    let nested=false;
    let collision=true;
    let inverse=false;
    let mut parent=descriptor("Parent");
    for name in ["link","remaining"] {let mut rel=relation(name,"Child","link","owner");rel.many=j["many"].as_bool().unwrap();parent.relations.push(rel);}
    let mut child=descriptor("Child");
    for name in ["link","remaining"] {child.relations.push(relation(name,"Leaf","link","owner"));}
    child.relations.push(relation("parent","Parent","owner","link"));child.relations.push(relation("tags","Leaf","id","owner"));
    let mut context=RuntimeModule::new().descriptor(parent).descriptor(child).descriptor(descriptor("Leaf")).into_context();
    context.disable_sql_log();
    let executor=Mock{requests:Mutex::new(vec![])};
    let repo=EntityDataService::for_executor(&context,"Parent",&executor);
    let mut query=SelectQuery::new("Parent");
    for name in [if collision {"link"}else{"remaining"}] {
        let mut child_query=SelectQuery::new("Child").comment("Inspect sibling relation keys");child_query.purpose=Some("Verify independent hydration".into());
        if j["sliced"].as_bool().unwrap() { child_query=child_query.offset(1).limit(1).order_desc("id"); }
        if j["facets"].as_bool().unwrap() {
            let facet=SelectQuery::new("Leaf").comment("Observe relation facet values");
            child_query.facets.push(teaql_core::request::FacetRequest::new("tagsFacet","tags",facet,j["include_all"].as_bool().unwrap()));
        }
        if nested {
            for leaf_name in [if collision {"link"}else{"remaining"}, "remaining"] {
                let mut leaf=SelectQuery::new("Leaf").comment("Inspect descendant relation keys");leaf.purpose=Some("Verify descendant hydration".into());
                child_query.relations.push(RelationLoad::with_query(leaf_name,leaf));
            }
        }
        if inverse {
            let mut target=SelectQuery::new("Parent").comment("Inspect explicit inverse target");target.purpose=Some("Preserve filtered inverse projection".into());
            child_query.relations.push(RelationLoad::with_query("parent",target));
        }
        query.relations.push(RelationLoad::with_query(name,child_query));
    }
    assert!(!repo.__cpp_relation_plans(&[],Some(&query)).unwrap().is_empty());
    let mut parents=(1..=2).map(|i|CompactRow::from_map(BTreeMap::from([
        ("id".into(),Value::U64(i)),("link".into(),if j["keys"].as_u64().unwrap()==0 {Value::U64(i)} else {Value::TypedNull(DataType::U64)})
    ]))).collect::<Vec<_>>();
    if j["keys"].as_u64().unwrap()==2 { for parent in &mut parents { parent.remove("link"); } }
    let result=wait(repo.__cpp_embedded_query(&mut parents,&query));
    json!({"error":result.err().map(|e|e.to_string()),
        "parents":parents.iter().map(|r|r.clone().into_map().iter().map(|(k,v)|(k.clone(),format!("{v:?}"))).collect::<BTreeMap<_,_>>()).collect::<Vec<_>>(),
        "facets":parents.iter().map(|r|r.loaded_relation("link").map(|l|l.facets.iter().map(|(name,list)|(name.clone(),list.data.iter().map(|r|r.clone().into_map().iter().map(|(k,v)|(k.clone(),format!("{v:?}"))).collect::<BTreeMap<_,_>>()).collect::<Vec<_>>())).collect::<BTreeMap<_,_>>())).collect::<Vec<_>>(),
        "requests":executor.requests.lock().unwrap().clone()})
}
fn main(){for line in io::stdin().lock().lines(){println!("{}",evaluate(&serde_json::from_str::<J>(&line.unwrap()).unwrap()))}}
