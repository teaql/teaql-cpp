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

struct PolicyObservation {
    using allocator_type = Allocator;
    Json::Array calls;
    std::uint64_t selects = 0;

    explicit PolicyObservation(std::allocator_arg_t, Allocator a) : calls(a.resource()) {}
};

struct JsonRequestPolicy final : RequestPolicy {
    using allocator_type = Allocator;
    String mode;
    PolicyObservation& observation;

    JsonRequestPolicy(std::allocator_arg_t,
                      Allocator a,
                      std::string_view mode,
                      PolicyObservation& observation)
        : mode(mode, a.resource()), observation(observation) {}

    RuntimeHookResult
    enforce_select(Memory r, const UserContext&, SelectQuery& query) const override {
        auto before = describe(r, query);
        if (mode == "restrict" || mode == "slice") {
            std::uint64_t id = query.entity == "Person"    ? 2
                               : query.entity == "Order"   ? 11
                               : query.entity == "Product" ? 6
                                                           : 21;
            auto predicate = Expr::eq(r, "id", id);
            query.filter(query.predicate ? query.predicate->and_expr(predicate)
                                         : std::move(predicate));
        }
        if (mode == "slice") {
            query.offset(0).limit(1);
        }
        Json::Object call(r);
        call.emplace("before", std::move(before));
        call.emplace("after", describe(r, query));
        call.emplace("comment", query.comment_text ? Json(r, *query.comment_text) : Json(r));
        call.emplace("purpose", query.purpose_text ? Json(r, *query.purpose_text) : Json(r));
        Json::Array aggregates(r), trace(r);
        for (const auto& aggregate : query.aggregates) {
            aggregates.emplace_back(debug_string(r, aggregate));
        }
        for (const auto& node : query.trace_chain) {
            trace.emplace_back(debug_string(r, node));
        }
        call.emplace("aggregates", Json(r, aggregates));
        call.emplace("trace", Json(r, trace));
        observation.calls.emplace_back(call);
        if (mode == "deny") {
            return RuntimeHookResult::err(r, RuntimeError::policy(r, "DYNAMIC_JSON_POLICY_DENIED"));
        }
        return RuntimeHookResult::ok(r, {});
    }
};

static int
observe_database(void* data, int action, const char*, const char*, const char*, const char*) {
    if (action == SQLITE_SELECT) {
        ++static_cast<PolicyObservation*>(data)->selects;
    }
    return SQLITE_OK;
}

