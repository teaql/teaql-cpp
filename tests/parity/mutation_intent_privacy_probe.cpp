#include "../support/runtime_mutation_fixture.hpp"
#include <teaql/core/detail/model_json.hpp>
#include "fixture_value.hpp"
#include <iostream>

using namespace teaql;

static Json evaluate(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    user_context.disable_sql_log();
    auto& state = user_context.make<mutation_fixture::State>();
    auto& executor = user_context.make_resource<mutation_fixture::Executor>(state);
    ResourceBorrow<mutation_fixture::Executor> borrow(
        std::allocator_arg, Allocator(r), executor, detail::resource_arena(r));
    auto& context_service =
        user_context.make<detail::ContextDataService<mutation_fixture::Executor>>(user_context,
                                                                                  borrow);
    auto first = MutationIntent::create(r, "first reason");
    auto next = MutationIntent::create(
        r, text(field(input, "replacement")) == "different" ? "next reason" : "first reason");
    SqlIntentRedactions redactions(r);
    auto mode = text(field(input, "privacy"));
    if (mode == "nonempty") {
        redactions.capture_target_id(Value(r, std::uint64_t(7)));
    }
    const auto* privacy = mode == "none" ? nullptr : &redactions;
    auto& command = user_context.make<InsertCommand>("Payment");
    command.values.fields.emplace(String("id", r), Value(r, std::uint64_t(7)));
    auto execute = [&](auto& service) {
        service.with_mutation_intent(first.value(), privacy);
        if (text(field(input, "replacement")) != "none") {
            service.with_mutation_intent(next.value());
        }
        const auto& result = service.insert(command).wait();
        if (!result || result.value() != 1 || state.requests.size() != 1) {
            throw std::logic_error("fixture insert did not complete");
        }
    };
    if (text(field(input, "layer")) == "base") {
        auto base = context_service.data_service();
        execute(base);
    } else {
        execute(context_service);
    }
    const auto& request = state.requests.front();
    auto output = detail::model_json(r);
    detail::model_put(output, "comment", request.comment());
    detail::model_put(output,
                      "privacy",
                      request.diagnostic_redactions()
                          ? Json(r, request.diagnostic_redactions()->is_empty())
                          : Json(r));
    return output;
}

int main() {
    auto previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    std::string line;
    while (std::getline(std::cin, line)) {
        GlobalContext global;
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        auto input = Json::parse(r, line);
        std::cout << evaluate(*user_context, *input).dump(r) << '\n';
    }
    std::pmr::set_default_resource(previous);
}
