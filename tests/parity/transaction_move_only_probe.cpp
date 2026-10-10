#include "../transaction_move_only_fixture.hpp"
#include <teaql/core/detail/model_json.hpp>
#include "fixture_value.hpp"
#include <iostream>
using namespace transaction_move_fixture;

static Json evaluate(GlobalContext& global, const Json& input) {
    auto result_owner = global.memory_resource();
    auto output = detail::model_json(result_owner);
    auto outcome = detail::model_json(result_owner);
    unsigned destroyed = 0, invoked = 0;
    saver_fixture::Controls controls;
    controls.fail_begin = std::get<bool>(field(input, "begin_fail").data);
    controls.fail_commit = std::get<bool>(field(input, "commit_fail").data);
    controls.fail_rollback = std::get<bool>(field(input, "rollback_fail").data);
    {
        auto user = global.make_user_context();
        user->disable_sql_log();
        auto& state = user->make<mutation_fixture::State>();
        auto executor = user->make_resource_ref<Ambient>(state, controls);
        user->insert_resource(executor);
        Callback callback(std::allocator_arg,
                          Allocator(user->memory_resource()),
                          text(field(input, "payload")),
                          destroyed,
                          std::get<bool>(field(input, "body_fail").data),
                          invoked);
        auto& work = user->execute_in_transaction<Ambient, MoveOnlyOutput>(std::move(callback));
        auto& result = work.wait();
        if (result) {
            detail::model_put(outcome, "value", result.value().payload);
        } else {
            detail::model_put(outcome, "error", result.error().to_string(user->memory_resource()));
        }
        Json::Array notes(result_owner);
        for (const auto& note : state.notes) {
            notes.emplace_back(note);
        }
        detail::model_put(output, "notes", notes);
    }
    detail::model_put(output, "outcome", outcome);
    detail::model_put(output, "destroyed", destroyed);
    detail::model_put(output, "abandoned", controls.abandoned);
    return output;
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto input = Json::parse(global.memory_resource(), line);
        if (!input) {
            throw std::invalid_argument("Invalid move-only transaction fixture");
        }
        std::cout << evaluate(global, *input).dump(global.memory_resource()) << '\n';
    }
}
