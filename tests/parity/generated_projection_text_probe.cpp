#include "../support/dynamic_json_runtime_fixture.hpp"
#include "fixture_value.hpp"
#include <teaql/core/query_debug.hpp>
#include <iostream>
#include <sstream>
#include <ranges>

using namespace cpp_dynamic_json;

static AggregateFunction function(std::string_view name) {
    const std::array names{"Count",
                           "Sum",
                           "Avg",
                           "Min",
                           "Max",
                           "Stddev",
                           "StddevPop",
                           "VarSamp",
                           "VarPop",
                           "BitAnd",
                           "BitOr",
                           "BitXor"};
    for (std::size_t i = 0; i < names.size(); ++i) {
        if (name == names[i]) {
            return static_cast<AggregateFunction>(i);
        }
    }
    throw std::logic_error("Unknown fixture aggregate function");
}

template <class Request>
static Json emit(UserContext& user_context, Request query, const Json& input) {
    auto r = user_context.memory_resource();
    auto output = detail::model_json(r);
    detail::model_put(output, "query", debug_string(r, query.query()));
    if (std::get<bool>(field(input, "execute").data)) {
        teaql::test::initialize_dynamic_json_database(user_context);
        const auto& result = std::move(query)
                                 .comment("inspect projection and text")
                                 .purpose("qualify projection and text")
                                 .executeForRows(user_context)
                                 .wait();
        Json::Array rows(r);
        Json error(r);
        if (result) {
            for (const auto& row : result.value().data) {
                Json::Array columns(r);
                for (auto key : {std::string_view("id"),
                                 std::string_view("version"),
                                 text(field(input, "field")),
                                 text(field(input, "alias")),
                                 std::string_view("_group")}) {
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

template <class Request, class Apply, class Order>
static Json
text_query(UserContext& user_context, Request query, Apply apply, Order order, const Json& input) {
    auto operand = value(user_context.memory_resource(), field(input, "value"));
    for (int n = 0; n < (std::get<bool>(field(input, "repeat").data) ? 2 : 1); ++n) {
        apply(query, operand);
    }
    order(query);
    return emit(user_context, std::move(query), input);
}

template <class Request, class Apply, class Order>
static Json numeric_query(
    UserContext& user_context, Request query, Apply apply, Order order, const Json& input) {
    auto operation = function(text(field(input, "function")));
    auto alias = text(field(input, "alias"));
    for (const auto& item : std::get<Json::Array>(field(input, "operations").data)) {
        apply(query, text(item), alias, operation);
    }
    order(query);
    return emit(user_context, std::move(query), input);
}

static Json observe(UserContext& user_context, const Json& input) {
    auto Q = user_context.queries<query_factory>();
    bool minimal = std::get<bool>(field(input, "minimal").data);
    auto name = text(field(input, "field"));
    auto sorting = text(field(input, "order"));
#define ORDER(ASC, DESC, GBK)                                                                      \
    [sorting](auto& query) {                                                                       \
        if (sorting == "asc") {                                                                    \
            query.ASC();                                                                           \
        } else if (sorting == "desc") {                                                            \
            query.DESC();                                                                          \
        } else if (sorting == "gbk") {                                                             \
            query.GBK();                                                                           \
        }                                                                                          \
    }
#define TEXT(REQUEST, CONTAIN, NOT_CONTAIN, START, NOT_START, END, NOT_END, SOUND, ASC, DESC, GBK) \
    text_query(                                                                                    \
        user_context,                                                                              \
        REQUEST,                                                                                   \
        [operation = text(field(input, "operation"))](auto& query, const Value& operand) {         \
            if (operation == "contains") {                                                         \
                query.CONTAIN(*operand.try_text());                                                \
            } else if (operation == "not_contains") {                                              \
                query.NOT_CONTAIN(*operand.try_text());                                            \
            } else if (operation == "starts") {                                                    \
                query.START(*operand.try_text());                                                  \
            } else if (operation == "not_starts") {                                                \
                query.NOT_START(*operand.try_text());                                              \
            } else if (operation == "ends") {                                                      \
                query.END(*operand.try_text());                                                    \
            } else if (operation == "not_ends") {                                                  \
                query.NOT_END(*operand.try_text());                                                \
            } else {                                                                               \
                query.SOUND(operand);                                                              \
            }                                                                                      \
        },                                                                                         \
        ORDER(ASC, DESC, GBK),                                                                     \
        input)
#define NUMERIC(REQUEST, SELECT, RAW, REMOVE, DEFAULT, NAMED, GROUP, ASC, DESC, GBK)               \
    numeric_query(                                                                                 \
        user_context,                                                                              \
        REQUEST,                                                                                   \
        [](auto& query, std::string_view op, std::string_view alias, AggregateFunction function) { \
            if (op == "select") {                                                                  \
                query.SELECT();                                                                    \
            } else if (op == "raw") {                                                              \
                query.RAW("1");                                                                    \
            } else if (op == "remove") {                                                           \
                query.REMOVE();                                                                    \
            } else if (op == "default") {                                                          \
                query.DEFAULT(function);                                                           \
            } else if (op == "named") {                                                            \
                query.NAMED(alias, function);                                                      \
            } else if (op == "group") {                                                            \
                query.GROUP("_group");                                                             \
            }                                                                                      \
        },                                                                                         \
        ORDER(ASC, DESC, GBK),                                                                     \
        input)
    if (text(field(input, "route")) == "text") {
        if (name == "name") {
            return TEXT(minimal ? Q.peopleMinimal() : Q.people(),
                        whoseNameContains,
                        whoseNameNotContains,
                        whoseNameStartsWith,
                        whoseNameNotStartsWith,
                        whoseNameEndsWith,
                        whoseNameNotEndsWith,
                        whoseNameSoundsLike,
                        orderByNameAsc,
                        orderByNameDesc,
                        orderByNameAscUsingGbk);
        }
        return TEXT(minimal ? Q.ordersMinimal() : Q.orders(),
                    withCodeContaining,
                    withCodeNotContaining,
                    withCodeStartingWith,
                    withCodeNotStartingWith,
                    withCodeEndingWith,
                    withCodeNotEndingWith,
                    withCodeSoundingLike,
                    orderByCodeAsc,
                    orderByCodeDesc,
                    orderByCodeAscUsingGbk);
    }
    if (name == "priority") {
        return NUMERIC(minimal ? Q.ordersMinimal() : Q.orders(),
                       selectPriority,
                       selectPriorityRaw,
                       unselectPriority,
                       selectPriorityWithFunction,
                       selectPriorityAsWithFunction,
                       groupByPriorityAs,
                       orderByPriorityAsc,
                       orderByPriorityDesc,
                       orderByPriorityAscUsingGbk);
    }
    return NUMERIC(minimal ? Q.ordersMinimal() : Q.orders(),
                   selectAmount,
                   selectAmountRaw,
                   unselectAmount,
                   selectAmountWithFunction,
                   selectAmountAsWithFunction,
                   groupByAmountAs,
                   orderByAmountAsc,
                   orderByAmountDesc,
                   orderByAmountAscUsingGbk);
#undef NUMERIC
#undef TEXT
#undef ORDER
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
