// Source counterpart uses a noncopyable payload, as the original oracle does.
#include <teaql/data_service/sql_log.hpp>
#include <teaql/core/detail/model_json.hpp>
#include <teaql/runtime/context.hpp>
#include "fixture_value.hpp"
#include <iostream>

struct ProjectionStatePayload {
    using allocator_type = Allocator;
    String text;

    ProjectionStatePayload(std::allocator_arg_t, Allocator owner, std::string_view value)
        : text(value, owner.resource()) {}

    ProjectionStatePayload(const ProjectionStatePayload&) = delete;
    ProjectionStatePayload(ProjectionStatePayload&&) = default;
};

static Json evaluate(UserContext& user, const Json& input) {
    auto owner = user.memory_resource();
    auto text_value = text(field(input, "text"));
    auto first = SqlProjectionState::make<ProjectionStatePayload>(owner, text_value);
    auto cloned = first;
    auto independent = SqlProjectionState::make<ProjectionStatePayload>(owner, text_value);
    SqlProjectionState empty(owner);
    auto before = detail::model_json(owner);
    detail::model_put(before, "text", cloned.get<ProjectionStatePayload>()->text);
    detail::model_put(before, "wrong_type_absent", cloned.get<String>() == nullptr);
    detail::model_put(before, "clone_equal", cloned == first);
    detail::model_put(before, "independent_equal", independent == first);
    detail::model_put(before, "empty_equal", empty == SqlProjectionState(owner));
    detail::model_put(before, "empty_some_equal", empty == first);
    detail::model_put(before, "debug", SqlProjectionState::debug());
    first = SqlProjectionState(owner);
    auto output = detail::model_json(owner);
    detail::model_put(output, "before_drop", before);
    detail::model_put(output, "after_drop", cloned.get<ProjectionStatePayload>()->text);
    return output;
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context();
        auto input = Json::parse(user->memory_resource(), line);
        if (!input) {
            throw std::invalid_argument("Invalid projection-state input");
        }
        std::cout << evaluate(*user, *input).dump(user->memory_resource()) << '\n';
    }
}
