// Appended observer; original runtime bodies remain byte-identical.
#[cfg(test)]
mod cpp_memory_entries_observer {
    use super::*;
    #[derive(Debug,Clone,PartialEq,teaql_macros::TeaqlEntity)]
    #[teaql(entity="Order",table="orders")]
    struct Order {#[teaql(id)] id:u64,#[teaql(version)] version:i64,name:String}
    fn row()->BTreeMap<String,Value> {BTreeMap::from([("id".into(),Value::U64(1)),("version".into(),Value::I64(3)),("name".into(),Value::from("one"))])}
    fn service(child:bool)->MemoryDataService {
        let parent=teaql_core::EntityDescriptor::new("Order")
            .relation(teaql_core::RelationDescriptor::new("lines","OrderLine").local_key("id").foreign_key("order_id").many())
            .relation(teaql_core::RelationDescriptor::new("ghost","MissingChild").local_key("id").foreign_key("order_id").many());
        let mut metadata=InMemoryMetadataStore::new().with_entity(parent);
        if child {metadata=metadata.with_entity(teaql_core::EntityDescriptor::new("OrderLine"));}
        MemoryDataService::new(metadata).with_rows("Order",vec![row()]).with_rows("OrderLine",vec![BTreeMap::from([("order_id".into(),Value::U64(1))]),BTreeMap::from([("order_id".into(),Value::U64(1))])])
    }
    fn rows(rows:&[CompactRow])->serde_json::Value {
        let values:Vec<BTreeMap<String,String>>=rows.iter().map(|r|r.clone().into_map().into_iter().map(|(k,v)|(k,format!("{v:?}"))).collect()).collect();serde_json::json!(values)
    }
    fn error(e:DataServiceError<MemoryDataServiceError>)->serde_json::Value {
        let kind=match &e {DataServiceError::Runtime(_)=>"runtime",DataServiceError::Entity(_)=>"entity",DataServiceError::Executor(_)=>"executor"};
        serde_json::json!({"error_kind":kind,"error":e.to_string()})
    }
    fn list<T>(list:&SmartList<T>,values:serde_json::Value)->serde_json::Value {
        serde_json::json!({"rows":values,"loaded":list.is_loaded,"total":list.total_count,"aggregations":list.aggregations.len(),"summary":list.summary.len(),"facets":list.facets.len()})
    }
    fn emit(name:String,mut output:serde_json::Value) {output["case"]=serde_json::json!(name);println!("MEMORY_ENTRIES_OBSERVATION {}",output);}
    #[test]
    fn entries() {
        for dataset in 0..8 {for method in 0..4 {
            let mut service=service(true);
            let mut input=row();if dataset==2 {input.remove("name");}if dataset==3 {input.insert("name".into(),Value::I64(9));}
            service.seed("Order",if dataset==1 {vec![]}else{vec![input]});
            let mut query=SelectQuery::new(if dataset==4 {"Missing"}else{"Order"});
            if dataset==5 {query=query.filter(Expr::value(1i64));}if dataset==7 {query=query.project("id");}
            let aggregate=RelationAggregate::new(if dataset==6 {"missing"}else{"lines"},"total",SelectQuery::new("Ignored"),true);
            let output=if method<2 {
                let result=if method==0 {service.fetch_smart_list(&query)}else{service.fetch_smart_list_with_relation_aggregates(&query,&[aggregate])};
                match result {Ok(value)=>list(&value,rows(&value.data)),Err(e)=>error(e)}
            } else {
                let result=if method==2 {service.fetch_entities::<Order>(&query)}else{service.fetch_entities_with_relation_aggregates::<Order>(&query,&[aggregate])};
                match result {Ok(value)=>{let mapped:Vec<_>=value.data.iter().map(|v|CompactRow::from_map(BTreeMap::from([("id".into(),Value::U64(v.id)),("version".into(),Value::I64(v.version)),("name".into(),Value::from(v.name.clone()))]))).collect();list(&value,rows(&mapped))},Err(e)=>error(e)}
            };
            emit(format!("fetch_{dataset}_{method}"),output);
        }}
        for mode in 0..10 {
            let service=service(mode!=6);let mut parents=service.fetch_all(&SelectQuery::new("Order")).unwrap();
            if [5,6,9].contains(&mode) {parents.clear();}
            let mut aggregates=Vec::new();
            if mode!=0 && mode!=9 {
                aggregates.push(RelationAggregate::new(if mode==5 {"missing"}else{"lines"},if mode==7 {"id"}else{"first"},SelectQuery::new("Ignored"),true));
                if [2,3,7,8].contains(&mode) {
                    let query=if mode==3 {SelectQuery::new("Ignored").filter(Expr::value(1i64))}else{SelectQuery::new("Ignored")};
                    aggregates.push(RelationAggregate::new(if mode==2 {"missing"}else if mode==8 {"ghost"}else{"lines"},"second",query,true));
                }
            }
            let result=service.enhance_relation_aggregates(if [0,4,9].contains(&mode) {"Missing"}else{"Order"},&mut parents,&aggregates);
            let result=match result {Ok(())=>serde_json::json!({"ok":true}),Err(e)=>error(e)};
            emit(format!("enhance_{mode}"),serde_json::json!({"result":result,"rows":rows(&parents),"stored":rows(&service.fetch_all(&SelectQuery::new("Order")).unwrap())}));
        }
        let service=service(true);let mut shared=service.clone();let mut changed=row();changed.insert("name".into(),Value::from("replacement"));shared.seed("Order",vec![changed]);
        emit("shared_seed".into(),serde_json::json!({"rows":rows(&service.fetch_all(&SelectQuery::new("Order")).unwrap()),"shared_rows":rows(&shared.fetch_all(&SelectQuery::new("Order")).unwrap())}));
    }
}
