#[cfg(test)]
mod cpp_enhanced_aggregate_observer {
    use super::*;
    use crate::{InMemoryMetadataStore, UserContext};
    use serde_json::{Value as Json, json};
    use std::{future::Future, io, sync::{Arc, Mutex}, task::{Context, Poll, Wake, Waker}};
    use teaql_core::*;
    use teaql_data_service::*;

    struct Model(CompactRow);
    impl TeaqlEntity for Model {
        const ENTITY_NAME: &'static str = "Parent";
        fn entity_descriptor() -> EntityDescriptor { EntityDescriptor::new("Parent") }
    }
    impl Entity for Model {
        fn from_compact_row(row: CompactRow) -> Result<Self, EntityError> { Ok(Self(row)) }
        fn into_values(self) -> MutationValues { self.0.into_map().into() }
    }
    struct Executor { requests: Mutex<Vec<Json>>, empty: bool, failure: bool, protocol: bool, membership: bool }
    impl DataServiceExecutor for Executor {
        type Error = io::Error;
        fn capabilities(&self) -> DataServiceCapabilities { DataServiceCapabilities::default() }
    }
    impl QueryExecutor for Executor {
        async fn query(&self, request: QueryRequest) -> Result<QueryResult, Self::Error> {
            let mut event = json!({"entity":request.query.entity,
                "inline":request.query.relation_aggregates.len(),"aggregates":request.query.aggregates.len()});
            if self.membership { event["filter"]=json!(request.query.filter.as_ref().map(|expr|format!("{expr:?}"))); }
            if self.protocol {
                event["projection"] = json!(request.query.projection);
                event["limit"] = json!(request.query.slice.and_then(|slice|slice.limit));
                event["offset"] = json!(request.query.slice.map(|slice|slice.offset));
            }
            self.requests.lock().unwrap().push(event);
            if request.query.entity == "Child" && self.failure { return Err(io::Error::other("aggregate failed")); }
            let rows = if request.query.entity == "Parent" && !self.empty {
                let mut fields=BTreeMap::from([("id".into(),Value::U64(1))]);
                if self.membership { fields.insert("bucket".into(),Value::U64(1)); }
                vec![CompactRow::from_map(fields)]
            } else if request.query.entity == "Child" {
                vec![CompactRow::from_map(BTreeMap::from([("parent_id".into(),Value::U64(1)),("metric".into(),Value::U64(2))]))]
            } else if request.query.entity == "Lookup" {
                vec![CompactRow::from_map(BTreeMap::from([("id".into(),Value::U64(1)),("label".into(),Value::Text("dimension".into()))]))]
            } else { vec![] };
            Ok(QueryResult { rows, metadata:ExecutionMetadata::unrecorded_query(0) })
        }
    }
    impl MutationExecutor for Executor {
        async fn mutate(&self, _:MutationRequest) -> Result<MutationResult,Self::Error> { panic!("unexpected mutation"); }
    }
    struct CursorStore(Arc<Mutex<Vec<Json>>>);
    #[async_trait::async_trait]
    impl crate::ContinuousPageCursorStore for CursorStore {
        async fn get(&self,_:&str,offset:u64)->Result<Option<crate::ContinuousPageCursor>,String> {
            self.0.lock().unwrap().push(json!({"op":"get","offset":offset}));
            Ok(None)
        }
        async fn put(&self,cursor:crate::ContinuousPageCursor)->Result<(),String> {
            self.0.lock().unwrap().push(json!({"op":"put","entity":cursor.entity,
                "boundary":format!("{:?}",cursor.boundary),"size":cursor.page_size,"next":cursor.next_offset}));
            Ok(())
        }
        async fn invalidate(&self,_:&str)->Result<(),String> { Ok(()) }
    }
    struct NoWake;
    impl Wake for NoWake { fn wake(self:Arc<Self>) {} }
    fn wait<F:Future>(future:F) -> F::Output {
        let mut future=std::pin::pin!(future);
        let waker=Waker::from(Arc::new(NoWake)); let mut context=Context::from_waker(&waker);
        match future.as_mut().poll(&mut context) { Poll::Ready(value)=>value, Poll::Pending=>panic!("unexpected pending") }
    }
    fn rows_output(rows:impl IntoIterator<Item=CompactRow>,loaded:Option<bool>,total:Option<u64>,protocol:bool)->Json {
        let mut output=json!({"loaded":loaded,"rows":rows.into_iter().map(|row|row.into_map().iter()
            .map(|(key,value)|(key.clone(),format!("{value:?}"))).collect::<BTreeMap<_,_>>()).collect::<Vec<_>>()});
        if protocol { output["total"]=json!(total); }
        output
    }
    fn evaluate(input:&Json) -> Json {
        let membership=input["membership"]==true;
        let mut parent=EntityDescriptor::new("Parent").property(PropertyDescriptor::new("id",DataType::U64).id());
        if membership { parent=parent.property(PropertyDescriptor::new("bucket",DataType::U64)); }
        let mut relation=RelationDescriptor::new("children","Child");
        relation.local_key=if membership {"bucket"} else {"id"}.into(); relation.foreign_key="parent_id".into(); relation.many=true;
        parent.relations.push(relation);
        let child=EntityDescriptor::new("Child")
            .property(PropertyDescriptor::new("id",DataType::U64).id())
            .property(PropertyDescriptor::new("parent_id",DataType::U64));
        let mut metadata=InMemoryMetadataStore::new().with_entity(parent).with_entity(child);
        if membership { metadata=metadata.with_entity(EntityDescriptor::new("Lookup")
            .property(PropertyDescriptor::new("id",DataType::U64).id())
            .property(PropertyDescriptor::new("label",DataType::Text))); }
        let mut user_context=UserContext::new().with_metadata(metadata);
        user_context.disable_sql_log();
        let cursor_events=Arc::new(Mutex::new(vec![]));
        if input["cursor_probe"]==true {
            user_context.set_continuous_page_cursor_store(Arc::new(CursorStore(cursor_events.clone())));
        }
        let protocol=input["protocol"]==true;
        if protocol {
            user_context.observe_continuous_page("UNEXECUTED",Some("seed".into()));
            user_context.observe_id_set("UNEXECUTED",Some(77));
        }
        let executor=Executor{requests:Mutex::new(vec![]),empty:input["empty"]==true,failure:input["failure"]==true,protocol,membership};
        let repository=EntityDataService::for_executor(&user_context,"Parent",&executor);
        let mut query=SelectQuery::new("Parent").comment("inspect parent totals");
        let aggregate=RelationAggregate::new(if input["missing"]==true {"missing"} else {"children"},
            input["alias"].as_str().unwrap(),SelectQuery::new("Child").aggregate(Aggregate::count("metric")), input["single"]==true);
        let placement=input["placement"].as_str().unwrap();
        if placement=="inline" || placement=="both" { query.relation_aggregates.push(aggregate.clone()); }
        let separate=if placement=="separate" || placement=="both" {vec![aggregate]} else {vec![]};
        if membership { query.object_group_bys.push(ObjectGroupBy{
            property_name:if input["group"]=="overwrite" {"bucket"} else {"dimension"}.into(),
            storage_field:"bucket".into(),query:SelectQuery::new("Lookup")}); }
        match input["feature"].as_str() {
            Some("projection")=>query=query.projects(["id"]),
            Some("continuous_first")=>query=query.order_asc("id").page(0,1).optimize_for_continuous_page_fetch(),
            Some("continuous_miss")=>query=query.order_asc("id").page(1,1).optimize_for_continuous_page_fetch(),
            Some("id_set")=>query=query.page(0,1).optimize_pagination_with_id_set_config("entry_protocol",600,2),
            Some("dynamic")=>query=query.select_dynamic_fields(teaql_core::dynamic_fields::DynamicFieldSelection::All),
            _=>{},
        }
        let purposed=PurposedSelectQuery::new(query,"verify aggregate transport");
        let result=match input["entry"].as_str().unwrap() {
            "rows"=>wait(repository.fetch_all(&purposed)).map(|rows|rows_output(rows,None,None,protocol)),
            "rows_owned"=>wait(repository.fetch_all_owned(purposed)).map(|rows|rows_output(rows,None,None,protocol)),
            "rows_aggregates"=>wait(repository.fetch_all_with_relation_aggregates_internal(purposed.as_query(),&separate)).map(|rows|rows_output(rows,None,None,protocol)),
            "list"=>wait(repository.fetch_smart_list(&purposed)).map(|list|rows_output(list.data,Some(list.is_loaded),list.total_count,protocol)),
            "list_aggregates"=>wait(repository.fetch_smart_list_with_relation_aggregates(&purposed,&separate)).map(|list|rows_output(list.data,Some(list.is_loaded),list.total_count,protocol)),
            "typed"=>wait(repository.fetch_entities::<Model>(&purposed)).map(|list|rows_output(list.data.into_iter().map(|model|model.0),Some(list.is_loaded),list.total_count,protocol)),
            "typed_aggregates"=>wait(repository.fetch_entities_with_relation_aggregates_internal::<Model>(purposed.as_query(),&separate)).map(|list|rows_output(list.data.into_iter().map(|model|model.0),Some(list.is_loaded),list.total_count,protocol)),
            "enhanced"=>wait(repository.fetch_enhanced_entities::<Model>(&purposed)).map(|list|rows_output(list.data.into_iter().map(|model|model.0),Some(list.is_loaded),list.total_count,protocol)),
            "owned"=>wait(repository.fetch_enhanced_entities_with_relation_aggregates_owned::<Model>(purposed,&separate)).map(|list|rows_output(list.data.into_iter().map(|model|model.0),Some(list.is_loaded),list.total_count,protocol)),
            _=>wait(repository.fetch_enhanced_entities_with_relation_aggregates::<Model>(&purposed,&separate)).map(|list|rows_output(list.data.into_iter().map(|model|model.0),Some(list.is_loaded),list.total_count,protocol)),
        };
        let mut output=match result {
            Ok(output)=>output,
            Err(error)=>json!({"error":error.to_string()}),
        };
        if protocol {
            output["continuous"]=json!(user_context.continuous_page_plan());
            output["cursor"]=json!(user_context.continuous_page_cursor_id());
            output["id_plan"]=json!(user_context.id_set_plan());
            output["id_count"]=json!(user_context.id_set_count());
        }
        if input["cursor_probe"]==true { output["cursor_events"]=json!(*cursor_events.lock().unwrap()); }
        output["requests"]=json!(*executor.requests.lock().unwrap()); output
    }
    #[test]
    fn replay_query_owned_and_separate_aggregates() {
        let inputs:Vec<Json>=serde_json::from_str(include_str!("__TEAQL_ENHANCED_AGGREGATE_INPUTS__")).unwrap();
        for input in inputs { println!("TEAQL_ENHANCED_AGGREGATE_JSON {}",evaluate(&input)); }
    }
}
