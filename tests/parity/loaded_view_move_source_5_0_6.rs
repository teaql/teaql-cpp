// Appended only to a disposable, byte-checked copy of original entity_runtime.rs.
impl EntityRuntimeState {
    #[doc(hidden)]
    pub fn __cpp_loaded_view_move_506(input: serde_json::Value) -> serde_json::Value {
        use serde_json::{Value as Json, json};
        use teaql_core::dynamic_fields::{DynamicFieldDefinitions, DynamicFieldValues};
        use teaql_core::{CompactRow, DataType};
        fn load(state: &mut EntityRuntimeState, mode: u64, marker: i64) {
            if mode & 1 != 0 {
                state.set_original_compact_row("Item", CompactRow::from(std::collections::BTreeMap::from([
                    ("id".to_owned(), Value::U64(7)),
                    ("version".to_owned(), Value::I64(marker)),
                ])));
            }
            if mode & 2 != 0 {
                let definitions = DynamicFieldDefinitions::new("Item", "v1",
                    vec![("count".to_owned(), DataType::I64)]).unwrap();
                let fields = DynamicFieldValues::from_values(definitions,
                    std::collections::HashMap::from([("count".to_owned(), Value::I64(marker))])).unwrap();
                state.install_loaded_dynamic_fields(fields);
            }
        }
        fn describe(state: &EntityRuntimeState) -> Json {
            json!({"binding":state.inner.shared.get().is_some(),
                "ledger":state.inner.get().is_some(),
                "view":state.json_view_id(), "comment":state.get_comment(),
                "row":state.original_snapshot().map(|s|s.iter().map(|(k,v)|
                    (k.clone(),v.to_json_value())).collect::<serde_json::Map<_,_>>()),
                "dynamic":state.loaded_dynamic_fields().map(|s|s.values().iter().map(|(k,v)|
                    (k.clone(),v.to_json_value())).collect::<serde_json::Map<_,_>>())})
        }
        let root = EntityRuntimeState::default();
        if input["frozen"] == true {
            let mut graph = EntityGraphBuilder::default();
            graph.install(10, 100_i64);
            root.freeze_graph(graph).unwrap();
        }
        let mut receiver = if input["weak"] == true {
            EntityRuntimeState::fresh_with_weak_graph(&root)
        } else { EntityRuntimeState::fresh_with_shared_graph(&root) }.with_json_view(3);
        let mut source = EntityRuntimeState::default().with_json_view(17);
        load(&mut receiver, input["receiver_load"].as_u64().unwrap(), 11);
        load(&mut source, input["source_load"].as_u64().unwrap(), 22);
        if input["receiver_active"] == true { receiver.set_comment("receiver".to_owned()); }
        if input["source_active"] == true { source.set_comment("source".to_owned()); }
        let before = describe(&receiver);
        let mut retained = Some(receiver);
        let result = if input["copy"] == true {
            retained.as_ref().unwrap().clone().with_loaded_view_from(&source)
        } else { retained.take().unwrap().with_loaded_view_from(&source) };
        let after = describe(&result);
        let source_after = describe(&source);
        // Replacing the source view must not alter the transferred private values.
        load(&mut source, 3, 99);
        let detached = describe(&result);
        let mut graph = EntityGraphBuilder::default();
        graph.install(29, 808_i64);
        let publish = result.freeze_graph(graph);
        let published = publish.is_ok();
        let rejected_value = publish.err().map(|builder| {
            let destination = EntityRuntimeState::default();
            destination.freeze_graph(builder).unwrap();
            *destination.resolve_entity::<i64>(29).unwrap()
        });
        let mut root_builder = EntityGraphBuilder::default();
        root_builder.install(31, 909_i64);
        let root_published = root.freeze_graph(root_builder).is_ok();
        result.set_comment("result".to_owned());
        json!({"before":before,"after":after,"source_after":source_after,"detached":detached,
            "published":published,"rejected_value":rejected_value,"root_published":root_published,
            "root_29":root.resolve_entity::<i64>(29),"root_31":root.resolve_entity::<i64>(31),
            "retained_comment":retained.as_ref().map(|s|s.get_comment()),
            "source_comment_after_result_write":source.get_comment()})
    }
}
