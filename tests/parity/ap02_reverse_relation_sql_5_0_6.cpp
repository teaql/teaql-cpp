#include "ap02_reverse_relation_consumer.hpp"
#include <teaql/runtime/entity_expression.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <sqlite3.h>

using namespace teaql;
using namespace ap02_reverse_relation_consumer;

static const Json& field(const Json& value, std::string_view key) {
    const auto& object = std::get<Json::Object>(value.data);
    auto found = object.find(key);
    if (found == object.end()) {
        throw std::runtime_error("Missing query input field");
    }
    return found->second;
}

static Json child_value(UserContext& user_context, const Child& child) {
    auto r = user_context.memory_resource();
    Json::Object output(r), secret(r);
    auto result = child.evalSecret();
    if (result.is_value()) {
        secret.emplace("value", Json(r, E::child(child).getSecret().unwrap()));
    } else if (result.is_null()) {
        secret.emplace("null", Json(r, true));
    } else {
        secret.emplace("not_loaded", Json(r, true));
    }
    output.emplace("id", Json(r, child.id()));
    output.emplace("secret", Json(r, std::move(secret)));
    return Json(r, std::move(output));
}

static Json observe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto connection = user_context.make_resource_ref<SqliteConnection>();
    require_value(serviceRuntimeFromConnection(user_context, connection));
    user_context.with_active_root("Platform", 1);
    std::ifstream source(TEAQL_AP02_SQL_FIXTURE);
    std::ostringstream sql_source;
    sql_source << source.rdbuf();
    if (!source || sqlite3_exec(user_context.get(connection).native_handle(),
                                sql_source.str().c_str(),
                                nullptr,
                                nullptr,
                                nullptr) != SQLITE_OK) {
        throw std::runtime_error("SQLite fixture initialization failed");
    }
    auto Q = user_context.queries<query_factory>();
    auto query = Q.items().runtimeQuery();
    query.whereEqual("id", std::get<std::uint64_t>(field(input, "parent").data));
    if (std::get<bool>(field(input, "selected").data)) {
        EntityQuery<Child, ap02_reverse_relation_consumer::DataServiceExecutor> child_query(
            user_context, std::get<bool>(field(input, "minimal").data));
        child_query.orderAsc("id");
        query.selectRelation("links", child_query);
    }
    auto executable = std::move(query)
                          .comment("Inspect injected reverse relation")
                          .purpose("Qualify typed relation loading");
    const auto& result = executable.executeForList(user_context).wait();
    Json::Array rows(r);
    Json error(r);
    if (result) {
        for (const auto& item : result.value().data) {
            Json::Object row(r);
            Json::Array children(r);
            bool graph_present = false;
            if (std::get<bool>(field(input, "many").data)) {
                if (const auto* list = item.root.resolve_relation_list<Child>(
                        Item::ENTITY_NAME, item.id(), "links")) {
                    graph_present = true;
                    for (const auto& child : list->data) {
                        children.emplace_back(child_value(user_context, child));
                    }
                }
            } else if (const auto* edge = item.root.resolve_relation_option<Child>(
                           Item::ENTITY_NAME, item.id(), "links")) {
                graph_present = true;
                if (*edge) {
                    children.emplace_back(child_value(user_context, **edge));
                }
            }
            row.emplace("id", Json(r, item.id()));
            row.emplace("name", Json(r, E::item(item).getName().unwrap()));
            row.emplace("loaded", Json(r, item.load_state.is_loaded("links")));
            row.emplace("graph_present", Json(r, graph_present));
            row.emplace("children", Json(r, std::move(children)));
            EntityJsonTraversal traversal(std::allocator_arg, Allocator(r));
            auto json = item.borrowed_json(r, traversal);
            if (!json) {
                throw std::runtime_error("Generated JSON traversal failed");
            }
            row.emplace("json", std::move(*json));
            rows.emplace_back(std::move(row));
        }
    } else {
        error = Json(r, result.error().to_string(r));
    }
    Json::Array sql(r);
    for (const auto& entry : user_context.sql_logs()) {
        Json::Object value(r);
        value.emplace("sql", Json(r, entry.sql));
        value.emplace("comment", entry.comment ? Json(r, *entry.comment) : Json(r));
        value.emplace("purpose", entry.purpose ? Json(r, *entry.purpose) : Json(r));
        value.emplace("count", entry.result_count ? Json(r, *entry.result_count) : Json(r));
        sql.emplace_back(std::move(value));
    }
    Json::Object output(r);
    output.emplace("rows", Json(r, std::move(rows)));
    output.emplace("error", std::move(error));
    output.emplace("sql", Json(r, std::move(sql)));
    return Json(r, std::move(output));
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    for (std::string source; std::getline(std::cin, source);) {
        auto user_context = global.make_user_context();
        {
            auto r = user_context->memory_resource();
            auto input = Json::parse_result(r, source);
            std::cout << observe(*user_context, require_value(input)).dump(r) << '\n';
        }
        user_context.reset();
        if (global.live_user_bytes() != 0) {
            throw std::runtime_error("UserContext memory remains after close");
        }
    }
}
