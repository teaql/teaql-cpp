#include "../support/dynamic_json_runtime_fixture.hpp"
#include "fixture_value.hpp"
#include <teaql/core/query_debug.hpp>
#include <iostream>
#include <sstream>
#include <ranges>

using namespace cpp_dynamic_json;

template <class Request, class Apply>
static Json run(UserContext& user_context, Request query, Apply apply, const Json& input) {
    auto r = user_context.memory_resource();
    auto operand = value(r, field(input, "value"));
    auto upper = value(r, field(input, "upper"));
    for (int repeat = 0; repeat < (std::get<bool>(field(input, "repeat").data) ? 2 : 1); ++repeat) {
        apply(query, operand, upper);
    }
    auto output = detail::model_json(r);
    detail::model_put(output, "query", debug_string(r, query.query()));
    if (std::get<bool>(field(input, "execute").data)) {
        teaql::test::initialize_dynamic_json_database(user_context);
        const auto& result = std::move(query)
                                 .comment("inspect scalar comparison")
                                 .purpose("qualify scalar comparison")
                                 .executeForRows(user_context)
                                 .wait();
        Json::Array rows(r);
        Json error(r);
        if (result) {
            for (const auto& row : result.value().data) {
                Json::Array columns(r);
                for (auto key : {"id", "version"}) {
                    auto* item = row.get(key);
                    columns.emplace_back(item ? Json(r, debug_string(r, *item)) : Json(r));
                }
                rows.emplace_back(columns);
            }
        } else {
            error = Json(r, result.error().to_string(r));
        }
        detail::model_put(output, "rows", Json(r, rows));
        detail::model_put(output, "error", error);
    }
    return output;
}

static Json observe(UserContext& user_context, const Json& input) {
    auto Q = user_context.queries<query_factory>();
    bool minimal = std::get<bool>(field(input, "minimal").data);
    auto name = text(field(input, "field"));
    auto operation = text(field(input, "operation"));
#define SCALAR(REQUEST, EQ, NE, GT, GTE, LT, LTE, BETWEEN)                                         \
    run(                                                                                           \
        user_context,                                                                              \
        REQUEST,                                                                                   \
        [operation](auto& request, const Value& operand, const Value& upper) {                     \
            if (operation == "eq") {                                                               \
                request.EQ(operand);                                                               \
            } else if (operation == "ne") {                                                        \
                request.NE(operand);                                                               \
            } else if (operation == "gt") {                                                        \
                request.GT(operand);                                                               \
            } else if (operation == "gte") {                                                       \
                request.GTE(operand);                                                              \
            } else if (operation == "lt") {                                                        \
                request.LT(operand);                                                               \
            } else if (operation == "lte") {                                                       \
                request.LTE(operand);                                                              \
            } else {                                                                               \
                request.BETWEEN(operand, upper);                                                   \
            }                                                                                      \
        },                                                                                         \
        input)
#define IDENTITY(REQUEST, EQ, NE)                                                                  \
    run(                                                                                           \
        user_context,                                                                              \
        REQUEST,                                                                                   \
        [operation](auto& request, const Value& operand, const Value&) {                           \
            if (operation == "eq") {                                                               \
                request.EQ(operand);                                                               \
            } else {                                                                               \
                request.NE(operand);                                                               \
            }                                                                                      \
        },                                                                                         \
        input)
    if (name == "name") {
        return SCALAR(minimal ? Q.peopleMinimal() : Q.people(),
                      whoseNameIs,
                      whoseNameIsNot,
                      whoseNameGreaterThan,
                      whoseNameGreaterThanOrEqualTo,
                      whoseNameLessThan,
                      whoseNameLessThanOrEqualTo,
                      whoseNameBetween);
    }
    if (name == "person_id") {
        return IDENTITY(minimal ? Q.peopleMinimal() : Q.people(), whoseIdIs, whoseIdIsNot);
    }
    if (name == "code") {
        return SCALAR(minimal ? Q.ordersMinimal() : Q.orders(),
                      withCodeIs,
                      withCodeIsNot,
                      withCodeGreaterThan,
                      withCodeGreaterThanOrEqualTo,
                      withCodeLessThan,
                      withCodeLessThanOrEqualTo,
                      withCodeBetween);
    }
    if (name == "order_id") {
        return IDENTITY(minimal ? Q.ordersMinimal() : Q.orders(), withIdIs, withIdIsNot);
    }
    if (name == "priority") {
        return SCALAR(minimal ? Q.ordersMinimal() : Q.orders(),
                      withPriorityIs,
                      withPriorityIsNot,
                      withPriorityGreaterThan,
                      withPriorityGreaterThanOrEqualTo,
                      withPriorityLessThan,
                      withPriorityLessThanOrEqualTo,
                      withPriorityBetween);
    }
    return SCALAR(minimal ? Q.ordersMinimal() : Q.orders(),
                  withAmountIs,
                  withAmountIsNot,
                  withAmountGreaterThan,
                  withAmountGreaterThanOrEqualTo,
                  withAmountLessThan,
                  withAmountLessThanOrEqualTo,
                  withAmountBetween);
#undef SCALAR
#undef IDENTITY
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            {
                auto input = Json::parse(user_context->memory_resource(), line);
                std::cout << observe(*user_context, *input).dump(user_context->memory_resource())
                          << '\n';
            }
            user_context.reset();
            if (global.live_user_bytes()) {
                throw std::logic_error("Comparison retained User memory");
            }
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
