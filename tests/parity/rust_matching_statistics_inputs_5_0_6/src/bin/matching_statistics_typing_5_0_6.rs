use cpp_dynamic_json_core::Q;
use teaql_core::request::QuerySelection;
fn main() {
    let child = Q::products_minimal();
    let selected: QuerySelection = child.clone().into();
    let core = selected.clone().into_query();
    let _ = Q::orders_minimal().with_customer_matching(child.clone());
    let _ = Q::orders_minimal().without_customer_matching(selected.clone());
    let _ = Q::orders_minimal().roll_up_to_customer_with(core.clone());
    let _ = Q::people_minimal().with_orders_matching(child.clone());
    let _ = Q::people_minimal().without_orders_matching(core.clone());
    let _ = Q::people_minimal().sum_amount_of_orders_as("_sum", child.clone());
    let _ = Q::people_minimal().avg_amount_of_orders_as("_avg", selected.clone());
    let _ = Q::people_minimal().min_amount_of_orders_as("_min", core.clone());
    let _ = Q::people_minimal().max_amount_of_orders_as("_max", child.clone());
    let _ = Q::people_minimal().standard_deviation_amount_of_orders_as("_std", selected.clone());
    let _ = Q::people_minimal().square_root_of_population_standard_deviation_amount_of_orders_as("_pop", core.clone());
    let _ = Q::people_minimal().sample_variance_amount_of_orders_as("_var", selected.clone());
    let _ = Q::people_minimal().sample_population_variance_amount_of_orders_as("_pop_var", core);
    println!("Original matching, roll-up and all field-statistic families accept unrelated requests and both explicit conversions");
}
