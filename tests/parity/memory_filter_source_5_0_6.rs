// Appended observer only; every original memory.rs body remains byte-identical.
#[cfg(test)]
mod cpp_memory_filter_observer {
    use super::*;
    #[test]
    fn scalar_boundaries() {
        let row=CompactRow::from_map(BTreeMap::from([("name".into(),Value::from("teaql"))]));
        let cases=vec![
            ("literal_zero",Expr::value(0i64)),
            ("literal_text",Expr::value("teaql")),
            ("literal_null",Expr::value(Value::Null)),
            ("literal_true",Expr::value(true)),
            ("soundex_number",Expr::soundex(Expr::value(1i64))),
            ("soundex_zero_args",Expr::function(ExprFunction::Soundex,[])),
            ("soundex_two_args",Expr::function(ExprFunction::Soundex,[Expr::value("a"),Expr::value("b")])),
            ("soundex_text",Expr::soundex(Expr::column("name"))),
            ("gbk_zero_args",Expr::function(ExprFunction::Gbk,[])),
            ("gbk_boolean",Expr::gbk(Expr::value(true))),
            ("count_scalar",Expr::function(ExprFunction::Count,[])),
            ("in_non_list",Expr::Binary{left:Box::new(Expr::value(1i64)),op:BinaryOp::In,right:Box::new(Expr::value(1i64))}),
            ("not_in_non_list",Expr::Binary{left:Box::new(Expr::value(1i64)),op:BinaryOp::NotIn,right:Box::new(Expr::value(1i64))}),
            ("null_between",Expr::between("missing",1i64,2i64)),
            ("mixed_integer_equal",Expr::Binary{left:Box::new(Expr::value(1i64)),op:BinaryOp::Eq,right:Box::new(Expr::value(1u64))}),
            ("null_not_like",Expr::Binary{left:Box::new(Expr::value(Value::Null)),op:BinaryOp::NotLike,right:Box::new(Expr::value("x"))}),
        ];
        for (name,expr) in cases {
            let output=match eval_filter(&expr,&row) {
                Ok(value)=>serde_json::json!({"case":name,"ok":value}),
                Err(error)=>serde_json::json!({"case":name,"error":error.to_string()}),
            };
            println!("MEMORY_FILTER_OBSERVATION {}",output);
        }
    }
}
