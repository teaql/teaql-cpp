#include <teaql/core/query_debug.hpp>
#include <teaql/runtime/context.hpp>
#include <teaql/runtime/purposed_query_debug.hpp>
#include "debug_modes.hpp"
#include "fixture_value.hpp"
#include <iostream>

static Json observe(UserContext& user_context, const Json& input) {
    auto owner = user_context.memory_resource();
    SelectQuery query(std::allocator_arg, Allocator(owner), text(field(input, "entity")));
    query.project("id");
    if (auto* comment = std::get_if<String>(&field(input, "comment").data)) {
        query.comment(*comment);
    }
    Json::Array stages(owner);
    for (const auto& value : std::get<Json::Array>(field(input, "purposes").data)) {
        auto purpose = text(value);
        auto before = formatted_debug(owner, query);
        auto assigned = query.clone(owner);
        assigned.purpose_text.emplace(purpose, owner);
        query.purpose(purpose);
        Json::Object runtime(owner);
        try {
            PurposedSelectQuery wrapped(std::allocator_arg, Allocator(owner), query, purpose);
            PurposedSelectQuery copy(std::allocator_arg, Allocator(owner), wrapped);
            auto core = std::move(copy).into_query();
            PurposedSelectQuery twice(
                std::allocator_arg, Allocator(owner), core, "second boundary");
            runtime.emplace("debug", formatted_debug(owner, wrapped));
            PurposedSelectQuery cloned(std::allocator_arg, Allocator(owner), wrapped);
            runtime.emplace("clone", formatted_debug(owner, cloned));
            runtime.emplace("twice", formatted_debug(owner, twice));
            runtime.emplace("trace_count",
                            Json(owner, std::uint64_t(wrapped.as_query().trace_chain.size())));
        } catch (const std::invalid_argument& error) {
            runtime.emplace("error", Json(owner, error.what()));
        }
        Json::Object stage(owner);
        stage.emplace("before", std::move(before));
        stage.emplace("core", formatted_debug(owner, query));
        stage.emplace("same_as_assignment", Json(owner, query == assigned));
        stage.emplace("trace_count", Json(owner, std::uint64_t(query.trace_chain.size())));
        stage.emplace("prepared", formatted_debug(owner, query.prepare_for_list()));
        stage.emplace("runtime", Json(owner, std::move(runtime)));
        stages.emplace_back(std::move(stage));
    }
    return Json(owner, std::move(stages));
}

int main() {
    try {
        GlobalContext global;
        std::string line;
        while (std::getline(std::cin, line)) {
            auto user_context = global.make_user_context();
            auto input = Json::parse(user_context->memory_resource(), line);
            if (!input) {
                throw std::runtime_error("Invalid query intent fixture");
            }
            std::cout << observe(*user_context, *input).dump(user_context->memory_resource())
                      << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
