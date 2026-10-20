fn schema_rows(rows:&[CompactRow])->J {
    json!(rows.iter().map(|row|json!({
        "equals_plain":row==&CompactRow::from_map(row.clone().into_map()),
        "validation":row.validate_dynamic_properties().err(),
    })).collect::<Vec<_>>())
}

fn provider_rows(mock:&Mock)->Vec<CompactRow> {
    if mock.schema_probe && mock.empty {return vec![];}
    let mut values=std::collections::BTreeMap::from([("id".into(),Value::U64(1))]);
    if mock.schema_probe {
        values.insert("_result".into(),match mock.result_kind.as_str(){
            "u64"=>Value::U64(7),"text"=>Value::Text("result".into()),
            "typed_null"=>Value::TypedNull(DataType::U64),"null"=>Value::Null,
            _=>panic!("fixture result kind"),
        });
    }
    vec![CompactRow::from_map(values)]
}

fn schema_query(query:SelectQuery,input:&J)->SelectQuery {
    if input["schema_probe"]!=true || input["schema_kind"]=="none" {return query;}
    query.with_dynamic_property_definitions(teaql_core::dynamic_properties::DynamicPropertyDefinitions::new(
        [("_result".into(),if input["schema_kind"]=="text" {DataType::Text}else{DataType::U64})]).unwrap())
}

struct RecordingCache {
    inner:InMemoryAggregationCache,
    events:Mutex<Vec<J>>,
}
impl crate::AggregationCacheBackend for RecordingCache {
    fn namespace(&self)->&str {self.inner.namespace()}
    fn get(&self,key:&str,age:u64)->Option<Vec<CompactRow>> {
        let rows=self.inner.get(key,age);
        self.events.lock().unwrap().push(json!({"op":"get","age":age,"hit":rows.is_some(),
            "schema":rows.as_ref().map(|rows|schema_rows(rows))}));
        rows
    }
    fn put(&self,key:String,rows:Vec<CompactRow>) {
        self.events.lock().unwrap().push(json!({"op":"put","schema":schema_rows(&rows)}));
        self.inner.put(key,rows)
    }
    fn invalidate_namespace(&self,namespace:&str) {self.inner.invalidate_namespace(namespace)}
}
