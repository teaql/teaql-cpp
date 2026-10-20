#include "../generated_inheritance_graph/include/cpp_inheritance_graph.hpp"
#include "fixture_value.hpp"
#include <teaql/core/query_debug.hpp>
#include <teaql/data_service/sql_log_debug.hpp>
#include <iostream>
#include <sqlite3.h>
#include <fstream>
#include <sstream>
using namespace cpp_inheritance_graph;

static void initialize(UserContext& user_context) {
    auto& module = moduleWithBehaviorsAndCheckers(user_context);
    module.apply_to(user_context);
    user_context.set_metadata(user_context, module.metadata);
    auto& connection = user_context.make_resource<SqliteConnection>();
    auto& transport = user_context.make<SqliteMutationExecutor>(connection);
    use_sqlite_provider(user_context, user_context, transport);
    auto dialect = user_context.require_resource<SqliteDialect>();
    auto& state = user_context.make<SqlExecutorState>();
    auto executor = user_context.make_ref<cpp_inheritance_graph::DataServiceExecutor>(
        require_value(dialect).get(), transport, module.metadata, state);
    user_context.register_executor(executor);
    std::ifstream file(std::string(TEAQL_SOURCE_DIR) +
                       "/tests/support/dynamic_json_inherited_rows.sql");
    std::ostringstream sql;
    sql << file.rdbuf();
    if (!file ||
        sqlite3_exec(connection.native_handle(), sql.str().c_str(), nullptr, nullptr, nullptr) !=
            SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(connection.native_handle()));
    }
    std::ifstream values(std::string(TEAQL_SOURCE_DIR) +
                         "/tests/support/dynamic_json_privacy_values.sql");
    std::ostringstream values_sql;
    values_sql << values.rdbuf();
    if (!values ||
        sqlite3_exec(
            connection.native_handle(), values_sql.str().c_str(), nullptr, nullptr, nullptr) !=
            SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(connection.native_handle()));
    }
    user_context.with_active_root("Platform", 1);
}

