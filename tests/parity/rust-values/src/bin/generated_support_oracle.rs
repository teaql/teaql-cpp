//! Unchanged public generated-support calls with a recording runtime fixture.
use std::{collections::BTreeMap, future::Future, io::{self, BufRead}, pin::Pin,
    sync::{Arc, Mutex, atomic::{AtomicBool, AtomicUsize, Ordering}},
    task::{Context, Poll, Wake, Waker}};
use serde_json::{json, Value as J};
use teaql_core::*;
use teaql_core::request::{self, QueryOptions, QuerySelection};
use teaql_runtime::{execute_facets, restrict_facet_to_outer_query, EntityDataService, PurposedQuery,
    PurposedSelectQuery, RuntimeError, RuntimeModule, TeaqlRuntime, UserContext};
use teaql_data_service::{MutationExecutor, QueryExecutor, SchemaProvider};
#[path = "../fixture_value.rs"] mod fixture_value;
#[allow(dead_code)] #[path = "../fixture_sql.rs"] mod fixture_sql;
#[allow(dead_code)] #[path = "../fixture_query_keys.rs"] mod fixture_query_keys;

fn items<'a>(j: &'a J, key: &str) -> &'a [J] {
    j[key].as_array().map(Vec::as_slice).unwrap_or(&[])
}
fn options(j: &J) -> QueryOptions {
    let mut o = QueryOptions::default();
    o.comment = j["comment"].as_str().map(str::to_owned);
    o.raw_sql = j["raw_sql"].as_str().map(str::to_owned);
    o.raw_sql_search_criteria = items(j,"raw_criteria").iter().map(|v|v.as_str().unwrap().into()).collect();
    o.dynamic_properties = items(j,"dynamic").iter().map(|p|request::RawDynamicProperty::new(
        p["alias"].as_str().unwrap(), request::UnsafeRawSqlSegment::trusted(p["sql"].as_str().unwrap()))).collect();
    o.raw_projections = items(j,"raw_projection").iter().map(|p|request::RawProjection::new(
        p["alias"].as_str().unwrap(), request::UnsafeRawSqlSegment::trusted(p["sql"].as_str().unwrap()))).collect();
    o.object_group_bys = items(j,"object_groups").iter().map(|g|request::ObjectGroupBy::new(
        g["property"].as_str().unwrap(),g["storage"].as_str().unwrap(),selection(&g["selection"]))).collect();
    o.relation_aggregates = items(j,"aggregates").iter().map(|a|request::RelationAggregate::new(
        a["relation"].as_str().unwrap(),a["alias"].as_str().unwrap(),selection(&a["selection"]),a["single"].as_bool().unwrap())).collect();
    o.facets = items(j,"facets").iter().map(|f|request::FacetRequest::new(
        f["name"].as_str().unwrap(),f["relation"].as_str().unwrap(),selection(&f["selection"]),f["all"].as_bool().unwrap())).collect();
    o
}
fn selection(j: &J) -> QuerySelection {
    let mut s = QuerySelection::new(fixture_query_keys::query(&j["query"]));
    s.query_options = options(&j["options"]);
    s.child_enhancements = items(j,"children").iter().map(selection).collect();
    s.relation_selections = items(j,"selections").iter().map(|r|request::RelationSelection::new(
        r["name"].as_str().unwrap(),selection(&r["selection"]))).collect();
    s.relation_filters = items(j,"filters").iter().map(|r|request::RelationFilter::new(
        r["name"].as_str().unwrap(),selection(&r["selection"]))).collect();
    s
}
fn entity(j: &J) -> EntityDescriptor {
    let mut e = fixture_sql::entity(j);
    e.data_service = j["service"].as_str().map(str::to_owned);
    e.audit_value_max_len = j["audit_max"].as_u64().map(|v|v as usize);
    for r in items(j,"relations") {
        let mut relation = RelationDescriptor::new(r["name"].as_str().unwrap(),r["target"].as_str().unwrap());
        if let Some(s)=r["local"].as_str(){relation.local_key=s.into();}
        if let Some(s)=r["foreign"].as_str(){relation.foreign_key=s.into();}
        relation.many=r["many"].as_bool().unwrap_or(false);
        relation.attach=r["attach"].as_bool().unwrap_or(true);
        relation.delete_missing=r["delete_missing"].as_bool().unwrap_or(true);
        e.relations.push(relation);
    }
    e
}
fn trace(nodes: &[TraceNode]) -> J {
    json!(nodes.iter().map(|n|json!({"kind":n.kind as u8,"entity":n.entity_type,
        "id":n.entity_id,"comment":n.comment})).collect::<Vec<_>>())
}
fn aggregates(a: &[teaql_core::RelationAggregate]) -> J {
    json!(a.iter().map(|a|json!({"relation":a.relation_name,"alias":a.alias,
        "query":format!("{:?}",a.query),"single":a.single_result})).collect::<Vec<_>>())
}
fn error(e: &RuntimeError) -> J {
    let kind = match e { RuntimeError::Graph(_)=>"Graph", RuntimeError::MissingRelation{..}=>"MissingRelation",
        RuntimeError::Policy(_)=>"Policy", _=>panic!("unexpected error") };
    json!({"kind":kind,"text":e.to_string()})
}
fn list_out(list: &SmartList<CompactRow>) -> J {
    json!({"rows":list.data.iter().map(compact_row_to_json_value).collect::<Vec<_>>(),
        "total":list.total_count,"loaded":list.is_loaded,
        "aggregations":record_to_json_value(&list.aggregations),"summary":record_to_json_value(&list.summary),
        "facets":list.facets.iter().map(|(k,v)|(k.clone(),list_out(v))).collect::<BTreeMap<_,_>>()})
}
struct Gate<'a>(&'a AtomicBool);
impl Future for Gate<'_> {
    type Output = ();
    fn poll(self: Pin<&mut Self>, _: &mut Context<'_>) -> Poll<()> {
        if self.0.load(Ordering::Relaxed) { Poll::Ready(()) } else { Poll::Pending }
    }
}
struct Activity<'a> { count: &'a AtomicUsize, complete: bool }
impl Drop for Activity<'_> { fn drop(&mut self) { if !self.complete { self.count.fetch_add(1,Ordering::Relaxed); } } }
struct Runtime {
    context: UserContext, calls: Mutex<Vec<J>>, gate: AtomicBool,
    pending_at: Option<usize>, fail_at: Option<usize>, cancelled: AtomicUsize,
}
impl TeaqlRuntime for Runtime {
    fn user_context(&self) -> &UserContext { &self.context }
    async fn fetch_facet_smart_list(&self, entity: &str, query: &PurposedSelectQuery,
        relation_aggregates: &[teaql_core::RelationAggregate], chain: Vec<TraceNode>)
        -> Result<SmartList<CompactRow>,RuntimeError> {
        let index = { let mut calls=self.calls.lock().unwrap();let index=calls.len();
            calls.push(json!({"entity":entity,"query":format!("{:?}",query.as_query()),
                "aggregates":aggregates(relation_aggregates),"trace":trace(&chain)}));index };
        let mut activity = Activity{count:&self.cancelled, complete:false};
        if self.pending_at==Some(index) { Gate(&self.gate).await; }
        activity.complete=true;
        if self.fail_at==Some(index) { return Err(RuntimeError::Policy("facet denied".into())); }
        let mut rows=SmartList::new(vec![CompactRow::from_map(BTreeMap::from([
            ("call".into(),Value::U64(index as u64)),("payload".into(),Value::List(vec![Value::Text("retained".into())]))]))]);
        rows.total_count=Some(index as u64+10);
        rows.aggregations.insert("count".into(),Value::U64(index as u64+1));
        rows.summary.insert("name".into(),Value::Text(entity.into()));
        rows.add_facet("nested",SmartList::new(vec![]));
        Ok(rows)
    }
}
struct NoWake;
impl Wake for NoWake { fn wake(self: Arc<Self>) {} }

