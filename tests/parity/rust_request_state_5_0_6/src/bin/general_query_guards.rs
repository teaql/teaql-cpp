use cpp_dynamic_json_core as domain;
#[path="../../../rust_field_layout/src/bin/original_like_privacy_5_0_6.rs"] mod driver;
include!("../../../general_query_guard_observer_5_0_6.rs");

fn observe(input: &Json) -> Json {
    let pool = std::sync::Arc::new(std::sync::Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));
    pool.lock().unwrap().execute_batch(include_str!("../../../../support/dynamic_json_schema.sql")).unwrap();
    pool.lock().unwrap().execute_batch(include_str!("../../../../support/dynamic_json_dataset.sql")).unwrap();
    let user_context = driver::block_on(domain::service_runtime_from_pool(pool)).unwrap().with_active_root("Platform",1);
    match input["entity"].as_str().unwrap() {
        "platform" => observe_request!(input, &user_context, Platform, PlatformRequest, platforms, platforms_minimal, platforms_with_children),
        "person" => observe_request!(input, &user_context, Person, PersonRequest, people, people_minimal, people_with_children),
        "order" => observe_request!(input, &user_context, Order, OrderRequest, orders, orders_minimal, orders_with_children),
        "product" => observe_request!(input, &user_context, Product, ProductRequest, products, products_minimal, products_with_children),
        _ => unreachable!(),
    }
}
