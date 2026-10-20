#include <teaql/core/query_debug.hpp>
#include <teaql/runtime/context.hpp>
#include "debug_modes.hpp"
#include "fixture_value.hpp"
#include <array>
#include <iostream>

static Json observe(Memory owner, const Json& input) {
    const auto column = text(field(input, "column"));
    const auto original = text(field(input, "text"));
    const std::array expressions{Expr::contain(owner, column, original),
                                 Expr::not_contain(owner, column, original),
                                 Expr::begin_with(owner, column, original),
                                 Expr::not_begin_with(owner, column, original),
                                 Expr::end_with(owner, column, original),
                                 Expr::not_end_with(owner, column, original)};
    Json::Array observations(owner);
    for (std::size_t index = 0; index < expressions.size(); ++index) {
        const auto& expression = expressions[index];
        if (expression.kind != Expr::Kind::Binary || expression.args.size() != 2 ||
            expression.args[1].kind != Expr::Kind::LikePattern) {
            throw std::runtime_error("typed helper lost its binary provenance leaf");
        }
        const auto& leaf = expression.args[1];
        const auto pattern = *leaf.literal.try_text();
        auto raw = index % 2 == 0 ? Expr::like(owner, column, pattern)
                                  : Expr::not_like(owner, column, pattern);
        auto changed_original = leaf.clone(owner);
        changed_original.name += "x";
        auto changed_pattern = leaf.clone(owner);
        String altered_pattern(pattern, owner);
        altered_pattern += "x";
        changed_pattern.literal = Value(owner, std::string_view(altered_pattern));
        Json::Object result(owner);
        result.emplace("formatted", formatted_debug(owner, expression));
        result.emplace("leaf", formatted_debug(owner, leaf));
        result.emplace("pattern", Json(owner, pattern));
        result.emplace("original", Json(owner, leaf.name));
        result.emplace("clone_equal", Json(owner, expression == expression.clone(owner)));
        result.emplace("raw_equal", Json(owner, expression == raw));
        result.emplace("raw", formatted_debug(owner, raw));
        result.emplace("changed_original_equal", Json(owner, leaf == changed_original));
        result.emplace("changed_pattern_equal", Json(owner, leaf == changed_pattern));
        observations.emplace_back(Json(owner, std::move(result)));
    }
    return Json(owner, std::move(observations));
}

int main() {
    auto previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        GlobalContext global;
        std::string line;
        while (std::getline(std::cin, line)) {
            auto user_context_owner = global.make_user_context();
            {
                auto owner = user_context_owner->memory_resource();
                auto input = Json::parse(owner, line);
                if (!input) {
                    throw std::runtime_error("Invalid expression observation input");
                }
                auto result = observe(owner, *input);
                std::cout << result.dump(owner) << '\n';
            }
            user_context_owner.reset();
            if (global.live_user_bytes() != 0) {
                throw std::runtime_error("UserContext retained expression bytes after deletion");
            }
        }
        std::pmr::set_default_resource(previous);
        return 0;
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
