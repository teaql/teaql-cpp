#include <teaql/runtime.hpp>
#include <teaql/data_service/executor.hpp>
#include <teaql/data_service/sql_log_debug.hpp>
#include "fixture_query_keys.hpp"
#include <iostream>

struct NoIo final : QueryExecutor<String> {
    using allocator_type = Allocator;

    NoIo(std::allocator_arg_t, Allocator) {}

    DataServiceCapabilities capabilities() const override {
        return {};
    }

    DataServiceOperationResult<QueryResult, String>& query(Memory, QueryRequest) override {
        throw std::runtime_error("intent discovery must not execute a query");
    }
};

static Json observe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto result = detail::model_json(r);
    if (text(field(input, "kind")) == "field") {
        auto descriptor = entity(r, field(input, "entity"));
        auto policy = SqlIntentRedactions::field_policy(descriptor, text(field(input, "field")));
        detail::model_put(result, "policy", debug_string(r, policy));
        return result;
    }
    auto build = [&]() {
        if (text(field(input, "kind")) == "query") {
            return key_query(r, field(input, "query"));
        }
        auto original = text(field(input, "original"));
        auto mode = text(field(input, "mode"));
        auto expr = Expr::begin_with(r, "name", original);
        String pattern(r);
        if (mode == "suffix" || mode == "contain") {
            pattern += '%';
        }
        pattern += original;
        if (mode != "suffix") {
            pattern += '%';
        }
        if (mode == "changed_pattern" || mode == "not_like_changed") {
            pattern = "CURRENT-PATTERN%";
        }
        expr.args[1].literal = Value(r, pattern);
        if (mode == "changed_operator") {
            expr.op = BinaryOp::Eq;
        } else if (mode == "not_like" || mode == "not_like_changed") {
            expr.op = BinaryOp::NotLike;
        }
        SelectQuery query(std::allocator_arg, Allocator(r), "Root");
        query.filter(std::move(expr));
        return query;
    };
    auto query = build();
    auto& provider = user_context.make_resource<NoIo>();
    auto output = provider.query_log_intent(r, query);
    Vector<String> safe(r), debug(r);
    output.extend_secrets(false, safe);
    output.extend_secrets(true, debug);
    Json::Array safe_values(r), debug_values(r);
    for (const auto& value : safe) {
        safe_values.emplace_back(value);
    }
    for (const auto& value : debug) {
        debug_values.emplace_back(value);
    }
    auto cloned = std::make_obj_using_allocator<SqlIntentRedactions>(Allocator(r), output);
    cloned.clear();
    detail::model_put(result, "safe", Json(r, safe_values));
    detail::model_put(result, "debug", Json(r, debug_values));
    detail::model_put(result, "description", debug_string(r, output));
    detail::model_put(result, "pretty", debug_string(r, output, DebugOptions{.pretty = true}));
    detail::model_put(result, "empty", output.is_empty());
    detail::model_put(result, "cleared_empty", cloned.is_empty());
    return result;
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    for (std::string line; std::getline(std::cin, line);) {
        GlobalContext global;
        auto user_context = global.make_user_context();
        auto input = Json::parse(user_context->memory_resource(), line);
        if (!input) {
            return 1;
        }
        auto result = observe(*user_context, *input);
        std::cout << result.dump(user_context->memory_resource()) << '\n';
    }
}
