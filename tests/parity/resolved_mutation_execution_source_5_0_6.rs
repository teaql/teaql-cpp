#[cfg(test)]
mod cpp_resolved_mutation_execution_observer {
    use super::*;
    use crate::data_service::RuntimeDataService;

    __TEAQL_MUTATION_ACCESS_BRIDGE__

    __TEAQL_MUTATION_RETAINED_DRIVER__

    #[test]
    fn replay_bound_mutations_and_events() {
        std::panic::set_hook(Box::new(|_| {}));
        let inputs:Vec<J>=serde_json::from_str(include_str!("__TEAQL_RESOLVED_EXECUTION_INPUTS__")).unwrap();
        for input in inputs {println!("TEAQL_RESOLVED_EXECUTION_JSON {}",evaluate(input));}
    }
}
