#include <teaql/core/query_debug.hpp>
#include "../support/query_full_state_fixture.hpp"
#include "debug_modes.hpp"
#include "fixture_value.hpp"
#include <iostream>

static Json prepared(Memory owner, const SelectQuery& query) {
    Json::Object result(owner);
    try {
        result.emplace("ok", formatted_debug(owner, query.prepare_for_list()));
    } catch (const std::invalid_argument& error) {
        result.emplace("error", Json(owner, error.what()));
    }
    return Json(owner, std::move(result));
}

static Json observe(UserContext& user_context, const Json& input) {
    auto owner = user_context.memory_resource();
    auto base = query_full_state_fixture::make(
        user_context, unsigned(std::get<std::uint64_t>(field(input, "profile").data)));
    auto original = base.clone(owner);
    auto changed = base.clone(owner);
    query_full_state_fixture::mutate(
        user_context, changed, unsigned(std::get<std::uint64_t>(field(input, "field").data)));
    auto copy = changed.clone(owner);
    Json::Object output(owner);
    output.emplace("base", formatted_debug(owner, base));
    output.emplace("changed", formatted_debug(owner, changed));
    output.emplace("clone", formatted_debug(owner, copy));
    output.emplace("self_equal", Json(owner, changed == changed));
    output.emplace("equal_clone", Json(owner, changed == copy));
    output.emplace("changed_equal_base", Json(owner, changed == base));
    output.emplace("base_unchanged", Json(owner, base == original));
    output.emplace("schema_shared",
                   changed.dynamic_property_definitions
                       ? Json(owner,
                              changed.dynamic_property_definitions.get() ==
                                  copy.dynamic_property_definitions.get())
                       : Json(owner));
    output.emplace("selection_shared",
                   changed.dynamic_field_selection ? Json(owner,
                                                          changed.dynamic_field_selection.get() ==
                                                              copy.dynamic_field_selection.get())
                                                   : Json(owner));
    output.emplace("base_prepared", prepared(owner, base));
    output.emplace("changed_prepared", prepared(owner, changed));
    return Json(owner, std::move(output));
}

int main() {
    try {
        GlobalContext global;
        std::string line;
        while (std::getline(std::cin, line)) {
            auto user_context = global.make_user_context();
            auto input = Json::parse(user_context->memory_resource(), line);
            if (!input) {
                throw std::runtime_error("Invalid complete query fixture");
            }
            std::cout << observe(*user_context, *input).dump(user_context->memory_resource())
                      << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
