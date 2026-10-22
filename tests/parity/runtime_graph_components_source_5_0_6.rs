// Supplemental observation of the unchanged Rust graph model's derived contract.
#[cfg(test)]
mod cpp_dynamic_graph_components {
    use super::GraphNode;
    use std::collections::HashMap;
    use teaql_core::dynamic_fields::{DynamicFieldDefinitions, DynamicFieldValues};
    use teaql_core::{DataType, Value};

    fn child(revision: &str, selected: bool) -> GraphNode {
        let definitions = DynamicFieldDefinitions::new(
            "Child", revision, [("long_dynamic_title".into(), DataType::Text)],
        ).unwrap();
        let mut values = HashMap::new();
        if selected {
            values.insert("long_dynamic_title".into(), Value::Null);
        }
        let mut node = GraphNode::new("Child");
        node.dynamic_fields = Some(Box::new(
            DynamicFieldValues::from_values(definitions, values).unwrap(),
        ));
        node
    }

    #[test]
    fn recursive_dynamic_components() {
        let selected = child("revision_one", true);
        let changed = child("revision_two", true);
        let fields = selected.dynamic_fields.as_ref().unwrap();
        let changed_fields = changed.dynamic_fields.as_ref().unwrap();
        assert_eq!(fields.values(), changed_fields.values());
        assert_eq!(fields.selected_codes(), changed_fields.selected_codes());
        assert_ne!(selected, changed);
        let empty = child("revision_one", false);
        let empty_fields = empty.dynamic_fields.as_ref().unwrap();
        assert_eq!(fields.definitions(), empty_fields.definitions());
        assert!(empty_fields.values().is_empty());
        assert!(empty_fields.selected_codes().is_empty());
        assert_ne!(selected, empty);
        assert_ne!(GraphNode::new("Child"), empty);
        let mut root = GraphNode::new("Parent").relation("children", selected);
        let retained = root.clone();
        assert_eq!(root, retained);
        assert_eq!(format!("{root:?}"), format!("{retained:?}"));
        let nested = &mut root.relations.get_mut("children").unwrap()[0];
        nested.dynamic_fields.as_mut().unwrap().assign(
            "long_dynamic_title", Value::Text("changed child value with allocated storage".into()),
        ).unwrap();
        assert_ne!(root, retained);
        let nested = &mut root.relations.get_mut("children").unwrap()[0];
        nested.dynamic_fields.as_mut().unwrap().delete("long_dynamic_title").unwrap();
        assert_eq!(*nested, empty);
        drop(root);
        let retained_fields = retained.relations["children"][0].dynamic_fields.as_ref().unwrap();
        assert_eq!(retained_fields.values().len(), 1);
        assert_eq!(retained_fields.values()["long_dynamic_title"], Value::Null);
        assert_eq!(retained_fields.selected_codes().len(), 1);
    }
}
