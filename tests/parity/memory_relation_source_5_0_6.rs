// Appended observer; original runtime bodies remain byte-identical.
#[cfg(test)]
mod cpp_memory_relation_observer {
    use super::*;
    #[test]
    fn relations() {
        for mode in 0..20 { for single in [true,false] { for projected in [false,true] { for registered in [true,false] {
            let parent=teaql_core::EntityDescriptor::new("Parent").relation(teaql_core::RelationDescriptor::new("children","Child").local_key("id").foreign_key("pid").many());
            let mut metadata=InMemoryMetadataStore::new().with_entity(parent);
            if registered {metadata=metadata.with_entity(teaql_core::EntityDescriptor::new("Child"));}
            let parents:Vec<_>=(1u64..=3).map(|id| BTreeMap::from([("id".into(),Value::U64(id)),("name".into(),Value::from("parent"))])).collect();
            let children:Vec<_>=[(1u64,"b",2i64),(1,"a",3),(2,"a",5)].into_iter().map(|(id,kind,v)|BTreeMap::from([("pid".into(),Value::U64(id)),("kind".into(),Value::from(kind)),("v".into(),Value::I64(v))])).collect();
            let service=MemoryDataService::new(metadata).with_rows("Parent",parents).with_rows("Child",children);
            let mut child=SelectQuery::new("Ignored");
            child=match mode {
                1=>child.count("n"),2=>child.sum("v","sum"),3=>child.count("n").sum("v","sum"),
                4=>child.group_by("kind").count("n"),5=>child.group_by("kind"),6=>child.group_by("pid").count("n"),
                7=>child.count("n").having(Expr::gt("n",1i64)),8=>child.filter(Expr::gt("v",4i64)),
                9=>child.project("bad").project_expr("bad",Expr::function(ExprFunction::Count,[])).order_expr_desc(Expr::function(ExprFunction::Count,[])).offset(u64::MAX).limit(0).relation("missing"),
                10=>child.filter(Expr::value(1i64)),11=>child.having(Expr::value(1i64)),12=>child.sum("kind","sum"),
                13=>child.count("pid"),14=>child.group_by("kind").count("kind"),
                15=>child.count("n").having(Expr::gt("n",99i64)),
                19=>child.filter(Expr::value(1i64)).sum("kind","sum"),_=>child,
            };
            let mut query=SelectQuery::new(if mode==16 {"Missing"}else{"Parent"});
            if projected {query=query.project("name");}
            let first=RelationAggregate::new(if mode==17 {"missing"}else{"children"},if mode==18 {"id"}else{"result"},child,single);
            let mut aggregates=vec![first];
            if mode==18 {aggregates.push(RelationAggregate::new("children","again",SelectQuery::new("Ignored"),single));}
            let name=format!("relation_{mode}_{}_{}_{}",if single{1}else{0},if projected{1}else{0},if registered{1}else{0});
            let output=match service.fetch_all_with_relation_aggregates(&query,&aggregates) {
                Ok(rows)=>{let rows:Vec<BTreeMap<String,String>>=rows.into_iter().map(|r|r.into_map().into_iter().map(|(k,v)|(k,format!("{v:?}"))).collect()).collect();serde_json::json!({"case":name,"rows":rows})},
                Err(error)=>serde_json::json!({"case":name,"error":error.to_string()}),
            };
            println!("MEMORY_RELATION_OBSERVATION {}",output);
        }}}}
    }
}
