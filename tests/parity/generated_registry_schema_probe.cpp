#include "../generated_business_ids/include/cpp_business_ids.hpp"
#include "fixture_value.hpp"
#include <teaql/runtime/registry_debug.hpp>
#include <iostream>

using namespace teaql;

static Json observe(UserContext& user_context, const Json& input) {
    auto resource = user_context.memory_resource();
    auto name = text(field(input, "name"));
    auto& repository = cpp_business_ids::repositoryRegistry(user_context);
    auto& behaviors = cpp_business_ids::behaviorRegistry(user_context);
    auto& checkers = cpp_business_ids::checkerRegistry(user_context);
    auto& provider = user_context.make<cpp_business_ids::LocalSchemaProvider>();
    auto& copied_provider = user_context.make<cpp_business_ids::LocalSchemaProvider>(provider);
    auto* first = provider.get_entity(name);
    auto* second = copied_provider.get_entity(name);
    auto& module = cpp_business_ids::module(user_context);
    module.apply_to(user_context);
    user_context.set_metadata(user_context, module.metadata);
    auto behavior = behaviors.behavior(resource, name);
    auto checker = checkers.checker(resource, name);
    SelectQuery query(std::allocator_arg, Allocator(resource), name);
    Json::Object output(resource);
    output.emplace("repository", Json(resource, repository.contains(name)));
    output.emplace("repository_debug", Json(resource, debug_string(resource, repository)));
    auto& extended = user_context.make<InMemoryEntityRegistry>(repository);
    extended.register_entity("ExternalEntity");
    output.emplace("original_has_extension", Json(resource, repository.contains("ExternalEntity")));
    output.emplace("clone_has_extension", Json(resource, extended.contains("ExternalEntity")));
    output.emplace("behavior", Json(resource, bool(behavior)));
    output.emplace(
        "behavior_success",
        behavior
            ? Json(resource, bool(behavior->get().before_select(resource, user_context, query)))
            : Json(resource));
    output.emplace("checker", checker ? Json(resource, checker->get().entity()) : Json(resource));
    EntityValues values(std::allocator_arg, Allocator(resource));
    values.fields.emplace("id", Value(resource, std::uint64_t{1}));
    values.fields.emplace("name", Value(resource, text(field(input, "value"))));
    values.fields.emplace("version", Value(resource, std::int64_t{1}));
    CheckResults findings(resource);
    if (name == "Tenant") {
        checker->get().check_and_fix(
            user_context, values, ObjectLocation::root(resource), findings);
    }
    Json::Array observed_findings(resource);
    for (const auto& finding : findings) {
        observed_findings.push_back(
            finding.to_wire(resource, JsonFieldNamingProfile::SnakeCase).to_json(resource));
    }
    output.emplace("findings", Json(resource, std::move(observed_findings)));
    auto& replaced_behaviors =
        user_context.make<InMemoryEntityDataServiceBehaviorRegistry>(behaviors);
    auto& replacement_behavior = user_context.make<cpp_business_ids::TenantBehavior>();
    replaced_behaviors.register_behavior("Tenant", user_context, replacement_behavior);
    auto& replaced_checkers = user_context.make<InMemoryCheckerRegistry>(checkers);
    auto& replacement_checker = user_context.make<cpp_business_ids::TenantChecker>();
    auto& replacement_adapter =
        user_context
            .make<TypedEntityChecker<cpp_business_ids::Tenant, cpp_business_ids::TenantChecker>>(
                user_context, replacement_checker);
    replaced_checkers.register_checker(user_context, replacement_adapter);
    output.emplace("replacement_keeps_receipt_behavior",
                   Json(resource, bool(replaced_behaviors.behavior(resource, "Receipt"))));
    output.emplace("replacement_keeps_receipt_checker",
                   Json(resource, bool(replaced_checkers.checker(resource, "Receipt"))));
    output.emplace("schema",
                   first ? Json(resource, debug_string(resource, *first)) : Json(resource));
    output.emplace("schema_clone_equal",
                   first && second ? Json(resource, *first == *second) : Json(resource));
    output.emplace("schema_fresh",
                   first && second ? Json(resource, first != second) : Json(resource));
    return Json(resource, std::move(output));
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        std::string line;
        while (std::getline(std::cin, line)) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            auto resource = user_context->memory_resource();
            {
                auto input = Json::parse(resource, line);
                if (!input) {
                    throw std::runtime_error("Invalid registry observation input");
                }
                auto output = observe(*user_context, *input);
                std::cout << output.dump(resource) << '\n';
            }
            user_context.reset();
            if (global.live_user_bytes() != 0) {
                throw std::runtime_error("Standalone registry observation retained User memory");
            }
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
    return 0;
}