struct Schema(BTreeMap<String, Arc<EntityDescriptor>>);
impl SchemaProvider for Schema {
    fn get_entity(&self, name: &str) -> Option<Arc<EntityDescriptor>> { self.0.get(name).cloned() }
}
struct SqlRuntime<E> { context: UserContext, executor: E, calls: Mutex<Vec<J>> }
impl<E: QueryExecutor + MutationExecutor + Send + Sync> TeaqlRuntime for SqlRuntime<E> {
    fn user_context(&self) -> &UserContext { &self.context }
    async fn fetch_facet_smart_list(&self, entity: &str, query: &PurposedSelectQuery,
        relation_aggregates: &[teaql_core::RelationAggregate], chain: Vec<TraceNode>)
        -> Result<SmartList<CompactRow>,RuntimeError> {
        self.calls.lock().unwrap().push(json!({"entity":entity,"query":format!("{:?}",query.as_query()),
            "aggregates":aggregates(relation_aggregates),"trace":trace(&chain)}));
        let service=EntityDataService::for_executor(&self.context,entity,&self.executor).with_trace_context(chain);
        service.fetch_smart_list_with_relation_aggregates(query,relation_aggregates).await
            .map_err(|e|RuntimeError::Graph(e.to_string()))
    }
}
fn sqlite(j: &J) -> J {
    let mut module=RuntimeModule::new();for descriptor in items(j,"descriptors") { module=module.descriptor(entity(descriptor)); }
    let mut context=module.into_context();context.disable_sql_log();
    let schema=Schema(context.all_entities().into_iter().map(|e|(e.name.clone(),Arc::new(e.clone()))).collect());
    let db=rusqlite::Connection::open_in_memory().unwrap();
    db.execute_batch("CREATE TABLE parents(id INTEGER PRIMARY KEY,label TEXT); CREATE TABLE children(id INTEGER PRIMARY KEY,parent_id INTEGER,label TEXT); INSERT INTO parents VALUES(1,'one'),(2,'two'); INSERT INTO children VALUES(10,1,'first'),(11,1,'second'),(20,2,'third');").unwrap();
    let executor=teaql_sql::SqlDataServiceExecutor::new(teaql_provider_sqlite::SqliteDialect,
        teaql_provider_sqlite::SqliteMutationExecutor::from_connection(db),schema);
    let runtime=SqlRuntime{context,executor,calls:Mutex::new(vec![])};
    let outer=fixture_query_keys::query(&j["outer"]);let options=options(&j["options"]);
    let waker=Waker::from(Arc::new(NoWake));let mut cx=Context::from_waker(&waker);
    let mut future=Box::pin(execute_facets(&runtime,&outer,&options));
    let result=loop{if let Poll::Ready(r)=future.as_mut().poll(&mut cx){break r}};
    let mut out=match result{Ok(rows)=>json!({"state":"ready","facets":rows.iter().map(|(k,v)|(k.clone(),list_out(v))).collect::<BTreeMap<_,_>>() }),Err(e)=>json!({"state":"ready","error":error(&e)})};
    out["calls"]=json!(*runtime.calls.lock().unwrap());out["cancelled"]=json!(0);out["outer"]=json!(format!("{outer:?}"));out
}
fn formatted<T: std::fmt::Debug>(value: &T) -> J {
    json!({"compact":format!("{value:?}"),"pretty":format!("{value:#?}"),
        "lower_hex":format!("{value:x?}"),"upper_hex":format!("{value:X?}"),
        "pretty_lower_hex":format!("{value:#x?}"),"pretty_upper_hex":format!("{value:#X?}"),
        "width_precision":format!("{value:>40.2?}"),"fancy":format!("{value:🧊^+18.3?}"),
        "zero_pad":format!("{value:+012?}")})
}

