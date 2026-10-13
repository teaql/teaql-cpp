#include "../generated_dynamic_json/include/cpp_dynamic_json.hpp"
#include "fixture_value.hpp"
#include <teaql/core/query_debug.hpp>
#include "../support/dynamic_json_runtime_fixture.hpp"
#include <sstream>
#include <iostream>
#include <sqlite3.h>

using namespace cpp_dynamic_json;

static Json describe(Memory r, const SelectQuery& query) {
    Json::Object out(r);
    Json::Array projection(r), orders(r);
    for (const auto& name : query.projection) {
        projection.emplace_back(name);
    }
    for (const auto& order : query.order_by) {
        orders.emplace_back(debug_string(r, order));
    }
    out.emplace(String("entity", r), Json(r, query.entity));
    out.emplace(String("projection", r), Json(r, projection));
    out.emplace(String("filter", r),
                query.predicate ? Json(r, debug_string(r, *query.predicate)) : Json(r));
    out.emplace(String("orders", r), Json(r, orders));
    out.emplace(String("slice", r), query.slice ? Json(r, debug_string(r, *query.slice)) : Json(r));
    out.emplace(String("hard_limit", r), Json(r, query.hard_limit_value));
    return Json(r, out);
}

template <class Request>
static Json apply(UserContext& user_context, Request request, const Json& input) {
    auto r = user_context.memory_resource();
    Json::Object out(r);
    try {
        if (std::get<bool>(field(input, "pre_slice").data)) {
            request.offset(3).limit(7);
        }
        if (std::get<bool>(field(input, "unlimited").data)) {
            request.unlimited();
        }
        for (const auto& source : std::get<Json::Array>(field(input, "filters").data)) {
            request.filterWithJson(text(source));
        }
    } catch (const GeneratedDynamicJsonFormatError& error) {
        out.emplace(String("panic", r), Json(r, error.to_string(r)));
        return Json(r, out);
    } catch (const std::exception& error) {
        out.emplace(String("panic", r), Json(r, error.what()));
        return Json(r, out);
    }
    out.emplace(String("query", r), describe(r, request.query()));
    if (std::get<bool>(field(input, "execute").data)) {
        auto query = std::move(request)
                         .comment("what: inspect dynamic JSON query")
                         .purpose("why: qualify generated translation");
        const auto& result = query.executeForList(user_context).wait();
        Json::Array ids(r);
        if (result) {
            for (const auto& value : result.value().data) {
                Json::Array row(r);
                row.emplace_back(value.id());
                row.emplace_back(value.version());
                ids.emplace_back(row);
            }
            out.emplace(String("error", r), Json(r));
        } else {
            out.emplace(String("error", r), Json(r, result.error().to_string(r)));
        }
        out.emplace(String("rows", r), Json(r, ids));
    }
    return Json(r, out);
}

static Json observe(UserContext& user_context, const Json& input) {
    if (std::get<bool>(field(input, "execute").data)) {
        teaql::test::initialize_dynamic_json_database(user_context);
    }
    auto Q = user_context.queries<query_factory>();
    auto entity = text(field(input, "entity"));
    bool minimal = std::get<bool>(field(input, "minimal").data);
    if (entity == "person") {
        return apply(user_context, minimal ? Q.peopleMinimal() : Q.people(), input);
    }
    if (entity == "product") {
        return apply(user_context, minimal ? Q.productsMinimal() : Q.products(), input);
    }
    if (entity == "line") {
        return apply(user_context, minimal ? Q.linesMinimal() : Q.lines(), input);
    }
    return apply(user_context, minimal ? Q.ordersMinimal() : Q.orders(), input);
}

int main() {
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            auto input = Json::parse(user_context->memory_resource(), line);
            if (!input) {
                throw std::runtime_error("invalid dynamic JSON fixture");
            }
            std::cout << observe(*user_context, *input).dump(user_context->memory_resource())
                      << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
