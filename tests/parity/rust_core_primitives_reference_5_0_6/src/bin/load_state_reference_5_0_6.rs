// Historical observation drivers only; production code comes from the locked teaql-core.
use serde_json::{Value, json};
use std::io::{self, BufRead};
use teaql_core::FieldLayout;

fn strings(value: &Value) -> Vec<&str> {
    value
        .as_array()
        .unwrap()
        .iter()
        .map(|item| item.as_str().unwrap())
        .collect()
}

fn evaluate(input: &Value) -> Value {
    let indexes: Vec<_> = input["indexes"]
        .as_array()
        .unwrap()
        .iter()
        .map(|item| {
            (
                item[0].as_str().unwrap(),
                item[1].as_u64().unwrap() as usize,
            )
        })
        .collect();
    let mappings: Vec<_> = input["mappings"]
        .as_array()
        .unwrap()
        .iter()
        .map(|item| {
            (
                item[0].as_str().unwrap(),
                item[1].as_str().unwrap(),
                item[2].as_str().unwrap(),
            )
        })
        .collect();
    let relations = strings(&input["relations"]);
    let expected = strings(&input["expected_members"]);
    match FieldLayout::from_generated(
        input["entity"].as_str().unwrap(),
        input["revision"].as_str().unwrap(),
        &indexes,
        &mappings,
        &relations,
        &expected,
    ) {
        Err(error) => json!({"error": error}),
        Ok(layout) => {
            let queries = strings(&input["lookups"]);
            json!({"entity": layout.entity(), "revision": layout.revision(), "count": layout.field_count(),
                "indexes": queries.iter().map(|name| layout.index(name)).collect::<Vec<_>>(),
                "relations": queries.iter().map(|name| layout.is_relation(name)).collect::<Vec<_>>(),
                "shared_identity": layout.shared_entity_name().as_ptr() == layout.shared_entity_name().as_ptr()})
        }
    }
}

fn data_type(value: u64) -> teaql_core::DataType {
    use teaql_core::DataType::*;
    [
        Bool, I64, U64, F64, Decimal, Text, LargeText, Json, Date, Timestamp,
    ][value as usize]
}

fn native_value(input: &Value) -> teaql_core::Value {
    use teaql_core::Value as V;
    match input["kind"].as_str().unwrap() {
        "null" => V::Null,
        "typed_null" => V::TypedNull(data_type(input["datatype"].as_u64().unwrap())),
        "bool" => V::Bool(input["value"].as_bool().unwrap()),
        "i64" => V::I64(input["value"].as_str().unwrap().parse().unwrap()),
        "u64" => V::U64(input["value"].as_str().unwrap().parse().unwrap()),
        "f64" => V::F64(input["value"].as_str().unwrap().parse().unwrap()),
        "decimal" => V::Decimal(input["value"].as_str().unwrap().parse().unwrap()),
        "text" => V::Text(input["value"].as_str().unwrap().to_owned()),
        "date" => V::Date(
            teaql_core::Value::Text(input["value"].as_str().unwrap().to_owned())
                .try_date()
                .unwrap(),
        ),
        "timestamp" => V::Timestamp(teaql_core::time::Timestamp(
            input["value"].as_str().unwrap().parse().unwrap(),
        )),
        "json" => V::Json(input["value"].clone()),
        "list" => V::List(
            input["value"]
                .as_array()
                .unwrap()
                .iter()
                .map(native_value)
                .collect(),
        ),
        "object" => V::Object(
            input["value"]
                .as_object()
                .unwrap()
                .iter()
                .map(|(k, v)| (k.clone(), native_value(v)))
                .collect(),
        ),
        _ => panic!("unsupported readonly property fixture"),
    }
}

fn schema(
    input: &Value,
) -> Result<std::sync::Arc<teaql_core::dynamic_properties::DynamicPropertyDefinitions>, String> {
    teaql_core::dynamic_properties::DynamicPropertyDefinitions::new(
        input.as_array().unwrap().iter().map(|item| {
            (
                item[0].as_str().unwrap().to_owned(),
                data_type(item[1].as_u64().unwrap()),
            )
        }),
    )
}

