// Appended observer; original runtime bodies remain byte-identical.
#[cfg(test)]
mod cpp_memory_grouping_observer {
    use super::*;
    fn values() -> Vec<Value> {
        vec![Value::Null,Value::TypedNull(teaql_core::DataType::I64),Value::Bool(true),Value::Bool(false),
            Value::I64(1),Value::U64(1),Value::I64(-1),Value::F64(1.0),Value::F64(-0.0),Value::F64(0.0),
            Value::Decimal(Decimal::new(10,1)),Value::Decimal(Decimal::new(100,2)),Value::from("a"),Value::from("a"),
            Value::Object(BTreeMap::from([("x".into(),Value::I64(1))])),Value::Object(BTreeMap::from([("y".into(),Value::I64(2))])),
            Value::List(vec![Value::I64(1)]),Value::List(vec![Value::I64(2)]),
            Value::Json(serde_json::json!({"x":1})),Value::Json(serde_json::json!({"x":2})),
            Value::Date("1970-01-01".parse().unwrap()),Value::Timestamp(teaql_core::time::Timestamp(0))]
    }
    fn row(value: &Value) -> CompactRow { CompactRow::from_map(BTreeMap::from([("k".into(),value.clone())])) }
    fn emit(name: String, result: Result<Vec<CompactRow>,DataServiceError<MemoryDataServiceError>>) {
        let output = match result {
            Ok(rows) => {
                let rows: Vec<BTreeMap<String,String>> = rows.into_iter().map(|r| r.into_map().into_iter().map(|(k,v)| (k,format!("{v:?}"))).collect()).collect();
                serde_json::json!({"case":name,"rows":rows})
            },
            Err(error) => serde_json::json!({"case":name,"error":error.to_string()}),
        };
        println!("MEMORY_GROUPING_OBSERVATION {}", output);
    }
    fn service(child: bool) -> MemoryDataService {
        let parent = teaql_core::EntityDescriptor::new("Parent").relation(
            teaql_core::RelationDescriptor::new("children","Child").local_key("k").foreign_key("k").many());
        let mut metadata = InMemoryMetadataStore::new().with_entity(parent);
        if child { metadata = metadata.with_entity(teaql_core::EntityDescriptor::new("Child")); }
        MemoryDataService::new(metadata)
    }
    fn relation(service: &MemoryDataService, single: bool) -> Result<Vec<CompactRow>,DataServiceError<MemoryDataServiceError>> {
        let aggregate = RelationAggregate::new("children","total",SelectQuery::new("Ignored"),single);
        service.fetch_all_with_relation_aggregates(&SelectQuery::new("Parent"), &[aggregate])
    }
    #[test]
    fn grouping() {
        let values = values(); assert_eq!(values.len(),22);
        for (i,a) in values.iter().enumerate() {
            for (j,b) in values.iter().enumerate() {
                let service = service(true).with_rows("Parent",vec![row(a),row(b)]);
                emit(format!("group_{i}_{j}"),service.fetch_all(&SelectQuery::new("Parent").group_by("k").count("count")));
            }
        }
        let mut all: Vec<CompactRow> = values.iter().map(row).collect(); all.push(CompactRow::from_map(BTreeMap::new()));
        emit("group_all".into(),service(true).with_rows("Parent",all).fetch_all(&SelectQuery::new("Parent").group_by("k").count("count")));
        for (i,a) in values.iter().enumerate() {
            for (j,b) in values.iter().enumerate() {
                let service = service(true).with_rows("Parent",vec![row(a)]).with_rows("Child",vec![row(b)]);
                for single in [true,false] { emit(format!("relation_{i}_{j}_{}",if single {1} else {0}),relation(&service,single)); }
            }
        }
        for single in [true,false] {
            let mode = if single {1} else {0};
            let service = service(true).with_rows("Parent",vec![row(&Value::I64(1)),row(&Value::U64(1))])
                .with_rows("Child",vec![row(&Value::I64(1)),row(&Value::U64(1))]);
            emit(format!("relation_mixed_{mode}"),relation(&service,single));
            emit(format!("relation_empty_{mode}"),relation(&self::service(false),single));
            emit(format!("relation_missing_{mode}"),relation(&self::service(false).with_rows("Parent",vec![CompactRow::from_map(BTreeMap::new())]),single));
        }
    }
}
