#include "../generator/parity/relation_identity_contracts/cpp/include/cpp_relation_identities.hpp"
#include "fixture_value.hpp"
#include <teaql/core/query_debug.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <sqlite3.h>
using namespace cpp_relation_identities;

template <Entity T>
static Json snapshot(Memory r, const T& entity) {
    Json::Object output(r), loaded(r);
    auto descriptor = T::entity_descriptor(r);
    for (const auto& field : descriptor.properties) {
        loaded.emplace(String(field.name, r), Json(r, entity.is_field_loaded(field.name)));
    }
    for (const auto& relation : descriptor.relations) {
        loaded.emplace(String(relation.name, r), Json(r, entity.is_field_loaded(relation.name)));
    }
    T copy(std::allocator_arg, Allocator(r), entity);
    output.emplace("loaded", Json(r, loaded));
    output.emplace("json", std::move(copy).into_json(r));
    return Json(r, output);
}

template <Entity T, class Request, class Apply>
static Json run(UserContext& user_context, Request query, Apply apply, const Json& input) {
    auto r = user_context.memory_resource();
    for (const auto& operation : std::get<Json::Array>(field(input, "ops").data)) {
        apply(query, text(operation));
    }
    auto descriptor = T::entity_descriptor(r);
    Json::Array relations(r);
    for (const auto& relation : descriptor.relations) {
        Json::Object data(r);
        data.emplace("name", Json(r, relation.name));
        data.emplace("target", Json(r, relation.target_entity));
        data.emplace("local", Json(r, relation.local_key));
        data.emplace("foreign", Json(r, relation.foreign_key));
        data.emplace("many", Json(r, relation.many));
        data.emplace("attach", Json(r, relation.attach));
        data.emplace("delete_missing", Json(r, relation.delete_missing));
        relations.emplace_back(data);
    }
    Json::Object output(r);
    output.emplace("query", Json(r, debug_string(r, query.query())));
    output.emplace("relations", Json(r, relations));
    auto executable = std::move(query)
                          .comment("inspect canonical relationship identities")
                          .purpose("preserve query paths and graph reads");
    Json::Array rows(r);
    Json error(r);
    if (text(field(input, "mode")) == "rows") {
        const auto& result = executable.executeForRows(user_context).wait();
        if (result) {
            for (const auto& row : result.value().data) {
                Json::Object values(r);
                for (const auto& [key, value] : row.iter()) {
                    values.insert_or_assign(String(key, r), Json(r, debug_string(r, value)));
                }
                rows.emplace_back(values);
            }
        } else {
            error = Json(r, result.error().to_string(r));
        }
    } else {
        const auto& result = executable.executeForList(user_context).wait();
        if (result) {
            for (const auto& entity : result.value().data) {
                rows.emplace_back(snapshot(r, entity));
            }
        } else {
            error = Json(r, result.error().to_string(r));
        }
    }
    output.emplace("rows", Json(r, rows));
    output.emplace("error", std::move(error));
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

static Json observe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto connection = user_context.make_resource_ref<SqliteConnection>();
    auto& sqlite = user_context.get(connection);
    std::ifstream file(std::string(TEAQL_SOURCE_DIR) + "/tests/support/relation_identity_sql.sql");
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
    bool minimal = std::get<bool>(field(input, "minimal").data);
    if (text(field(input, "entity")) == "entry") {
        return run<Entry>(
            user_context,
            minimal ? Q.entriesMinimal() : Q.entries(),
            [&](auto& query, std::string_view op) {
                if (op == "self") {
                    query.selectSelf();
                } else if (op == "all") {
                    query.selectAll();
                } else if (op == "children") {
                    query.selectChildren();
                } else if (op == "first") {
                    query.selectModelNamespace();
                } else if (op == "second") {
                    query.selectModelOperator();
                } else if (op == "first_with") {
                    query.selectModelNamespaceWith(Q.owners().selectSelf().orderByIdAsc());
                } else if (op == "second_with") {
                    query.selectModelOperatorWith(Q.owners().selectSelf().orderByIdDesc());
                } else if (op == "remove_first") {
                    query.unselectModelNamespace();
                } else if (op == "deleted") {
                    query.withDeletedRows();
                } else if (op == "json") {
                    query.filterWithJson(text(field(input, "json")));
                } else {
                    throw std::logic_error("unknown relation fixture operation");
                }
            },
            input);
    }
    if (text(field(input, "entity")) == "owner") {
        return run<Owner>(
            user_context,
            minimal ? Q.ownersMinimal() : Q.owners(),
            [&](auto& query, std::string_view op) {
                if (op == "self") {
                    query.selectSelf();
                } else if (op == "all") {
                    query.selectAll();
                } else if (op == "children") {
                    query.selectChildren();
                } else if (op == "first") {
                    query.selectModelClass();
                } else if (op == "second") {
                    query.selectModelTemplate();
                } else if (op == "first_with") {
                    query.selectModelClassWith(Q.entries().selectSelf().orderByIdAsc());
                } else if (op == "second_with") {
                    query.selectModelTemplateWith(Q.entries().selectSelf().orderByIdDesc());
                } else if (op == "remove_first") {
                    query.selectSelf();
                } else if (op == "deleted") {
                    query.withDeletedRows();
                } else if (op == "json") {
                    query.filterWithJson(text(field(input, "json")));
                } else {
                    throw std::logic_error("unknown relation fixture operation");
                }
            },
            input);
    }
    if (text(field(input, "entity")) == "profile") {
        return run<Profile>(
            user_context,
            minimal ? Q.profilesMinimal() : Q.profiles(),
            [&](auto& query, std::string_view op) {
                if (op == "self") {
                    query.selectSelf();
                } else if (op == "all") {
                    query.selectAll();
                } else if (op == "children") {
                    query.selectChildren();
                } else if (op == "first") {
                    query.selectOwner();
                } else if (op == "second") {
                    query.selectOwner();
                } else if (op == "first_with") {
                    query.selectOwnerWith(Q.owners().selectSelf().orderByIdAsc());
                } else if (op == "second_with") {
                    query.selectOwnerWith(Q.owners().selectSelf().orderByIdDesc());
                } else if (op == "remove_first") {
                    query.unselectOwner();
                } else if (op == "deleted") {
                    query.withDeletedRows();
                } else if (op == "json") {
                    query.filterWithJson(text(field(input, "json")));
                } else {
                    throw std::logic_error("unknown relation fixture operation");
                }
            },
            input);
    }
    throw std::logic_error("unknown fixture entity");
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
                    throw std::runtime_error("invalid JSON input");
                }
                std::cout << observe(*user_context, *input).dump(user_context->memory_resource())
                          << '\n';
            }
            user_context.reset();
            if (global.live_user_bytes()) {
                throw std::logic_error("relationship probe retained User memory");
            }
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