static Json
policy_output(UserContext& user_context, const PolicyObservation& observation, Json result) {
    auto r = user_context.memory_resource();
    auto& out = std::get<Json::Object>(result.data);
    out.emplace("policy_calls", Json(r, observation.calls));
    out.emplace("database_selects", Json(r, observation.selects));
    Json::Array logs(r);
    for (const auto& entry : user_context.sql_logs()) {
        Json::Object log(r);
        log.emplace("sql", Json(r, entry.sql));
        log.emplace("comment", entry.comment ? Json(r, *entry.comment) : Json(r));
        log.emplace("purpose", entry.purpose ? Json(r, *entry.purpose) : Json(r));
        Json::Array trace(r);
        for (const auto& node : entry.trace_path) {
            trace.emplace_back(debug_string(r, node));
        }
        log.emplace("trace", Json(r, trace));
        Json::Array params(r);
        for (const auto& parameter : entry.params) {
            params.emplace_back(debug_string(r, parameter));
        }
        log.emplace("params", Json(r, params));
        logs.emplace_back(log);
    }
    out.emplace("logs", Json(r, logs));
    return result;
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
        Json::Array ids(r);
        Json error(r);
        auto mode = optional(input, "execution");
        if (mode && text(*mode) == "count") {
            const auto& result = query.executeForCount(user_context).wait();
            out.emplace("count", result ? Json(r, result.value()) : Json(r));
            if (!result) {
                error = Json(r, result.error().to_string(r));
            }
        } else if (mode && text(*mode) == "exists") {
            const auto& result = query.executeForExists(user_context).wait();
            out.emplace("exists", result ? Json(r, result.value()) : Json(r));
            if (!result) {
                error = Json(r, result.error().to_string(r));
            }
        } else if (mode && text(*mode) == "page") {
            const auto& result = query.executeForPage(user_context, 1, 1).wait();
            out.emplace("total",
                        result && result.value().total_count ? Json(r, *result.value().total_count)
                                                             : Json(r));
            if (result) {
                for (const auto& value : result.value().data) {
                    Json::Array row(r);
                    row.emplace_back(value.id());
                    row.emplace_back(value.version());
                    ids.emplace_back(row);
                }
            } else {
                error = Json(r, result.error().to_string(r));
            }
        } else if (mode && text(*mode) == "one") {
            const auto& result = query.executeForOne(user_context).wait();
            if (result) {
                if (result.value()) {
                    Json::Array row(r);
                    row.emplace_back(result.value()->id());
                    row.emplace_back(result.value()->version());
                    ids.emplace_back(row);
                }
            } else {
                error = Json(r, result.error().to_string(r));
            }
        } else if (mode && text(*mode) == "stream") {
            auto chunk = std::get<std::uint64_t>(field(input, "chunk").data);
            auto stop = std::get<std::uint64_t>(field(input, "stop").data);
            const auto& result =
                query.stream(static_cast<std::size_t>(chunk)).executeForStream(user_context).wait();
            if (result) {
                auto& stream = result.value().get();
                for (std::uint64_t index = 0; index < stop; ++index) {
                    auto* item = stream.wait_next();
                    if (!item) {
                        break;
                    }
                    if (!*item) {
                        error = Json(r, item->error().to_string(r));
                        break;
                    }
                    Json::Array row(r);
                    row.emplace_back(item->value().id());
                    row.emplace_back(item->value().version());
                    ids.emplace_back(row);
                }
                stream.cancel();
            } else {
                error = Json(r, result.error().to_string(r));
            }
            auto Q = user_context.queries<query_factory>();
            const auto& reused = Q.people()
                                     .whoseIdIs(std::uint64_t{2})
                                     .comment("what: reuse executor after JSON stream")
                                     .purpose("why: verify cursor lease release")
                                     .executeForOne(user_context)
                                     .wait();
            out.emplace("reused", Json(r, reused && bool(reused.value())));
        } else {
            const auto& result = query.executeForList(user_context).wait();
            if (result) {
                for (const auto& value : result.value().data) {
                    Json::Array row(r);
                    row.emplace_back(value.id());
                    row.emplace_back(value.version());
                    ids.emplace_back(row);
                }
            } else {
                error = Json(r, result.error().to_string(r));
            }
        }
        out.emplace(String("error", r), std::move(error));
        out.emplace(String("rows", r), Json(r, ids));
    }
    return Json(r, out);
}

static Json observe(UserContext& user_context, const Json& input) {
    PolicyObservation* observation = nullptr;
    sqlite3* database = nullptr;
    if (std::get<bool>(field(input, "execute").data)) {
        auto& connection = teaql::test::initialize_dynamic_json_database(user_context);
        if (auto policy = optional(input, "policy")) {
            observation = &user_context.make<PolicyObservation>();
            auto& instance = user_context.make<JsonRequestPolicy>(text(*policy), *observation);
            user_context.set_request_policy(user_context, instance);
            user_context.disable_sql_log();
            if (std::get<bool>(field(input, "logging").data)) {
                user_context.enable_select_sql_log();
            }
            if (sqlite3_set_authorizer(connection.native_handle(), observe_database, observation) !=
                SQLITE_OK) {
                throw std::runtime_error("Cannot install SQLite observer");
            }
            database = connection.native_handle();
        }
    }
    auto Q = user_context.queries<query_factory>();
    auto entity = text(field(input, "entity"));
    bool minimal = std::get<bool>(field(input, "minimal").data);
    auto result = [&]() {
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
    }();
    if (database) {
        sqlite3_set_authorizer(database, nullptr, nullptr);
    }
    return observation ? policy_output(user_context, *observation, std::move(result))
                       : std::move(result);
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
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
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
