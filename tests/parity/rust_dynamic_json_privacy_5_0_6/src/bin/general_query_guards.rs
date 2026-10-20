use cpp_inheritance_graph_core as domain;
#[path="../../../rust_field_layout/src/bin/original_like_privacy_5_0_6.rs"] mod driver;
include!("../../../general_query_guard_observer_5_0_6.rs");

fn observe(input: &Json) -> Json {
    let pool = std::sync::Arc::new(std::sync::Mutex::new(rusqlite::Connection::open_in_memory().unwrap()));
    pool.lock().unwrap().execute_batch(include_str!("../../../../support/dynamic_json_inherited_rows.sql")).unwrap();
    let user_context = driver::block_on(domain::service_runtime_from_pool(pool)).unwrap().with_active_root("Platform",1);
    match input["entity"].as_str().unwrap() {
        "platform" => observe_request!(input, &user_context, Platform, PlatformRequest, platforms, platforms_minimal, platforms_with_children),
        "specialist" => observe_request!(input, &user_context, Specialist, SpecialistRequest, specialists, specialists_minimal, specialists_with_children),
        "school" => observe_request!(input, &user_context, School, SchoolRequest, schools, schools_minimal, schools_with_children),
        "academy" => observe_request!(input, &user_context, Academy, AcademyRequest, academies, academies_minimal, academies_with_children),
        "research_academy" => observe_request!(input, &user_context, ResearchAcademy, ResearchAcademyRequest, research_academies, research_academies_minimal, research_academies_with_children),
        "enrollment" => observe_request!(input, &user_context, Enrollment, EnrollmentRequest, enrollments, enrollments_minimal, enrollments_with_children),
        _ => unreachable!(),
    }
}
