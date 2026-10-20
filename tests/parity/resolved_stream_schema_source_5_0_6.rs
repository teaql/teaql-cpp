#[cfg(test)]
mod cpp_resolved_stream_schema_observer {
    use super::*;
    __TEAQL_STREAM_DRIVER__

    fn schema_rows(rows:&[CompactRow])->J {
        json!(rows.iter().map(|row|json!({
            "equals_plain":row==&CompactRow::from_map(row.clone().into_map()),
            "validation":row.validate_dynamic_properties().err(),
        })).collect::<Vec<_>>())
    }

    fn provider_rows(index:usize,schema:bool,empty:bool,kind:&str)->Vec<CompactRow> {
        if schema && empty {return vec![];}
        (0..if schema {2}else{1}).map(|row| {
            let mut values=std::collections::BTreeMap::from([("id".into(),Value::U64(index as u64+1+row))]);
            if schema {
                values.insert("_result".into(),match kind {
                    "u64"=>Value::U64(7),"text"=>Value::Text("result".into()),
                    "typed_null"=>Value::TypedNull(DataType::U64),"null"=>Value::Null,
                    _=>panic!("Unknown stream fixture result kind"),
                });
            }
            CompactRow::from_map(values)
        }).collect()
    }

    #[test]
    fn replay_readonly_cursor_boundaries() {
        std::panic::set_hook(Box::new(|_| {}));
        let inputs:Vec<J>=serde_json::from_str(include_str!("__TEAQL_STREAM_INPUTS__")).unwrap();
        for input in inputs {println!("TEAQL_STREAM_SCHEMA_JSON {}",evaluate(&input));}
    }
}