fn schema_probe(input: &Value) -> Value {
    use std::sync::Arc;
    let left = match schema(&input["fields"]) {
        Ok(v) => v,
        Err(e) => return json!({"error":e}),
    };
    let validations: Vec<_> = input["values"]
        .as_array()
        .unwrap()
        .iter()
        .map(|item| {
            left.validate_value(
                item["name"].as_str().unwrap(),
                &native_value(&item["input"]),
            )
            .err()
        })
        .collect();
    let merged = match schema(&input["merge"]) {
        Err(error) => json!({"error":error}),
        Ok(right) => {
            match teaql_core::dynamic_properties::DynamicPropertyDefinitions::merge(&left, &right) {
                Err(error) => json!({"error":error}),
                Ok(next) => {
                    json!({"left":Arc::ptr_eq(&left,&next),"right":Arc::ptr_eq(&right,&next),
                "types": next.types().iter().map(|(name,ty)| json!([name,*ty as u64])).collect::<Vec<_>>()})
                }
            }
        }
    };
    json!({"types":left.types().iter().map(|(name,ty)| json!([name,*ty as u64])).collect::<Vec<_>>(),
        "validation_errors":validations, "merge":merged})
}

fn snapshot_probe(input: &Value) -> Value {
    use std::sync::Arc;
    use teaql_core::{LoadedSnapshot, eval::LoadState};
    let count = input["count"].as_u64().unwrap() as usize;
    let names: Vec<String> = (0..count)
        .map(|i| match i {
            0 => "id".to_owned(),
            1 => "version".to_owned(),
            _ => format!("field_{i}"),
        })
        .collect();
    let indexes: Vec<_> = names
        .iter()
        .enumerate()
        .map(|(i, n)| (n.as_str(), i))
        .collect();
    let mappings: Vec<_> = names
        .iter()
        .map(|n| (n.as_str(), n.as_str(), n.as_str()))
        .collect();
    let expected: Vec<_> = names.iter().map(String::as_str).collect();
    let layout =
        FieldLayout::from_generated("Item", "v1", &indexes, &mappings, &["lines"], &expected)
            .unwrap();
    if input["kind"] == "row" {
        return row_probe(input, layout);
    }
    let selected = strings(&input["selected"]);
    let build = || {
        if input["full"].as_bool().unwrap() {
            LoadedSnapshot::fully_loaded(layout.clone()).into_shared()
        } else {
            LoadedSnapshot::projection(layout.clone(), selected.iter().copied()).into_shared()
        }
    };
    let first = build();
    let duplicate = build();
    let mut current = first.clone();
    // Keep published states alive, matching C++'s explicit result-Context lifetime.
    let mut held = vec![first.clone(), duplicate.clone()];
    let mut actions = vec![];
    for action in input["actions"].as_array().unwrap() {
        match LoadedSnapshot::with_loaded(
            &current,
            action[0].as_str().unwrap(),
            action[1].as_bool().unwrap(),
        ) {
            Err(error) => actions.push(json!({"error":error})),
            Ok(next) => {
                actions.push(json!({"same_previous":Arc::ptr_eq(&current,&next),"same_initial":Arc::ptr_eq(&first,&next),"shared_overflow":match (current.overflow(),next.overflow()){(Some(a),Some(b))=>std::ptr::eq(a,b),_=>false},"shared_names":match (current.selected_names(),next.selected_names()){(Some(a),Some(b))=>std::ptr::eq(a,b),_=>false}}));
                held.push(next.clone());
                current = next;
            }
        }
    }
    if !input["schema"].is_null() {
        let definitions = match schema(&input["schema"]) {
            Ok(v) => v,
            Err(e) => return json!({"schema_error":e}),
        };
        current = LoadedSnapshot::with_dynamic_property_definitions(&current, definitions);
    }
    let lookups = strings(&input["lookups"]);
    let mut overflow: Vec<_> = current.overflow().into_iter().flatten().copied().collect();
    overflow.sort();
    let mut selected_names: Vec<_> = current
        .selected_names()
        .into_iter()
        .flatten()
        .cloned()
        .collect();
    selected_names.sort();
    let state = LoadState::Indexed(current.clone());
    json!({"same_initial_build":Arc::ptr_eq(&first,&duplicate),"actions":actions,
        "bits":current.bits().to_string(),"overflow":overflow,"overflow_present":current.overflow().is_some(),
        "selected_names":selected_names,"names_present":current.selected_names().is_some(),
        "loaded":lookups.iter().map(|n|current.is_loaded(n)).collect::<Vec<_>>(),
        "schema":current.dynamic_property_definitions().map(|d| d.types().iter().map(|(n,t)| json!([n,*t as u64])).collect::<Vec<_>>()),
        "serialize_error":serde_json::to_value(&state).err().map(|e|e.to_string())})
}

