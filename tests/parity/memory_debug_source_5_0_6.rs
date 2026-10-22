// Appended observer; original runtime bodies remain byte-identical.
#[cfg(test)]
mod cpp_memory_debug_observer {
    use super::*;
    #[test]
    fn debug() {
        for kind in 0..3 {for pretty in [false,true] {
            let error=match kind {0=>MemoryDataServiceError::Poisoned,1=>MemoryDataServiceError::UnsupportedExpression("quoted \"text\"\n".into()),_=>MemoryDataServiceError::UnsupportedAggregate("quoted \"text\"\n".into())};
            let debug=if pretty {format!("{error:#?}")}else{format!("{error:?}")};
            println!("MEMORY_DEBUG_OBSERVATION {}",serde_json::json!({"case":format!("error_{kind}_{}",u8::from(pretty)),"debug":debug,"display":error.to_string()}));
        }}
        for mode in 0..3 {for pretty in [false,true] {
            let metadata=InMemoryMetadataStore::new().with_entity(teaql_core::EntityDescriptor::new("Item"));
            let mut service=MemoryDataService::new(metadata);
            if mode!=0 {service.seed("Item",vec![BTreeMap::from([("id".into(),Value::I64(1)),("version".into(),Value::I64(i64::MAX)),("name".into(),Value::from("line\nquoted\""))])]);}
            if mode==2 {
                let mut update=UpdateCommand::new("Item",1i64);update.expected_version=Some(i64::MAX);
                assert!(std::panic::catch_unwind(std::panic::AssertUnwindSafe(||service.update(&update))).is_err());
            }
            let debug=if pretty {format!("{service:#?}")}else{format!("{service:?}")};
            println!("MEMORY_DEBUG_OBSERVATION {}",serde_json::json!({"case":format!("service_{mode}_{}",u8::from(pretty)),"debug":debug}));
        }}
    }
}
