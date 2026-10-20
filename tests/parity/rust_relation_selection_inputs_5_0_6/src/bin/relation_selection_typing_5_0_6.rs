use cpp_dynamic_json_core::Q;

fn main() {
    let child = Q::orders_minimal().select_code();
    let request = Q::people_minimal().select_platform_with(child.clone());
    let selection: teaql_core::request::QuerySelection = child.clone().into();
    let selected = Q::people_minimal().select_platform_with(selection);
    let selection: teaql_core::request::QuerySelection = child.into();
    let core = Q::people_minimal().select_platform_with(selection.into_query());
    let request_core: teaql_core::SelectQuery = request.into();
    let selected_core: teaql_core::SelectQuery = selected.into();
    let core_core: teaql_core::SelectQuery = core.into();
    assert_eq!(request_core, selected_core);
    assert_eq!(request_core, core_core);
    let _reverse = Q::platforms_minimal().select_person_list_with(Q::orders_minimal());
    println!("Original forward/reverse selectors accept other entity requests and both explicit value conversions");
}
