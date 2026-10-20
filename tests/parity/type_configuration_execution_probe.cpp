#include "fixture_value.hpp"
#include <teaql/core/query_debug.hpp>
#include <sqlite3.h>
#include <iostream>
#include <type_traits>

using namespace teaql;

static Json table(Memory r, sqlite3* database, std::string_view name) {
    sqlite3_stmt* raw = nullptr;
    if (sqlite3_prepare_v2(database,
                           "SELECT count(*) FROM sqlite_master WHERE type='table' AND name=?1",
                           -1,
                           &raw,
                           nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
    std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> exists(raw, sqlite3_finalize);
    sqlite3_bind_text(raw, 1, name.data(), static_cast<int>(name.size()), SQLITE_TRANSIENT);
    if (sqlite3_step(raw) != SQLITE_ROW) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
    if (!sqlite3_column_int64(raw, 0)) {
        return Json(r);
    }
    exists.reset();
    String query("SELECT * FROM ", r);
    query += name;
    query += " ORDER BY 1";
    if (sqlite3_prepare_v2(database, query.c_str(), -1, &raw, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
    std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> statement(raw, sqlite3_finalize);
    Json::Array rows(r);
    int status;
    while ((status = sqlite3_step(raw)) == SQLITE_ROW) {
        Json::Object row(r);
        for (int index = 0; index < sqlite3_column_count(raw); ++index) {
            Json value(r);
            switch (sqlite3_column_type(raw, index)) {
            case SQLITE_NULL:
                break;
            case SQLITE_INTEGER:
                value = Json(r, static_cast<std::int64_t>(sqlite3_column_int64(raw, index)));
                break;
            case SQLITE_FLOAT:
                value = Json(r, sqlite3_column_double(raw, index));
                break;
            default:
                value = Json(
                    r,
                    std::string_view(reinterpret_cast<const char*>(sqlite3_column_text(raw, index)),
                                     static_cast<std::size_t>(sqlite3_column_bytes(raw, index))));
                break;
            }
            row.emplace(String(sqlite3_column_name(raw, index), r), std::move(value));
        }
        rows.emplace_back(row);
    }
    if (status != SQLITE_DONE) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
    return Json(r, rows);
}

static Json snapshot(Memory r, sqlite3* database) {
    Json::Object result(r);
    result.emplace("platform", table(r, database, "platform_data"));
    result.emplace("priority", table(r, database, "priority_data"));
    result.emplace("ids", table(r, database, "teaql_id_space"));
    return Json(r, result);
}

static void fixture_sql(sqlite3* database, const char* query) {
    if (sqlite3_exec(database, query, nullptr, nullptr, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
}

template <class Request>
static void typed_filter(Request& request, std::int64_t value) {
    using Scalar =
        decltype(std::declval<const cpp_type_configuration_fixture::Priority&>().value());
    if constexpr (std::is_integral_v<std::remove_cvref_t<Scalar>>) {
        request.withValueIs(value);
    } else {
        request.withValueIs(std::to_string(value));
    }
}

static Json reads(UserContext& user_context, Memory r) {
    namespace domain = cpp_type_configuration_fixture;
    auto Q = user_context.queries<domain::query_factory>();
    Json::Array output(r);
    for (auto filter : {-1, 7, 9, 8}) {
        auto request = Q.priorities().selectValue().orderByIdAsc();
        if (filter >= 0) {
            typed_filter(request, filter);
        }
        auto executable = std::move(request)
                              .comment("what: read configured scalar values")
                              .purpose("why: verify generated type and expression semantics");
        const auto& result = executable.executeForList(user_context).wait();
        Json::Object query(r);
        query.emplace("filter", Json(r, std::int64_t(filter)));
        query.emplace("error", result ? Json(r) : Json(r, result.error().to_string(r)));
        Json::Array rows(r);
        if (result) {
            for (const auto& entity : result.value().data) {
                Json::Object row(r);
                row.emplace("id", Json(r, entity.id()));
                row.emplace("version", Json(r, entity.version()));
                row.emplace("value", Json(r, entity.value()));
                auto expression = domain::E::priority(entity).getValue().eval();
                row.emplace("e", expression ? Json(r, *expression) : Json(r));
                EntityJsonTraversal traversal(std::allocator_arg,
                                              Allocator(user_context.memory_resource()));
                auto presentation = entity.borrowed_json(user_context.memory_resource(), traversal);
                row.emplace("json", presentation ? Json(r, *presentation) : Json(r));
                rows.emplace_back(row);
            }
        }
        query.emplace("rows", Json(r, rows));
        output.emplace_back(query);
    }
    return Json(r, output);
}

static Json observe(GlobalContext& global, const Json& input) {
    namespace domain = cpp_type_configuration_fixture;
    auto r = global.memory_resource();
    auto& connection = global.make_resource<SqliteConnection>();
    auto& transport = global.make<SqliteMutationExecutor>(connection);
    auto* database = connection.native_handle();
    auto create = [&] {
        auto owner = global.make_user_context();
        auto& user_context = *owner;
        auto mode = text(field(input, "module"));
        auto& module = mode == "plain"      ? domain::module(user_context)
                       : mode == "checker"  ? domain::moduleWithCheckers(user_context)
                       : mode == "behavior" ? domain::moduleWithBehaviors(user_context)
                                            : domain::moduleWithBehaviorsAndCheckers(user_context);
        module.apply_to(user_context);
        use_sqlite_provider(user_context, global, transport);
        auto& ids = user_context.make<SqliteIdSpaceGenerator>(transport);
        user_context.set_internal_id_generator(user_context, ids);
        auto& state = user_context.make<SqlExecutorState>();
        auto executor = user_context.make_ref<domain::ServiceRuntimeExecutor>(
            user_context.require_resource<SqliteDialect>().value().get(),
            transport,
            module.metadata,
            state);
        user_context.register_executor(executor);
        user_context.disable_sql_log();
        if (std::get<bool>(field(input, "logging").data)) {
            user_context.enable_all_sql_log();
        }
        return owner;
    };
    auto state = text(field(input, "state"));
    Json warmup_error(r);
    if (state != "fresh") {
        auto user_context = create();
        const auto& result = user_context->ensure_schema().wait();
        if (!result) {
            warmup_error = Json(r, result.error().to_string(r));
        }
        user_context.reset();
        if (std::holds_alternative<std::monostate>(warmup_error.data)) {
            if (state == "drift") {
                fixture_sql(database,
                            "UPDATE priority_data SET name='stale',value=8,version=3 WHERE id=1");
            } else if (state == "missing") {
                fixture_sql(database, "DELETE FROM priority_data WHERE id=22");
            } else if (state == "deleted") {
                fixture_sql(database, "UPDATE priority_data SET version=-1 WHERE id=1");
            } else if (state == "extra") {
                fixture_sql(
                    database,
                    "INSERT INTO priority_data(id,platform,name,code,value,version) VALUES(777,1,'Extra','EXTRA',8,4)");
            } else if (state != "existing") {
                throw std::runtime_error("Unknown configured-type stored state");
            }
        }
    }
    Json::Array rounds(r);
    auto count = std::get<std::uint64_t>(field(input, "rounds").data);
    for (std::uint64_t index = 0; index < count; ++index) {
        Json::Object round(r);
        round.emplace("before", snapshot(r, database));
        auto user_context = create();
        const auto& result = user_context->ensure_schema().wait();
        round.emplace("error", result ? Json(r) : Json(r, result.error().to_string(r)));
        auto root = user_context->require_active_root("Platform");
        round.emplace("root",
                      root ? Json(r, root.value().get().id) : Json(r, root.error().to_string(r)));
        round.emplace("after", snapshot(r, database));
        round.emplace("reads", reads(*user_context, r));
        Json::Array logs(r);
        for (const auto& entry : user_context->sql_logs()) {
            Json::Object item(r);
            item.emplace("sql", Json(r, entry.sql));
            item.emplace("comment", entry.comment ? Json(r, *entry.comment) : Json(r));
            item.emplace("purpose", entry.purpose ? Json(r, *entry.purpose) : Json(r));
            item.emplace("audit_as", entry.audit_reason ? Json(r, *entry.audit_reason) : Json(r));
            Json::Array parameters(r);
            for (const auto& value : entry.params) {
                parameters.emplace_back(debug_string(r, value));
            }
            item.emplace("params", Json(r, parameters));
            logs.emplace_back(item);
        }
        round.emplace("logs", Json(r, logs));
        user_context.reset();
        rounds.emplace_back(round);
    }
    Json::Object output(r);
    output.emplace("warmup_error", std::move(warmup_error));
    output.emplace("rounds", Json(r, rounds));
    return Json(r, output);
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto input = Json::parse(global.memory_resource(), line);
            if (!input) {
                throw std::runtime_error("Invalid configured-type input");
            }
            std::cout << observe(global, *input).dump(global.memory_resource()) << '\n';
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
