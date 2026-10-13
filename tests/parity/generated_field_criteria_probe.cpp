#include "../support/dynamic_json_runtime_fixture.hpp"
#include "fixture_value.hpp"
#include <teaql/core/query_debug.hpp>
#include <iostream>

using namespace cpp_dynamic_json;

static request::FieldOperator operation(std::string_view name) {
    using O = request::FieldOperator;
    for (const auto& pair : {std::pair{"eq", O::Equal},
                             {"ne", O::NotEqual},
                             {"gt", O::GreaterThan},
                             {"gte", O::GreaterThanOrEqual},
                             {"lt", O::LessThan},
                             {"lte", O::LessThanOrEqual},
                             {"between", O::Between},
                             {"in", O::In},
                             {"not_in", O::NotIn},
                             {"contain", O::Contain},
                             {"not_contain", O::NotContain},
                             {"begin", O::BeginWith},
                             {"not_begin", O::NotBeginWith},
                             {"end", O::EndWith},
                             {"not_end", O::NotEndWith},
                             {"sound", O::SoundsLike},
                             {"null", O::IsNull},
                             {"not_null", O::IsNotNull}}) {
        if (name == pair.first) {
            return pair.second;
        }
    }
    throw std::invalid_argument("unknown operator");
}

template <class Request>
static Json emit(UserContext& user_context, Request query, Json criterion, const Json& input) {
    auto r = user_context.memory_resource();
    auto out = detail::model_json(r);
    detail::model_put(out, "query", debug_string(r, query.query()));
    detail::model_put(out, "criterion", criterion);
    if (auto* execute = optional(input, "execute"); execute && std::get<bool>(execute->data)) {
        teaql::test::initialize_dynamic_json_database(user_context);
        const auto& result = std::move(query)
                                 .comment("inspect field criteria")
                                 .purpose("qualify field criteria")
                                 .executeForRows(user_context)
                                 .wait();
        Json::Array rows(r);
        Json error(r);
        if (result) {
            for (const auto& row : result.value().data) {
                Json::Array fields(r);
                for (auto key : {"id", "version"}) {
                    auto* v = row.get(key);
                    fields.emplace_back(v ? Json(r, debug_string(r, *v)) : Json(r));
                }
                rows.emplace_back(fields);
            }
        } else {
            error = Json(r, result.error().to_string(r));
        }
        detail::model_put(out, "rows", rows);
        detail::model_put(out, "error", error);
    }
    return out;
}

template <class Request, class Direct, class Create>
static Json ordinary(UserContext& user_context,
                     Request query,
                     Direct direct,
                     Create create,
                     request::FieldOperator op,
                     Vector<Value>& values,
                     const Json& input) {
    auto r = user_context.memory_resource();
    Json diagnostic(r);
    auto repeat = optional(input, "repeat") && std::get<bool>(field(input, "repeat").data);
    if (text(field(input, "route")) == "criteria") {
        auto criterion = create(query, op, values);
        diagnostic = Json(r, debug_string(r, user_context.get(criterion)));
        query.appendSearchCriteria(criterion);
        if (repeat) {
            query.appendSearchCriteria(criterion);
        }
    } else {
        direct(query, op, values);
        if (repeat) {
            direct(query, op, values);
        }
    }
    return emit(user_context, std::move(query), std::move(diagnostic), input);
}

static Json observe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    Vector<Value> values(r);
    for (const auto& entry : std::get<Json::Array>(field(input, "values").data)) {
        values.emplace_back(value(r, entry));
    }
    auto Q = user_context.queries<query_factory>();
    auto op = operation(text(field(input, "operator")));
    auto route = text(field(input, "route"));
    auto name = text(field(input, "field"));
    bool minimal = std::get<bool>(field(input, "minimal").data);
#define ORDINARY(REQUEST, DIRECT, CREATE)                                                          \
    ordinary(                                                                                      \
        user_context,                                                                              \
        std::move(REQUEST),                                                                        \
        [](auto& q, auto op, auto& values) {                                                       \
            q.DIRECT(op, values);                                                                  \
        },                                                                                         \
        [](const auto& q, auto op, auto& values) {                                                 \
            return q.CREATE(op, values);                                                           \
        },                                                                                         \
        op,                                                                                        \
        values,                                                                                    \
        input)
#define RANGE(REQUEST, BEFORE, AFTER, BETWEEN)                                                     \
    [&] {                                                                                          \
        if (route == "before") {                                                                   \
            REQUEST.BEFORE(values.at(0));                                                          \
        } else if (route == "after") {                                                             \
            REQUEST.AFTER(values.at(0));                                                           \
        } else {                                                                                   \
            auto& range =                                                                          \
                user_context.make<request::DateRange<Value>>(values.at(0), values.at(1));          \
            REQUEST.BETWEEN(range);                                                                \
        }                                                                                          \
        return emit(user_context, std::move(REQUEST), Json(r), input);                             \
    }()
    if (text(field(input, "entity")) == "person") {
        auto query = minimal ? Q.peopleMinimal() : Q.people();
        if (route == "direct" || route == "criteria") {
            if (name == "id") {
                return ORDINARY(query, whoseId, createIdCriteria);
            }
            if (name == "email") {
                return ORDINARY(query, whoseEmail, createEmailCriteria);
            }
            return ORDINARY(query, whoseName, createNameCriteria);
        }
        if (name == "email") {
            return RANGE(query, whoseEmailBefore, whoseEmailAfter, whoseEmailBetweenRange);
        }
        return RANGE(query, whoseNameBefore, whoseNameAfter, whoseNameBetweenRange);
    }
    auto query = minimal ? Q.ordersMinimal() : Q.orders();
    if (route == "direct" || route == "criteria") {
        if (name == "id") {
            return ORDINARY(query, withId, createIdCriteria);
        }
        if (name == "priority") {
            return ORDINARY(query, withPriority, createPriorityCriteria);
        }
        return ORDINARY(query, withCode, createCodeCriteria);
    }
    if (name == "priority") {
        return RANGE(query, withPriorityBefore, withPriorityAfter, withPriorityBetweenRange);
    }
    return RANGE(query, withCodeBefore, withCodeAfter, withCodeBetweenRange);
#undef RANGE
#undef ORDINARY
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        auto input = Json::parse(r, line);
        if (!input) {
            throw std::runtime_error("Invalid criteria fixture");
        }
        auto out = detail::model_json(r);
        try {
            out = observe(*user_context, *input);
        } catch (const std::exception& error) {
            detail::model_put(out, "panic", error.what());
        }
        std::cout << out.dump(r) << '\n';
    }
}
