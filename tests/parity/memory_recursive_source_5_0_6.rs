// Appended observer; original runtime bodies remain byte-identical.
#[cfg(test)]
mod cpp_memory_recursive_observer {
    use super::*;
    fn bad()->Expr {Expr::soundex(Expr::value(1i64))}
    fn count()->Expr {Expr::function(ExprFunction::Count,[])}
    fn leaf(i:usize)->Expr {match i {
        0=>Expr::value(true),1=>Expr::value(false),2=>Expr::value(Value::Null),3=>Expr::value(1i64),
        4=>Expr::column("missing"),5=>bad(),6=>Expr::gbk(Expr::value(true)),
        _=>Expr::in_subquery("id",teaql_core::EntityDescriptor::new("Child"),SelectQuery::new("Child"),"id")
    }}
    fn expression(i:usize)->Expr {
        if i<256 {
            let a=leaf(i/32);let b=leaf((i/4)%8);let mode=i%4;
            let expr=if mode%2==0 {Expr::and([a,b])}else{Expr::or([a,b])};
            return if mode>=2 {Expr::negate(expr)}else{expr};
        }
        match i-256 {
            0=>Expr::and([]),1=>Expr::or([]),2=>Expr::negate(leaf(4)),3=>Expr::negate(Expr::negate(leaf(0))),
            4=>Expr::Binary{left:Box::new(bad()),op:BinaryOp::Eq,right:Box::new(count())},
            5=>Expr::Binary{left:Box::new(count()),op:BinaryOp::Eq,right:Box::new(bad())},
            6=>Expr::Between{expr:Box::new(bad()),lower:Box::new(count()),upper:Box::new(Expr::function(ExprFunction::Gbk,[]))},
            7=>Expr::Between{expr:Box::new(leaf(2)),lower:Box::new(bad()),upper:Box::new(count())},
            8=>Expr::Between{expr:Box::new(leaf(2)),lower:Box::new(leaf(2)),upper:Box::new(bad())},
            9=>Expr::IsNull(Box::new(Expr::and([leaf(0),leaf(1)]))),
            10=>Expr::IsNotNull(Box::new(leaf(2))),
            11=>Expr::gbk(Expr::and([leaf(0),leaf(1)])),
            12=>Expr::soundex(Expr::or([leaf(0),leaf(1)])),
            13=>Expr::function(ExprFunction::Gbk,[leaf(0),bad()]),
            14=>Expr::soundex(leaf(2)),
            _=>Expr::not_in_subquery("id",teaql_core::EntityDescriptor::new("Child"),SelectQuery::new("Child"),"id")
        }
    }
    #[test]
    fn recursive() {
        let row=CompactRow::from_map(BTreeMap::new());
        for i in 0..272 {
            let out=match eval_filter(&expression(i),&row) {
                Ok(v)=>serde_json::json!({"case":format!("recursive_{i}"),"ok":v}),
                Err(e)=>serde_json::json!({"case":format!("recursive_{i}"),"error":e.to_string()})
            };
            println!("MEMORY_RECURSIVE_OBSERVATION {}",out);
        }
    }
}
