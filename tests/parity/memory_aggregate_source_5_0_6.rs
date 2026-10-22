// Appended observer; original runtime bodies remain byte-identical.
#[cfg(test)]
mod cpp_memory_aggregate_observer {
    use super::*;
    #[test]
    fn aggregates() {
        let datasets = vec![
            ("empty", vec![]),
            ("null", vec![Value::Null]),
            ("signed", vec![Value::I64(-3),Value::I64(1)]),
            ("unsigned", vec![Value::U64(4),Value::U64(7)]),
            ("mixed", vec![Value::I64(1),Value::U64(2)]),
            ("decimal", vec![Value::Decimal(Decimal::new(125,2)),Value::Decimal(Decimal::new(275,2))]),
            ("float", vec![Value::F64(1.5),Value::F64(2.5)]),
            ("text", vec![Value::from("b"),Value::from("a")]),
            ("single_bool", vec![Value::Bool(true)]),
            ("bool_pair", vec![Value::Bool(true),Value::Bool(false)]),
            ("typed_null", vec![Value::TypedNull(teaql_core::DataType::I64)]),
            ("u64_overflow", vec![Value::U64(u64::MAX),Value::U64(1)]),
            ("i64_underflow", vec![Value::I64(i64::MIN),Value::I64(-1)]),
            ("nonfinite", vec![Value::F64(f64::NAN),Value::F64(f64::INFINITY)]),
            ("missing", vec![Value::Null]),
            ("single_text", vec![Value::from("a")]),
        ];
        let functions = [AggregateFunction::Count,AggregateFunction::Sum,AggregateFunction::Avg,
            AggregateFunction::Min,AggregateFunction::Max,AggregateFunction::Stddev,
            AggregateFunction::StddevPop,AggregateFunction::VarSamp,AggregateFunction::VarPop,
            AggregateFunction::BitAnd,AggregateFunction::BitOr,AggregateFunction::BitXor,
            AggregateFunction::Count];
        for (name, values) in datasets {
            let rows: Vec<CompactRow> = values.into_iter().map(|value| CompactRow::from_map(
                if name == "missing" {BTreeMap::new()} else {BTreeMap::from([("v".into(),value)])}
            )).collect();
            let metadata = InMemoryMetadataStore::new().with_entity(teaql_core::EntityDescriptor::new("Item"));
            let service = MemoryDataService::new(metadata).with_rows("Item", rows);
            for (index,function) in functions.iter().enumerate() {
                let case = format!("{name}_{index}");
                let query = SelectQuery::new("Item").aggregate(Aggregate::new(*function,if index==12 {"*"} else {"v"},"result"));
                let output = match service.fetch_all(&query) {
                    Ok(rows) => serde_json::json!({"case":case,"value":format!("{:?}",rows[0].get("result").unwrap())}),
                    Err(error) => serde_json::json!({"case":case,"error":error.to_string()}),
                };
                println!("MEMORY_AGGREGATE_OBSERVATION {}",output);
            }
        }
    }
}
