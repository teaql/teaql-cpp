//! Execute the unchanged standalone registry factories and local schema provider.
use cpp_business_ids_core::{LocalSchemaProvider, TenantBehavior, TenantChecker};
use serde_json::{json, Value as Json};
use std::io::{self, BufRead};
use teaql_core::{SelectQuery, Value};
use teaql_data_service::SchemaProvider;
use teaql_runtime::{CheckerRegistry, EntityDataServiceBehaviorRegistry, EntityRegistry};

fn observe(input: &Json) -> Json {
    let name = input["name"].as_str().unwrap();
    let repository = cpp_business_ids_core::repository_registry();
    let behaviors = cpp_business_ids_core::behavior_registry();
    let checkers = cpp_business_ids_core::checker_registry();
    let provider = LocalSchemaProvider;
    let first = provider.get_entity(name);
    let second = provider.clone().get_entity(name);
    let user_context = cpp_business_ids_core::module().into_context();
    let behavior = behaviors.behavior(name);
    let mut query = SelectQuery::new(name);
    let behavior_success = behavior.as_ref().map(|value| value.before_select(&user_context, &mut query).is_ok());
    let checker = checkers.checker(name);
    let mut values = teaql_runtime::EntityValues::new();
    values.insert("id".to_owned(), Value::U64(1));
    values.insert("name".to_owned(), Value::Text(input["value"].as_str().unwrap().to_owned()));
    values.insert("version".to_owned(), Value::I64(1));
    let mut findings = Vec::new();
    if name == "Tenant" {
        checker.as_ref().unwrap().check_and_fix(
            &user_context, &mut values,
            &teaql_runtime::ObjectLocation::root(), &mut findings);
    }
    let extended = repository.clone().with_entity("ExternalEntity");
    // Replacing defaults in a clone must not remove another model's registration.
    let replaced_behaviors = behaviors.clone().with_behavior("Tenant", TenantBehavior::default());
    let replaced_checkers = checkers.clone().with_checker(
        teaql_runtime::TypedEntityChecker::<cpp_business_ids_core::Tenant, _>::new(TenantChecker::default()));
    json!({
        "repository": repository.contains(name),
        "repository_debug": format!("{repository:?}"),
        "original_has_extension": repository.contains("ExternalEntity"),
        "clone_has_extension": extended.contains("ExternalEntity"),
        "behavior": behavior.is_some(),
        "behavior_success": behavior_success,
        "checker": checker.as_ref().map(|value| value.entity().to_owned()),
        "findings": findings.iter().map(|value| value.to_wire(teaql_runtime::JsonFieldNamingProfile::SnakeCase)).collect::<Vec<_>>(),
        "replacement_keeps_receipt_behavior": replaced_behaviors.behavior("Receipt").is_some(),
        "replacement_keeps_receipt_checker": replaced_checkers.checker("Receipt").is_some(),
        "schema": first.as_ref().map(|value| format!("{value:?}")),
        "schema_clone_equal": match (&first, &second) { (Some(a),Some(b))=>Some(a == b), _=>None },
        "schema_fresh": match (&first, &second) { (Some(a),Some(b))=>Some(!std::sync::Arc::ptr_eq(a,b)), _=>None },
    })
}

fn main() {
    for line in io::stdin().lock().lines() {
        let input: Json = serde_json::from_str(&line.unwrap()).unwrap();
        println!("{}", observe(&input));
    }
}
