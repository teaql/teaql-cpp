#[cfg(test)]
mod cpp_relation_key_snapshot_observer {
    use super::*;
    use serde_json::{Value as Json, json};

    #[test]
    fn replay_key_snapshot_lengths() {
        let inputs: Vec<Json> =
            serde_json::from_str(include_str!("__TEAQL_KEY_SNAPSHOT_INPUTS__")).unwrap();
        for input in inputs {
            let parent_count = input["parents"].as_u64().unwrap();
            let key_count = input["keys"].as_u64().unwrap();
            let single = input["single"].as_bool().unwrap();
            let overwritten = input["overwritten"].as_bool().unwrap();
            let mut parents: Vec<CompactRow> = (0..parent_count)
                .map(|i| {
                    CompactRow::from_map(BTreeMap::from([
                        ("id".into(), Value::U64(if overwritten { 100 + i } else { i })),
                        ("agg".into(), Value::I64(-1)),
                    ]))
                })
                .collect();
            let keys: Vec<CompactRow> = (0..key_count)
                .map(|i| CompactRow::from_map(BTreeMap::from([("id".into(), Value::U64(i))])))
                .collect();
            let rows = (0..3_u64)
                .map(|i| {
                    CompactRow::from_map(BTreeMap::from([
                        ("parent".into(), Value::U64(i)),
                        ("value".into(), Value::U64(10 + i)),
                    ]))
                })
                .collect();
            let plan = RelationLoadPlan {
                parent_entity: "Parent".into(),
                relation_name: "items".into(),
                path: "items".into(),
                target_entity: "Child".into(),
                local_key: "id".into(),
                foreign_key: "parent".into(),
                many: true,
                query: None,
                children: vec![],
            };
            let aggregate = RelationAggregate {
                relation_name: "items".into(),
                alias: "agg".into(),
                query: SelectQuery::new("Child"),
                single_result: single,
            };
            attach_relation_aggregate_rows(&mut parents, &keys, &plan, &aggregate, rows);
            let output: Vec<_> = parents
                .iter()
                .map(|row| {
                    row.clone()
                        .into_map()
                        .iter()
                        .map(|(k, v)| (k.clone(), format!("{v:?}")))
                        .collect::<BTreeMap<_, _>>()
                })
                .collect();
            println!(
                "TEAQL_KEY_SNAPSHOT_JSON {}",
                json!({"parents": parent_count, "keys": key_count,
                    "single": single, "overwritten": overwritten,
                    "error": null, "rows": output})
            );
        }
    }
}