fn evaluate(j: J) -> J {
    if j["op"]=="sqlite" { return sqlite(&j); }
    let mut module=RuntimeModule::new();for descriptor in items(&j,"descriptors") { module=module.descriptor(entity(descriptor)); }
    let runtime=Runtime{context:module.into_context(),calls:Mutex::new(vec![]),gate:AtomicBool::new(false),
        pending_at:j["pending_at"].as_u64().map(|v|v as usize),fail_at:j["fail_at"].as_u64().map(|v|v as usize),cancelled:AtomicUsize::new(0)};
    let outer=fixture_query_keys::query(&j["outer"]);
    let mut out=match j["op"].as_str().unwrap() {
        "restrict" => match restrict_facet_to_outer_query(&runtime,selection(&j["selection"]),&outer,j["relation"].as_str().unwrap()) {
            Ok(s)=>json!({"query":format!("{:?}",s.query),"materialized":format!("{:?}",s.clone().into_query()),
                "aggregates":aggregates(&request::runtime_relation_aggregates(&s.query_options)),
                "selections":s.relation_selections.len(),"filters":s.relation_filters.len(),"children":s.child_enhancements.len()}),
            Err(e)=>json!({"error":error(&e)}),
        },
        "purpose" => {
            let purpose=j["purpose"].as_str().unwrap();
            let generic=PurposedQuery::new(outer.clone(),purpose);
            let selected=std::panic::catch_unwind(std::panic::AssertUnwindSafe(||PurposedSelectQuery::new(outer.clone(),purpose)));
            let mut result=json!({"generic":format!("{:?}",generic.inner),"purpose":generic.purpose});
            result["select"]=match selected {
                Ok(q)=>{
                    if j["protocol"]==true {
                        let copied=q.clone();
                        result["debug"]=formatted(&q);
                        result["clone_debug"]=formatted(&copied);
                        result["borrowed"]=json!(format!("{:?}",q.as_query()));
                        result["clone_into"]=json!(format!("{:?}",copied.into_query()));
                    }
                    json!(format!("{:?}",q.into_query()))
                },
                Err(_)=>J::Null,
            };
            result
        },
        "execute" => {
            let options=options(&j["options"]);
            let mut future=Box::pin(execute_facets(&runtime,&outer,&options));
            let waker=Waker::from(Arc::new(NoWake));let mut cx=Context::from_waker(&waker);
            match future.as_mut().poll(&mut cx) {
                Poll::Ready(result)=>match result { Ok(rows)=>json!({"state":"ready","facets":rows.iter().map(|(k,v)|(k.clone(),list_out(v))).collect::<BTreeMap<_,_>>()}),Err(e)=>json!({"state":"ready","error":error(&e)}) },
                Poll::Pending=>{
                    let first_calls=runtime.calls.lock().unwrap().clone();
                    if j["cancel"]==true {drop(future);json!({"state":"cancelled","first_calls":first_calls})}
                    else { runtime.gate.store(true,Ordering::Relaxed);
                        match future.as_mut().poll(&mut cx) { Poll::Ready(Ok(rows))=>json!({"state":"resumed","first_calls":first_calls,"facets":rows.iter().map(|(k,v)|(k.clone(),list_out(v))).collect::<BTreeMap<_,_>>() }),
                            Poll::Ready(Err(e))=>json!({"state":"resumed","first_calls":first_calls,"error":error(&e)}),Poll::Pending=>panic!("unexpected second suspension") }
                    }
                },
            }
        },
        _=>panic!("unknown operation"),
    };
    out["calls"]=json!(*runtime.calls.lock().unwrap());
    out["cancelled"]=json!(runtime.cancelled.load(Ordering::Relaxed));
    out["outer"]=json!(format!("{outer:?}"));out
}
fn main() {
    std::panic::set_hook(Box::new(|_|{}));unsafe{std::env::set_var("TEAQL_LOG_ENDPOINT","off");}
    for line in io::stdin().lock().lines(){println!("{}",evaluate(serde_json::from_str(&line.unwrap()).unwrap()));}
}
