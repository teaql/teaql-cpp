// Appended observer; every original memory.rs body remains byte-identical.
#[cfg(test)]
mod cpp_memory_pipeline_observer {
    use super::*;
    #[test]
    fn row_pipeline() {
        let metadata = InMemoryMetadataStore::new().with_entity(teaql_core::EntityDescriptor::new("Item"));
        let rows = vec![
            BTreeMap::from([("id".into(), Value::U64(1)), ("name".into(), Value::from("zed")), ("n".into(), Value::I64(3))]),
            BTreeMap::from([("id".into(), Value::U64(2)), ("name".into(), Value::from("alpha")), ("n".into(), Value::I64(1))]),
            BTreeMap::from([("id".into(), Value::U64(3)), ("name".into(), Value::from("beta"))]),
        ];
        let service = MemoryDataService::new(metadata).with_rows("Item", rows);
        let q = || SelectQuery::new("Item");
        let count_fn = || Expr::function(ExprFunction::Count, []);
        let cases = vec![
            ("project_soundex", q().project_expr("code", Expr::soundex(Expr::column("name")))),
            ("project_gbk", q().project_expr("copy", Expr::gbk(Expr::column("name")))),
            ("project_missing", q().project("absent").project_expr("copy", Expr::column("absent"))),
            ("project_binary_error", q().project_expr("flag", Expr::eq("n", 3i64))),
            ("project_function_error", q().project_expr("count", count_fn())),
            ("project_source_snapshot", q().project_expr("name", Expr::value("changed")).project_expr("copy", Expr::column("name"))),
            ("sort_gbk_asc", q().order_gbk_asc("name").project("id")),
            ("sort_gbk_desc", q().order_gbk_desc("name").project("id")),
            ("sort_missing_field", q().order_asc("n").project("id")),
            ("sort_error_equal", q().order_expr_asc(count_fn()).project("id")),
            ("slice_before_error", q().offset(3).project_expr("bad", count_fn())),
            ("slice_huge_offset", q().offset(u64::MAX).project("id")),
            ("slice_order", q().order_asc("name").offset(1).limit(1).project("id")),
            ("having_signed_count", q().count("total").having(Expr::eq("total", 3i64))),
            ("having_non_boolean", q().count("total").having(Expr::column("total"))),
            ("aggregate_gbk", q().count("total").project_expr("copy", Expr::gbk(Expr::column("total")))),
            ("aggregate_sequential", q().count("total").project_expr("next", Expr::column("total")).project_expr("copy", Expr::column("next"))),
            ("aggregate_ignores_slice", q().count("total").offset(99).limit(0)),
        ];
        for (name, query) in cases {
            let output = match service.fetch_all(&query) {
                Ok(rows) => {
                    let values: Vec<BTreeMap<String, String>> = rows.into_iter().map(|row| row.into_map().into_iter().map(|(k,v)| (k,format!("{v:?}"))).collect()).collect();
                    serde_json::json!({"case":name,"rows":values})
                }
                Err(error) => serde_json::json!({"case":name,"error":error.to_string()}),
            };
            println!("MEMORY_PIPELINE_OBSERVATION {}", output);
        }
    }
}
