#include "fixture_value.hpp"
#include <teaql/core/query_debug.hpp>
#include <teaql/runtime/checker_model_debug.hpp>
#include <sqlite3.h>
#include <iostream>

using namespace teaql;
namespace domain = constant_topology_fixture;

template <class Model, class BaseLogic>
struct ExtensionLogic : BaseLogic {
    using allocator_type = Allocator;
    String mode;
    Json::Array& hooks;

    ExtensionLogic(std::allocator_arg_t, Allocator a, std::string_view mode, Json::Array& hooks)
        : BaseLogic(std::allocator_arg, a), mode(mode, a.resource()), hooks(hooks) {}

    void record(Model& entity,
                CheckObjectStatus status,
                const ObjectLocation& location,
                CheckResults& results) const {
        auto r = hooks.get_allocator().resource();
        String before(entity.code(), r);
        if (mode == "fix") {
            String changed(before, entity.memory_resource());
            changed += "-fixed";
            entity.updateCode(changed);
        }
        if (mode == "deny") {
            auto resource = results.get_allocator().resource();
            ObjectLocation path(std::allocator_arg, Allocator(resource), location);
            results.push_back(CheckResult::required(resource, std::move(path).member("code")));
        }
        Json::Object entry(r);
        entry.emplace("kind", Json(r, "checker"));
        entry.emplace("model", Json(r, Model::ENTITY_NAME));
        entry.emplace("status", Json(r, debug_string(r, status)));
        entry.emplace("before", Json(r, before));
        entry.emplace("after", Json(r, entity.code()));
        hooks.emplace_back(entry);
    }
};

struct CategoryLogic : ExtensionLogic<domain::Category, domain::CategoryCheckerLogic> {
    using ExtensionLogic::ExtensionLogic;

    void checkAndFixCategory(const UserContext&,
                             domain::Category& entity,
                             CheckObjectStatus status,
                             const ObjectLocation& location,
                             CheckResults& results) const override {
        record(entity, status, location, results);
    }
};

struct StatusLogic : ExtensionLogic<domain::Status, domain::StatusCheckerLogic> {
    using ExtensionLogic::ExtensionLogic;

    void checkAndFixStatus(const UserContext&,
                           domain::Status& entity,
                           CheckObjectStatus status,
                           const ObjectLocation& location,
                           CheckResults& results) const override {
        record(entity, status, location, results);
    }
};

struct ExtensionBehavior final : EntityDataServiceBehavior {
    using allocator_type = Allocator;
    String mode;
    Json::Array& hooks;

    ExtensionBehavior(std::allocator_arg_t, Allocator a, std::string_view mode, Json::Array& hooks)
        : mode(mode, a.resource()), hooks(hooks) {}

    RuntimeHookResult
    before_select(Memory r, const UserContext&, SelectQuery& query) const override {
        auto destination = hooks.get_allocator().resource();
        Json::Object entry(destination);
        entry.emplace("kind", Json(destination, "select"));
        entry.emplace("query", Json(destination, debug_string(destination, query)));
        hooks.emplace_back(entry);
        if (mode == "deny") {
            return RuntimeHookResult::err(r, RuntimeError::event(r, "topology select denial"));
        }
        return RuntimeHookResult::ok(r, {});
    }
};

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
    result.emplace("status", table(r, database, "status_data"));
    result.emplace("category", table(r, database, "category_data"));
    result.emplace("ids", table(r, database, "teaql_id_space"));
    return Json(r, result);
}

