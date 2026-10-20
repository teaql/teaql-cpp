#include "../generator/parity/query_alias_contracts/cpp/include/cpp_query_aliases.hpp"
#include "fixture_value.hpp"
#include <teaql/core/query_debug.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <sqlite3.h>
using namespace cpp_query_aliases;

static Json observe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto connection = user_context.make_resource_ref<SqliteConnection>();
    auto& sqlite = user_context.get(connection);
    std::ifstream file(std::string(TEAQL_SOURCE_DIR) + "/tests/support/query_alias_sql.sql");
    std::ostringstream sql;
    sql << file.rdbuf();
    if (!file ||
        sqlite3_exec(sqlite.native_handle(), sql.str().c_str(), nullptr, nullptr, nullptr) !=
            SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(sqlite.native_handle()));
    }
    require_value(serviceRuntimeFromConnection(user_context, connection));
    user_context.with_active_root("Platform", 1);
    auto Q = user_context.queries<query_factory>();
    auto query = std::get<bool>(field(input, "minimal").data) ? Q.recordsMinimal() : Q.records();
    if (std::get<bool>(field(input, "deleted").data)) {
        query.withDeletedRows();
    }
    auto name = text(field(input, "field"));
    auto selection = text(field(input, "selection"));
    bool canonical = text(field(input, "style")) == "canonical";
    auto select = [&] {
        if (name == "class") {
            if (canonical) {
                query.selectClass();
            } else {
                query.selectModelClass();
            }
        } else if (name == "operator") {
            if (canonical) {
                query.selectOperator();
            } else {
                query.selectModelOperator();
            }
        } else {
            if (canonical) {
                query.selectTemplate();
            } else {
                query.selectModelTemplate();
            }
        }
    };
    auto remove = [&] {
        if (name == "class") {
            if (canonical) {
                query.unselectClass();
            } else {
                query.unselectModelClass();
            }
        } else if (name == "operator") {
            if (canonical) {
                query.unselectOperator();
            } else {
                query.unselectModelOperator();
            }
        } else {
            if (canonical) {
                query.unselectTemplate();
            } else {
                query.unselectModelTemplate();
            }
        }
    };
    if (selection == "selected" || selection == "removed" || selection == "replaced") {
        select();
    }
    if (selection == "removed" || selection == "replaced") {
        remove();
    }
    if (selection == "replaced") {
        select();
    }
    if (selection == "self") {
        query.selectSelf();
    }
    Json::Object output(r);
    output.emplace("query", Json(r, debug_string(r, query.query())));
    output.emplace("present", Json(r, false));
    output.emplace("value", Json(r));
    output.emplace("diagnostic", Json(r));
    output.emplace("error", Json(r));
    auto request = std::move(query)
                       .withIdIs(std::get<std::uint64_t>(field(input, "id").data))
                       .comment("Inspect a keyword scalar")
                       .purpose("Apply the actual missing-load repair");
    const auto& loaded = request.executeForOne(user_context).wait();
    if (!loaded) {
        output.insert_or_assign("error", Json(r, loaded.error().to_string(r)));
    } else if (loaded.value()) {
        output.insert_or_assign("present", Json(r, true));
        try {
            auto entity = E::record(*loaded.value());
            auto expression = name == "class"
                                  ? (canonical ? entity.getClass() : entity.getModelClass())
                              : name == "operator"
                                  ? (canonical ? entity.getOperator() : entity.getModelOperator())
                                  : (canonical ? entity.getTemplate() : entity.getModelTemplate());
            String value(r);
            if (text(field(input, "action")) == "eval") {
                auto result = expression.eval();
                DebugWriter writer(value, {});
                if (result) {
                    writer.tuple("Some", [&](auto& inner) {
                        write_debug(inner, *result);
                    });
                } else {
                    writer.raw("None");
                }
            } else {
                value = debug_string(r, expression.unwrap());
            }
            output.insert_or_assign("value", Json(r, value));
        } catch (const ExpressionNotLoadedError& error) {
            output.insert_or_assign("diagnostic", error.diagnostic().to_json(r));
        }
    }
    Json::Array logs(r);
    for (const auto& entry : user_context.sql_logs()) {
        Json::Object log(r);
        Json::Array params(r), trace(r);
        for (const auto& value : entry.params) {
            params.emplace_back(debug_string(r, value));
        }
        for (const auto& node : entry.trace_path) {
            trace.emplace_back(debug_string(r, node));
        }
        log.emplace("sql", Json(r, entry.sql));
        log.emplace("params", Json(r, params));
        log.emplace("comment", entry.comment ? Json(r, *entry.comment) : Json(r));
        log.emplace("purpose", entry.purpose ? Json(r, *entry.purpose) : Json(r));
        log.emplace("trace", Json(r, trace));
        logs.emplace_back(log);
    }
    output.emplace("logs", Json(r, logs));
    return Json(r, output);
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            {
                auto input = Json::parse(user_context->memory_resource(), line);
                if (!input) {
                    throw std::runtime_error("Invalid scalar diagnostic fixture");
                }
                std::cout << observe(*user_context, *input).dump(user_context->memory_resource())
                          << '\n';
            }
            user_context.reset();
            if (global.live_user_bytes()) {
                throw std::logic_error("Scalar fixture retained User memory");
            }
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
