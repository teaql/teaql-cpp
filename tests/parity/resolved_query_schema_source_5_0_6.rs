#[cfg(test)]
mod cpp_resolved_query_schema_observer {
    use super::*;
    __TEAQL_QUERY_CAPTURE_DRIVER__
    __TEAQL_QUERY_SCHEMA_HELPERS__
    #[test]
    fn replay_result_schema_and_cache_paths() {
        std::panic::set_hook(Box::new(|_| {}));
        let inputs:Vec<J>=serde_json::from_str(include_str!("__TEAQL_QUERY_SCHEMA_INPUTS__")).unwrap();
        for input in inputs {println!("TEAQL_QUERY_SCHEMA_JSON {}",evaluate(&input));}
    }
}