static void fixture_sql(sqlite3* database, const char* query) {
    if (sqlite3_exec(database, query, nullptr, nullptr, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
}

template <class F>
static void expression_read(Json::Object& row, Memory r, std::string_view prefix, F&& read) {
    String value_name(prefix, r), state_name(prefix, r);
    value_name += "_code";
    state_name += "_state";
    try {
        auto value = read().eval();
        row.emplace(value_name, value ? Json(r, *value) : Json(r));
        row.emplace(state_name, Json(r, value ? "Value" : "Null"));
    } catch (const ExpressionNotLoadedError& error) {
        std::cerr << error.diagnostic().to_json(r).dump(r) << '\n';
        row.emplace(value_name, Json(r));
        row.emplace(state_name, Json(r, "NotLoaded"));
    }
}

static Json observe(GlobalContext& global, const Json& input) {
    namespace domain = constant_topology_fixture;
    auto r = global.memory_resource();
    auto& connection = global.make_resource<SqliteConnection>();
    auto& transport = global.make<SqliteMutationExecutor>(connection);
    auto* database = connection.native_handle();
    auto& hooks = global.make<Json::Array>();
    auto create = [&](bool extensions) {
        auto owner = global.make_user_context();
        auto& user_context = *owner;
        auto mode = text(field(input, "module"));
        auto& module = mode == "plain"      ? domain::module(user_context)
                       : mode == "checker"  ? domain::moduleWithCheckers(user_context)
                       : mode == "behavior" ? domain::moduleWithBehaviors(user_context)
                                            : domain::moduleWithBehaviorsAndCheckers(user_context);
        if (extensions) {
            auto checker_mode = text(field(input, "checker"));
            if (checker_mode != "none" && text(field(input, "target")) == "category") {
                auto& logic = user_context.make<CategoryLogic>(checker_mode, hooks);
                auto& checker = user_context.make<domain::CategoryChecker>(logic);
                auto& adapter =
                    user_context
                        .make<TypedEntityChecker<domain::Category, domain::CategoryChecker>>(
                            user_context, checker);
                module.checker(user_context, adapter);
            } else if (checker_mode != "none") {
                auto& logic = user_context.make<StatusLogic>(checker_mode, hooks);
                auto& checker = user_context.make<domain::StatusChecker>(logic);
                auto& adapter =
                    user_context.make<TypedEntityChecker<domain::Status, domain::StatusChecker>>(
                        user_context, checker);
                module.checker(user_context, adapter);
            }
            auto behavior_mode = text(field(input, "behavior"));
            if (behavior_mode != "none") {
                auto& behavior = user_context.make<ExtensionBehavior>(behavior_mode, hooks);
                module.entity_with_behavior<domain::Status>(user_context, behavior);
            }
        }
        module.apply_to(user_context);
        use_sqlite_provider(user_context, global, transport);
        auto& ids = user_context.make<SqliteIdSpaceGenerator>(transport);
        user_context.set_internal_id_generator(user_context, ids);
        auto& cache = user_context.make<SqlExecutorState>();
        auto executor = user_context.make_ref<domain::ServiceRuntimeExecutor>(
            user_context.require_resource<SqliteDialect>().value().get(),
            transport,
            module.metadata,
            cache);
        user_context.register_executor(executor);
        user_context.set_trace_id("constant-topology");
        user_context.set_user_identifier("fixture");
        if (std::get<bool>(field(input, "logging").data)) {
            user_context.enable_all_sql_log();
        } else {
            user_context.disable_sql_log();
        }
        return owner;
    };
    auto state = text(field(input, "state"));
    Json warmup_error(r);
    if (state != "fresh") {
        auto user_context = create(false);
        const auto& result = user_context->ensure_schema().wait();
        if (!result) {
            warmup_error = Json(r, result.error().to_string(r));
        }
        user_context.reset();
        if (std::holds_alternative<std::monostate>(warmup_error.data)) {
            if (state == "drift") {
                fixture_sql(
                    database,
                    "UPDATE category_data SET code='STALE',version=3 WHERE id=(SELECT MIN(id) FROM category_data); UPDATE status_data SET code='STALE',version=3 WHERE id=(SELECT MIN(id) FROM status_data)");
            } else if (state == "missing") {
                fixture_sql(
                    database,
                    "DELETE FROM status_data WHERE id=12; DELETE FROM category_data WHERE id=22");
            } else if (state == "deleted") {
                fixture_sql(
                    database,
                    "UPDATE category_data SET version=-1 WHERE id=(SELECT MIN(id) FROM category_data); UPDATE status_data SET version=-1 WHERE id=11");
            } else if (state == "extra") {
#ifdef TEAQL_TOPOLOGY_SELF_PARENT
                fixture_sql(
                    database,
                    "INSERT INTO category_data(id,platform,parent,code,version) VALUES(999,1,1,'EXTRA_CATEGORY',4)");
#else
                fixture_sql(
                    database,
                    "INSERT INTO category_data(id,platform,code,version) VALUES(999,1,'EXTRA_CATEGORY',4)");
#endif
                fixture_sql(
                    database,
                    "INSERT INTO status_data(id,platform,category,code,version) VALUES(777,1,999,'EXTRA',4)");
            } else if (state != "existing") {
                throw std::runtime_error("Unknown constant topology state");
            }
        }
    }
    Json::Array rounds(r);
    for (std::uint64_t index = 0; index < std::get<std::uint64_t>(field(input, "rounds").data);
         ++index) {
        Json::Object round(r);
        round.emplace("before", snapshot(r, database));
        auto user_context = create(true);
        const auto& result = user_context->ensure_schema().wait();
        round.emplace("error", result ? Json(r) : Json(r, result.error().to_string(r)));
        auto root = user_context->require_active_root("Platform");
        round.emplace("root",
                      root ? Json(r, root.value().get().id) : Json(r, root.error().to_string(r)));
        auto Q = user_context->queries<domain::query_factory>();
        auto categories = Q.categories().selectSelfFields();
#ifdef TEAQL_TOPOLOGY_SELF_PARENT
        categories.selectParent();
#endif
        const auto& queried = Q.statuses()
                                  .selectSelfFields()
                                  .selectCategoryWith(categories)
                                  .orderByIdAsc()
                                  .comment("what: inspect constant topology rows")
                                  .purpose("why: verify generated identifier and seed behavior")
                                  .executeForList(*user_context)
                                  .wait();
        round.emplace("query_error", queried ? Json(r) : Json(r, queried.error().to_string(r)));
        Json::Array values(r);
        if (queried) {
            for (const auto& entity : queried.value().data) {
                Json::Object row(r);
                row.emplace("id", Json(r, entity.id()));
                row.emplace("version", Json(r, entity.version()));
                row.emplace("code", Json(r, entity.code()));
                row.emplace("category_id", Json(r, entity.categoryId()));
                row.emplace("expression", Json(r, domain::E::status(entity).getCode().unwrap()));
                expression_read(row, r, "category", [&] {
                    return domain::E::status(entity).getCategory().getCode();
                });
#ifdef TEAQL_TOPOLOGY_SELF_PARENT
                expression_read(row, r, "parent", [&] {
                    return domain::E::status(entity).getCategory().getParent().getCode();
                });
#else
                row.emplace("parent_code", Json(r));
                row.emplace("parent_state", Json(r, "NotApplicable"));
#endif
                values.emplace_back(row);
            }
        }
        round.emplace("values", Json(r, values));
        round.emplace("after", snapshot(r, database));
        Json::Array logs(r);
        for (const auto& entry : user_context->sql_logs()) {
            Json::Object log(r);
            log.emplace("sql", Json(r, entry.sql));
            log.emplace("comment", entry.comment ? Json(r, *entry.comment) : Json(r));
            log.emplace("purpose", entry.purpose ? Json(r, *entry.purpose) : Json(r));
            log.emplace("audit_as", entry.audit_reason ? Json(r, *entry.audit_reason) : Json(r));
            Json::Array params(r);
            for (const auto& value : entry.params) {
                params.emplace_back(debug_string(r, value));
            }
            log.emplace("params", Json(r, params));
            logs.emplace_back(log);
        }
        round.emplace("logs", Json(r, logs));
        user_context.reset();
        round.emplace("after_close", snapshot(r, database));
        rounds.emplace_back(round);
    }
    Json::Object output(r);
    output.emplace("warmup_error", std::move(warmup_error));
    output.emplace("rounds", Json(r, rounds));
    output.emplace("hooks", Json(r, hooks));
    return Json(r, output);
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto input = Json::parse(global.memory_resource(), line);
            if (!input) {
                throw std::runtime_error("Invalid constant topology input");
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
