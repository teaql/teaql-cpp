// Appended observer; original runtime bodies remain byte-identical.
#[cfg(test)]
mod cpp_memory_debug_failure_observer {
    use super::*;
    use std::fmt::Write;
    struct FailingWriter {text:String, marker:&'static str}
    impl std::fmt::Write for FailingWriter {
        fn write_str(&mut self,s:&str)->std::fmt::Result {
            self.text.push_str(s);
            if self.text.contains(self.marker) {panic!("injected formatter failure");}
            Ok(())
        }
    }
    fn service()->MemoryDataService {
        let metadata=InMemoryMetadataStore::new().with_entity(teaql_core::EntityDescriptor::new("Item"));
        MemoryDataService::new(metadata).with_rows("Item",vec![BTreeMap::from([("name".into(),Value::from("x".repeat(4096)))])])
    }
    #[test]
    fn debug_failure() {
        for pretty in [false,true] {
            let service=service();
            let guard=service.data.lock().unwrap();
            let debug=if pretty {format!("{service:#?}")}else{format!("{service:?}")};
            drop(guard);
            println!("MEMORY_DEBUG_FAILURE_OBSERVATION {}",serde_json::json!({"case":format!("locked_{}",u8::from(pretty)),"debug":debug,"rows":service.fetch_all(&SelectQuery::new("Item")).unwrap().len()}));
        }
        for (mode,marker) in ["MemoryDataService","Mutex { data:","loaded_relations: None }]}, poisoned:"].iter().enumerate() {
            let service=service();let mut writer=FailingWriter{text:String::new(),marker};
            let failed=std::panic::catch_unwind(std::panic::AssertUnwindSafe(||write!(&mut writer,"{service:?}"))).is_err();
            let after=match service.fetch_all(&SelectQuery::new("Item")) {Ok(rows)=>serde_json::json!({"rows":rows.len()}),Err(e)=>serde_json::json!({"error":e.to_string()})};
            println!("MEMORY_DEBUG_FAILURE_OBSERVATION {}",serde_json::json!({"case":format!("failure_{mode}"),"injected":failed,"after":after}));
        }
    }
}
