// Appended observer; original runtime bodies remain byte-identical.
#[cfg(test)]
mod cpp_memory_mutation_observer {
    use super::*;
    fn outcome(result: Result<u64,DataServiceError<MemoryDataServiceError>>) -> serde_json::Value {
        match result {Ok(count)=>serde_json::json!({"count":count}),Err(error)=>serde_json::json!({"error":error.to_string()})}
    }
    fn snapshot(service:&MemoryDataService, entity:&str) -> serde_json::Value {
        match service.fetch_all(&SelectQuery::new(entity)) {
            Ok(rows)=> {
                let rows: Vec<BTreeMap<String,String>>=rows.into_iter().map(|r|r.into_map().into_iter().map(|(k,v)|(k,format!("{v:?}"))).collect()).collect();
                serde_json::json!({"rows":rows})
            },Err(error)=>serde_json::json!({"error":error.to_string()})
        }
    }
    #[test]
    fn mutations() {
        let versions=vec![None,Some(Value::Null),Some(Value::I64(-3)),Some(Value::I64(0)),Some(Value::I64(3)),Some(Value::U64(3)),Some(Value::from("3")),Some(Value::I64(i64::MAX)),Some(Value::I64(i64::MIN)),Some(Value::I64(i64::MAX-1))];
        for named in 0..2 {
            let (id_field,version_field)=if named==0 {("id","version")}else{("pk","revision")};
            for (v,version) in versions.iter().enumerate() {
                for identity in 0..3 {
                    for op in 0..19 {
                        let mut descriptor=teaql_core::EntityDescriptor::new("Item");
                        if named==1 {descriptor=descriptor.property(teaql_core::PropertyDescriptor::new(id_field,teaql_core::DataType::I64).id()).property(teaql_core::PropertyDescriptor::new(version_field,teaql_core::DataType::I64).version());}
                        let metadata=InMemoryMetadataStore::new().with_entity(descriptor);
                        let mut row=BTreeMap::from([(id_field.into(),Value::I64(1)),("name".into(),Value::from("before"))]);
                        if let Some(version)=version {row.insert(version_field.into(),version.clone());}
                        let mut service=MemoryDataService::new(metadata).with_rows("Item",vec![row]);
                        let id=match identity {0=>Value::I64(1),1=>Value::U64(1),_=>Value::I64(2)};
                        let result=std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
                            if op<6 || op==18 {
                                let expected=if op==18 {Some(3)}else{[None,Some(3),Some(-3),Some(i64::MAX),Some(i64::MIN),Some(0)][op]};
                                let mut command=UpdateCommand::new("Item",id.clone()).value("name","after");command.expected_version=expected;
                                if op==18 {command=command.value(version_field,42i64);}
                                service.update(&command)
                            }else if op<13 {
                                let expected=if op<11 {[None,Some(3),Some(-3),Some(i64::MAX),Some(i64::MIN)][op-6]}else{[None,Some(3)][op-11]};
                                let mut command=DeleteCommand::new("Item",id.clone());command.expected_version=expected;command.soft_delete=op<11;
                                service.delete(&command)
                            }else {service.recover(&RecoverCommand::new("Item",id.clone(),[3,-3,i64::MAX,i64::MIN,0][op-13]))}
                        }));
                        let panicked=result.is_err();
                        let result=match result {
                            Ok(result)=>outcome(result),
                            Err(error)=>{let message=error.downcast_ref::<String>().map(String::as_str).or_else(||error.downcast_ref::<&str>().copied()).expect("string arithmetic panic");serde_json::json!({"panic":message})}
                        };
                        let mut output=serde_json::json!({"case":format!("mutation_{named}_{v}_{identity}_{op}"),"result":result,"after":snapshot(&service,"Item")});
                        if panicked {
                            service.seed("Item",vec![BTreeMap::from([("name".into(),Value::from("replacement"))])]);
                            output["after_seed"]=snapshot(&service,"Item");
                            output["poison_actions"]=serde_json::json!([
                                outcome(service.insert(&InsertCommand::new("Item"))),outcome(service.update(&UpdateCommand::new("Item",1i64))),
                                outcome(service.delete(&DeleteCommand::new("Item",1i64))),outcome(service.recover(&RecoverCommand::new("Item",1i64,3))),snapshot(&service,"Missing")]);
                        }
                        println!("MEMORY_MUTATION_OBSERVATION {}",output);
                    }
                }
            }
        }
        for mode in 0..4 {
            let metadata=InMemoryMetadataStore::new().with_entity(teaql_core::EntityDescriptor::new("Item"));
            let service=MemoryDataService::new(metadata);
            let command=if mode==3 {InsertCommand::new("Item")}else{InsertCommand::new(if mode==2 {"Missing"}else{"Item"}).value("id",1i64).value("version",3i64)};
            let first=outcome(service.insert(&command));
            let second=if mode==1 {Some(outcome(service.insert(&command)))}else{None};
            println!("MEMORY_MUTATION_OBSERVATION {}",serde_json::json!({"case":format!("insert_{mode}"),"first":first,"second":second,"after":snapshot(&service,"Item")}));
        }
    }
}