fn load_state_probe(input: &Value) -> Value {
    use teaql_core::{LoadedSnapshot, eval::LoadState};
    let count = input["count"].as_u64().unwrap() as usize;
    let names: Vec<String> = (0..count)
        .map(|i| match i {
            0 => "id".to_owned(),
            1 => "version".to_owned(),
            _ => format!("field_{i}"),
        })
        .collect();
    let indexes: Vec<_> = names
        .iter()
        .enumerate()
        .map(|(i, n)| (n.as_str(), i))
        .collect();
    let mappings: Vec<_> = names
        .iter()
        .map(|n| (n.as_str(), n.as_str(), n.as_str()))
        .collect();
    let expected: Vec<_> = names.iter().map(String::as_str).collect();
    let layout =
        FieldLayout::from_generated("Item", "v1", &indexes, &mappings, &["lines"], &expected)
            .unwrap();
    let selected = strings(&input["selected"]);
    let mut state = match input["mode"].as_str().unwrap() {
        "partial" => LoadState::Partial(selected.iter().map(|s| s.to_string()).collect()),
        "partial_compact" => LoadState::PartialCompact(
            selected
                .iter()
                .map(|s| std::borrow::Cow::Owned(s.to_string()))
                .collect(),
        ),
        "shared_columns" => LoadState::SharedColumns(
            selected
                .iter()
                .map(|s| s.to_string())
                .collect::<Vec<_>>()
                .into(),
        ),
        "full" => LoadState::FullyLoaded,
        "indexed" => LoadState::Indexed(
            LoadedSnapshot::projection(layout.clone(), selected.iter().copied()).into_shared(),
        ),
        _ => LoadState::NotLoaded,
    };
    let initial = state.clone();
    let lookups = strings(&input["lookups"]);
    let mut errors = vec![];
    for action in input["actions"].as_array().unwrap() {
        let field = action[0].as_str().unwrap();
        let result = if action[1].as_bool().unwrap() {
            state.mark_loaded(field)
        } else {
            state.mark_unloaded(field)
        };
        errors.push(result.err());
    }
    let before: Vec<_> = lookups.iter().map(|f| state.is_loaded(f)).collect();
    let original: Vec<_> = lookups.iter().map(|f| initial.is_loaded(f)).collect();
    let other = if input["different_layout"].as_bool().unwrap() {
        FieldLayout::from_generated("Item", "v1", &indexes, &mappings, &["lines"], &expected)
            .unwrap()
    } else {
        layout
    };
    match state.into_indexed(other) {
        Err(error) => {
            json!({"before":before,"original":original,"actions":errors,"conversion_error":error})
        }
        Ok(next) => {
            json!({"before":before,"original":original,"actions":errors,"conversion_error":Value::Null,
            "after":lookups.iter().map(|f|next.is_loaded(f)).collect::<Vec<_>>(),
            "serialize_error":serde_json::to_value(&next).err().map(|e|e.to_string())})
        }
    }
}

