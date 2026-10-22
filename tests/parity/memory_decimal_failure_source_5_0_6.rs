// Appended observer; original runtime bodies remain byte-identical.
#[cfg(test)]
mod cpp_memory_decimal_failure_observer {
    use super::*;
    fn rows(values:Vec<Value>)->Vec<BTreeMap<String,Value>> {values.into_iter().map(|v|BTreeMap::from([("v".into(),v)])).collect()}
    fn output(result:Result<Vec<CompactRow>,DataServiceError<MemoryDataServiceError>>)->serde_json::Value {
        match result {
            Ok(rows)=>{let rows:Vec<BTreeMap<String,String>>=rows.into_iter().map(|r|r.into_map().into_iter().map(|(k,v)|(k,format!("{v:?}"))).collect()).collect();serde_json::json!({"rows":rows})},
            Err(e)=>serde_json::json!({"error":e.to_string()})
        }
    }
    #[test]
    fn decimal_failure() {
        for dataset in 0..6 {for op in 0..2 {
            let values=match dataset {
                0=>vec![Value::Decimal(Decimal::MAX),Value::Decimal(Decimal::MAX)],
                1=>vec![Value::Decimal(Decimal::MIN),Value::Decimal(Decimal::MIN)],
                2=>vec![Value::Decimal(Decimal::MAX),Value::I64(1)],
                3=>vec![Value::Decimal(Decimal::MAX),Value::Decimal(Decimal::MIN)],
                4=>vec![Value::Decimal(Decimal::MAX),Value::I64(1),Value::I64(-1)],
                _=>vec![Value::F64(f64::MAX),Value::F64(f64::INFINITY)]
            };
            let metadata=InMemoryMetadataStore::new().with_entity(teaql_core::EntityDescriptor::new("Item"));
            let mut service=MemoryDataService::new(metadata).with_rows("Item",rows(values));
            let query=if op==0 {SelectQuery::new("Item").sum("v","result")}else{SelectQuery::new("Item").avg("v","result")};
            let attempt=std::panic::catch_unwind(std::panic::AssertUnwindSafe(||service.fetch_all(&query)));
            let result=match attempt {Ok(result)=>output(result),Err(error)=>{
                let message=error.downcast_ref::<String>().map(String::as_str).or_else(||error.downcast_ref::<&str>().copied()).expect("string Decimal panic");
                assert_eq!(message,"Addition overflowed");serde_json::json!({"panic":message})
            }};
            let after=output(service.fetch_all(&SelectQuery::new("Item")));
            service.seed("Item",rows(vec![Value::I64(2)]));
            let replacement=output(service.fetch_all(&query));
            let inserted=service.insert(&InsertCommand::new("Item").value("v",4i64)).unwrap();
            let after_insert=output(service.fetch_all(&query));
            println!("MEMORY_DECIMAL_FAILURE_OBSERVATION {}",serde_json::json!({"case":format!("decimal_failure_{dataset}_{op}"),"result":result,"after":after,"replacement":replacement,"inserted":inserted,"after_insert":after_insert}));
        }}
    }
}
