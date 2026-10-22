// Appended observer; original runtime bodies remain byte-identical.
#[cfg(test)]
mod cpp_memory_sort_observer {
    use super::*;
    #[test]
    fn sort() {
        for size in [0usize, 1, 2, 17, 257] {
            for pattern in 0..3 {
                let metadata = InMemoryMetadataStore::new().with_entity(teaql_core::EntityDescriptor::new("Item"));
                let input = (0..size).map(|i| {
                    let key = match pattern {0 => i, 1 => size-i, _ => (i*37+11)%13};
                    BTreeMap::from([("id".into(),Value::U64(i as u64)),("key".into(),Value::I64(key as i64)),("secondary".into(),Value::I64((i%3) as i64)),("name".into(),Value::from(format!("item_{key:04}")))])
                }).collect();
                let service = MemoryDataService::new(metadata).with_rows("Item",input);
                for mode in 0..4 {
                    let query = match mode {
                        0 => SelectQuery::new("Item").order_asc("key"),
                        1 => SelectQuery::new("Item").order_desc("key"),
                        2 => SelectQuery::new("Item").order_asc("key").order_desc("secondary"),
                        _ => SelectQuery::new("Item").order_expr_asc(Expr::gbk(Expr::column("name"))),
                    };
                    let result=service.fetch_all(&query).unwrap();
                    let rows:Vec<BTreeMap<String,String>>=result.into_iter().map(|r|r.into_map().into_iter().map(|(k,v)|(k,format!("{v:?}"))).collect()).collect();
                    println!("MEMORY_SORT_OBSERVATION {}",serde_json::json!({"case":format!("sort_{size}_{pattern}_{mode}"),"rows":rows}));
                }
            }
        }
    }
}