fn row_probe(input: &Value, layout: std::sync::Arc<FieldLayout>) -> Value {
    use std::sync::Arc;
    use teaql_core::{CompactRow, Value as V, eval::LoadState};
    let names: Arc<[String]> = strings(&input["selected"])
        .into_iter()
        .map(str::to_owned)
        .collect();
    let mut rows = vec![
        CompactRow::new(names.clone(), vec![V::I64(11); names.len()]),
        CompactRow::new(names.clone(), vec![V::I64(22); names.len()]),
        CompactRow::new(Arc::from(["id".to_owned()]), vec![V::I64(33)]),
    ];
    CompactRow::share_layouts(&mut rows);
    if input["relation_loaded"].as_bool().unwrap_or(false) {
        let mut shapes = teaql_core::RelationShapeCache::default();
        rows[0].mark_relation_loaded("lines", &mut shapes);
        rows[1].mark_relation_loaded("lines", &mut shapes);
    }

    let first = rows[0].indexed_load_state(layout.clone());
    let second = rows[1].indexed_load_state(layout.clone());
    let sparse = rows[2].indexed_load_state(layout.clone());
    let snapshot = |state: &LoadState| match state {
        LoadState::Indexed(state) => state.clone(),
        _ => panic!("expected indexed row state"),
    };
    // A second immutable type identity consumes the same result shape.
    let count = input["count"].as_u64().unwrap() as usize;
    let field_names: Vec<String> = (0..count)
        .map(|i| match i {
            0 => "id".to_owned(),
            1 => "version".to_owned(),
            _ => format!("field_{i}"),
        })
        .collect();
    let indexes: Vec<_> = field_names
        .iter()
        .enumerate()
        .map(|(i, n)| (n.as_str(), i))
        .collect();
    let mappings: Vec<_> = field_names
        .iter()
        .map(|n| (n.as_str(), n.as_str(), n.as_str()))
        .collect();
    let expected: Vec<_> = field_names.iter().map(String::as_str).collect();
    let other_layout =
        FieldLayout::from_generated("Other", "v1", &indexes, &mappings, &["lines"], &expected)
            .unwrap();
    let other = rows[0].indexed_load_state(other_layout.clone());
    let repeated = rows[1].indexed_load_state(other_layout);
    let lookups = strings(&input["lookups"]);
    let availability = |state: &LoadState| {
        lookups
            .iter()
            .map(|name| state.is_loaded(name))
            .collect::<Vec<_>>()
    };
    let initial = availability(&first);
    let mut changed = first.clone();
    changed.mark_unloaded("id").unwrap();
    let mut out = json!({
        "shared_shape": Arc::ptr_eq(&rows[0].shared_layout(), &rows[1].shared_layout()),
        "sparse_shape_same": Arc::ptr_eq(&rows[0].shared_layout(), &rows[2].shared_layout()),
        "shared_state": Arc::ptr_eq(&snapshot(&first), &snapshot(&second)),
        "other_shared": Arc::ptr_eq(&snapshot(&other), &snapshot(&repeated)),
        "other_distinct": !Arc::ptr_eq(&snapshot(&first), &snapshot(&other)),
        "initial": initial, "changed": availability(&changed),
        "unchanged": availability(&second), "sparse": availability(&sparse),
    });
    if let Some(name) = names.first() {
        rows[0].insert(name.clone(), V::I64(99));
        out["value_isolated"] = json!(rows[1].get(name) == Some(&V::I64(22)));
    } else {
        out["value_isolated"] = json!(true);
    }
    out
}

fn main() {
    for line in io::stdin().lock().lines() {
        let input: Value = serde_json::from_str(&line.unwrap()).unwrap();
        let output = match input["kind"].as_str() {
            Some("snapshot") | Some("row") => snapshot_probe(&input),
            Some("schema") => schema_probe(&input),
            Some("load_state") => load_state_probe(&input),
            _ => evaluate(&input),
        };
        println!("{}", output);
    }
}