template <class Request, class Facet>
static Json apply(UserContext& user_context, Request query, const Json& input, Facet attach) {
    auto r = user_context.memory_resource();
    auto output = detail::model_json(r);
    auto shape = text(field(input, "shape"));
    if (shape == "readonly") {
        query.createPropertyAs("_computed", "id + 7");
    }
    if (shape == "facet") {
        attach(query);
    }
    try {
        for (const auto& source : std::get<Json::Array>(field(input, "filters").data)) {
            query.filterWithJson(text(source));
        }
    } catch (const GeneratedDynamicJsonFormatError& error) {
        detail::model_put(output, "panic", error.to_string(r));
        return output;
    } catch (const std::exception& error) {
        detail::model_put(output, "panic", std::string_view(error.what()));
        return output;
    }
    detail::model_put(output, "query", debug_string(r, query.query()));
    initialize(user_context);
    if (!std::get<bool>(field(input, "logging").data)) {
        user_context.disable_sql_log();
    }
    auto executable = std::move(query)
                          .comment("inspect private binding PRIVATE-SCHOOL-CANARY")
                          .purpose("qualify privacy PRIVATE-SCHOOL-CANARY");
    Json::Array rows(r);
    Json error(r), scalar(r);
    auto mode = text(field(input, "mode"));
    if (mode == "count") {
        const auto& result = executable.executeForCount(user_context).wait();
        if (result) {
            scalar = Json(r, result.value());
        } else {
            error = Json(r, result.error().to_string(r));
        }
    } else if (mode == "exists") {
        const auto& result = executable.executeForExists(user_context).wait();
        if (result) {
            scalar = Json(r, result.value());
        } else {
            error = Json(r, result.error().to_string(r));
        }
    } else if (mode == "list") {
        const auto& result = executable.executeForList(user_context).wait();
        if (result) {
            for (const auto& value : result.value().data) {
                Json::Array row(r);
                row.emplace_back(value.id());
                row.emplace_back(value.version());
                const auto* computed = value.dynamic_property("_computed");
                row.emplace_back(computed ? Json(r, debug_string(r, *computed)) : Json(r));
                rows.emplace_back(row);
            }
        } else {
            error = Json(r, result.error().to_string(r));
        }
    } else {
        const auto& result = executable.executeForRows(user_context).wait();
        if (result) {
            for (const auto& row : result.value().data) {
                Json::Array columns(r);
                for (auto key : {"id",
                                 "version",
                                 "name",
                                 "rank",
                                 "platform_id",
                                 "advisor_id",
                                 "campus_code",
                                 "budget",
                                 "school_id",
                                 "note"}) {
                    auto* item = row.get(key);
                    columns.emplace_back(item ? Json(r, debug_string(r, *item)) : Json(r));
                }
                rows.emplace_back(columns);
            }
        } else {
            error = Json(r, result.error().to_string(r));
        }
    }
    detail::model_put(output, "rows", Json(r, rows));
    detail::model_put(output, "error", error);
    detail::model_put(output, "scalar", scalar);
    Json::Array logs(r);
    for (const auto& entry : user_context.sql_logs()) {
        Json::Object log(r);
        Json::Array params(r);
        for (const auto& p : entry.params) {
            params.emplace_back(debug_string(r, p));
        }
        log.emplace("sql", Json(r, entry.sql));
        log.emplace("params", Json(r, params));
        log.emplace("comment", entry.comment ? Json(r, *entry.comment) : Json(r));
        log.emplace("purpose", entry.purpose ? Json(r, *entry.purpose) : Json(r));
        log.emplace("debug_sql", Json(r, entry.debug_sql));
        log.emplace("pretty_sql", Json(r, entry.pretty_sql));
        log.emplace("context", Json(r, debug_string(r, entry.log_context)));
        Json::Array trace(r);
        for (const auto& node : entry.trace_path) {
            trace.emplace_back(debug_string(r, node));
        }
        log.emplace("trace", Json(r, trace));
        log.emplace("result_count", entry.result_count ? Json(r, *entry.result_count) : Json(r));
        log.emplace("result_type", entry.result_type ? Json(r, *entry.result_type) : Json(r));
        log.emplace("affected_rows", entry.affected_rows ? Json(r, *entry.affected_rows) : Json(r));
        log.emplace("summary", Json(r, entry.result_summary));
        logs.emplace_back(log);
    }
    detail::model_put(output, "sql", Json(r, logs));
    return output;
}

static Json observe(UserContext& user_context, const Json& input) {
    auto Q = user_context.queries<query_factory>();
    auto entity = text(field(input, "entity"));
    bool minimal = std::get<bool>(field(input, "minimal").data);
    if (entity == "school") {
        return apply(
            user_context, minimal ? Q.schoolsMinimal() : Q.schools(), input, [&](auto& root) {
                root.facetByAdvisorAsWithOptions(
                    "_privacyFacet",
                    Q.specialists().comment("inspect privacy facet PRIVATE-SCHOOL-CANARY"),
                    true);
            });
    }
    if (entity == "academy") {
        return apply(
            user_context, minimal ? Q.academiesMinimal() : Q.academies(), input, [&](auto& root) {
                root.facetByAdvisorAsWithOptions(
                    "_privacyFacet",
                    Q.specialists().comment("inspect privacy facet PRIVATE-SCHOOL-CANARY"),
                    true);
            });
    }
    if (entity == "research_academy") {
        return apply(user_context,
                     minimal ? Q.researchAcademiesMinimal() : Q.researchAcademies(),
                     input,
                     [&](auto& root) {
                         root.facetByAdvisorAsWithOptions(
                             "_privacyFacet",
                             Q.specialists().comment("inspect privacy facet PRIVATE-SCHOOL-CANARY"),
                             true);
                     });
    }
    return apply(
        user_context, minimal ? Q.enrollmentsMinimal() : Q.enrollments(), input, [&](auto& root) {
            root.facetBySchoolAsWithOptions(
                "_privacyFacet",
                Q.schools().comment("inspect privacy facet PRIVATE-SCHOOL-CANARY"),
                true);
        });
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
                throw std::logic_error("Extension retained User memory");
            }
        }
    } catch (const std::exception& e) {
        std::pmr::set_default_resource(previous);
        std::cerr << e.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
