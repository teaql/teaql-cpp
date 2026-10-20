use cpp_dynamic_json_core::Q;
use teaql_core::request::QuerySelection;

fn main() {
    let child = Q::products_minimal();
    let selected: QuerySelection = child.clone().into();
    let core = selected.clone().into_query();
    let _ = Q::orders_minimal().facet_by_customer_as("_facet", child.clone());
    let _ = Q::orders_minimal().facet_by_customer_as("_facet", selected.clone());
    let _ = Q::orders_minimal().facet_by_customer_as_with_options("_facet", core.clone(), false);
    let _ = Q::orders_minimal().group_by_customer_with(child.clone());
    let _ = Q::orders_minimal().group_by_customer_with(selected.clone());
    let _ = Q::orders_minimal().group_by_customer_with_details_from(core.clone());
    let _ = Q::people_minimal().count_orders_with("_metric", child.clone());
    let _ = Q::people_minimal().count_orders_with("_metric", selected.clone());
    let _ = Q::people_minimal().count_orders_with("_metric", core.clone());
    let _ = Q::people_minimal().stats_from_orders(child);
    let _ = Q::people_minimal().stats_from_orders_as("_metric", selected);
    let _ = Q::people_minimal().group_by_orders_with_details(core);
    println!("Original facet/group/relation aggregate APIs accept unrelated requests, selections and core values");
}
