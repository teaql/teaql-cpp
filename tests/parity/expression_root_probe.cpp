#include "../generator/parity/expression_root_contracts/cpp/include/cpp_expression_roots.hpp"
#include "fixture_value.hpp"
#include <teaql/core/query_debug.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <sqlite3.h>
using namespace cpp_expression_roots;

template <class Request, class Entry>
static Json apply(UserContext& user_context, Request query, const Json& input, Entry entry) {
    auto r = user_context.memory_resource();
    auto Q = user_context.queries<query_factory>();
    if (std::get<bool>(field(input, "deleted").data)) {
        query.withDeletedRows();
    }
    auto selection = text(field(input, "selection"));
    if (selection == "name") {
        query.selectName();
    } else if (selection == "removed") {
        query.selectName().unselectName();
    } else if (selection == "replaced") {
        query.selectName().unselectName().selectName();
    } else if (selection == "platform_minimal") {
        query.selectPlatformWith(Q.platformsMinimal());
    } else if (selection == "platform_named") {
        query.selectPlatformWith(Q.platformsMinimal().selectName());
    } else if (selection == "self") {
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
                       .comment("Inspect expression root identity")
                       .purpose("Use the actual generated E entry");
    const auto& result = request.executeForOne(user_context).wait();
    if (!result) {
        output.insert_or_assign("error", Json(r, result.error().to_string(r)));
    } else if (result.value()) {
        output.insert_or_assign("present", Json(r, true));
        try {
            auto expression = entry(*result.value());
            auto value = text(field(input, "path")) == "platform"
                             ? expression.getPlatform().getName()
                             : expression.getName();
            String rendered(r);
            if (text(field(input, "action")) == "eval") {
                auto evaluated = value.eval();
                DebugWriter writer(rendered, {});
                if (evaluated) {
                    writer.tuple("Some", [&](auto& out) {
                        write_debug(out, *evaluated);
                    });
                } else {
                    writer.raw("None");
                }
            } else {
                rendered = debug_string(r, value.unwrap());
            }
            output.insert_or_assign("value", Json(r, rendered));
        } catch (const ExpressionNotLoadedError& error) {
            output.insert_or_assign("diagnostic", error.diagnostic().to_json(r));
        }
    }
    Json::Array logs(r);
    for (const auto& log : user_context.sql_logs()) {
        Json::Object item(r);
        Json::Array params(r), trace(r);
        for (const auto& value : log.params) {
            params.emplace_back(debug_string(r, value));
        }
        for (const auto& node : log.trace_path) {
            trace.emplace_back(debug_string(r, node));
        }
        item.emplace("sql", Json(r, log.sql));
        item.emplace("params", Json(r, params));
        item.emplace("trace", Json(r, trace));
        item.emplace("comment", log.comment ? Json(r, *log.comment) : Json(r));
        item.emplace("purpose", log.purpose ? Json(r, *log.purpose) : Json(r));
        logs.emplace_back(item);
    }
    output.emplace("logs", Json(r, logs));
    return Json(r, output);
}

static Json observe(UserContext& user_context, const Json& input) {
    auto connection = user_context.make_resource_ref<SqliteConnection>();
    auto& sqlite = user_context.get(connection);
    std::ifstream file(std::string(TEAQL_SOURCE_DIR) + "/tests/support/expression_root_sql.sql");
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
    auto entity = text(field(input, "entity"));
    if (entity == "class") {
        return apply(user_context, Q.classesMinimal(), input, [](const Class& value) {
            return E::modelClass(value);
        });
    }
    if (entity == "operator") {
        return apply(user_context, Q.operatorsMinimal(), input, [](const Operator& value) {
            return E::modelOperator(value);
        });
    }
    if (entity == "template") {
        return apply(user_context, Q.templatesMinimal(), input, [](const Template& value) {
            return E::modelTemplate(value);
        });
    }
    if (entity == "namespace") {
        return apply(user_context, Q.namespacesMinimal(), input, [](const Namespace& value) {
            return E::modelNamespace(value);
        });
    }
    return apply(user_context, Q.plainRecordsMinimal(), input, [](const PlainRecord& value) {
        return E::plainRecord(value);
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
                if (!input) {
                    throw std::runtime_error("Invalid root identity input");
                }
                std::cout << observe(*user_context, *input).dump(user_context->memory_resource())
                          << '\n';
            }
            user_context.reset();
            if (global.live_user_bytes()) {
                throw std::logic_error("Expression root fixture retained User memory");
            }
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
