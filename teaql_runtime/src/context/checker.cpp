#include <teaql/runtime/context.hpp>
#include <algorithm>

namespace teaql {
Arena& detail::checker_reference_origin(const UserContext& context, std::uint64_t id) {
    return context.reference_arena(id, std::source_location::current());
}

Arena& detail::checker_call_arena(const UserContext& context) {
    return detail::resource_arena(context.memory_resource());
}

bool UserContext::has_checker(std::string_view entity) const {
    check_public();
    detail::CheckerCallScope active(detail::resource_arena(memory_resource()));
    return checker_registry_ &&
           checker_registry_->get().checker(memory_resource(), entity).has_value();
}

RuntimeHookResult UserContext::check_and_fix_values(std::string_view entity,
                                                    EntityValues& values) const {
    return check_and_fix_values_at(entity, values, ObjectLocation::root(memory_resource()));
}

RuntimeHookResult UserContext::check_and_fix_values_at(std::string_view entity,
                                                       EntityValues& values,
                                                       const ObjectLocation& location) const {
    check_public();
    auto r = memory_resource();
    detail::CheckerCallScope active(detail::resource_arena(r));
    auto status = check_object_status(values);
    auto checker = checker_registry_ ? checker_registry_->get().checker(r, entity)
                                     : std::optional<CheckerBorrow>{};
    CheckResults results(r);
    if (checker) {
        checker->get().check_and_fix(*this, values, location, results);
    }
    collect_required_property_results(entity, values, status, location, results);
    if (results.empty()) {
        return RuntimeHookResult::ok(r, {});
    }
    translate_check_results(results);
    return RuntimeHookResult::err(r, RuntimeError::check(r, std::move(results)));
}

void UserContext::collect_required_property_results(std::string_view name,
                                                    const EntityValues& values,
                                                    CheckObjectStatus status,
                                                    const ObjectLocation& location,
                                                    CheckResults& results) const {
    const auto* descriptor = entity(name);
    if (!descriptor) {
        return;
    }
    auto r = results.get_allocator().resource();
    for (const auto& property : descriptor->properties) {
        if (property.nullable || property.is_version) {
            continue;
        }
        auto found = values.fields.find(property.name);
        bool missing = found == values.fields.end();
        bool null = !missing && std::holds_alternative<std::monostate>(found->second.data);
        auto path = detail::clone_in(r, location);
        path.member(property.name);
        bool reported = std::any_of(results.begin(), results.end(), [&](const CheckResult& result) {
            return result.rule == CheckRule::Required && result.location == path;
        });
        if (((status == CheckObjectStatus::Create && missing) || null) && !reported) {
            results.emplace_back(CheckResult::required(r, std::move(path)));
        }
    }
}
}
