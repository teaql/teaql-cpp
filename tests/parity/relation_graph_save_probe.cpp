#include "../generator/parity/relation_identity_contracts/cpp/include/cpp_relation_identities.hpp"
#include "fixture_value.hpp"
#include <teaql/runtime/entity_runtime_debug.hpp>
#include <teaql/runtime/event_debug.hpp>
#include <teaql/runtime/mutation_policy_debug.hpp>
#include <teaql/runtime/log_model_debug.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <sqlite3.h>

using namespace cpp_relation_identities;

struct GraphSink final : SafeAuditEventSink {
    Vector<String>& events;
    bool fail;

    GraphSink(Vector<String>& events, bool fail) : events(events), fail(fail) {}

    Result<std::monostate, RuntimeError>
    on_safe_event(Memory r, const UserContext&, const SafeAuditEvent& event) const override {
        events.emplace_back(debug_string(r, event));
        return fail ? Result<std::monostate, RuntimeError>::err(
                          r, RuntimeError::event(r, "keyword audit failure"))
                    : Result<std::monostate, RuntimeError>::ok(r, {});
    }
};

struct GraphPolicy final : MutationPolicy {
    Vector<String>& plans;
    bool deny;

    GraphPolicy(Vector<String>& plans, bool deny) : plans(plans), deny(deny) {}

    MutationPolicyIdentity identity(Memory r) const override {
        return MutationPolicyIdentity(
            std::allocator_arg, Allocator(r), "keyword-graph", "1", "canonical");
    }

    MutationDecision review(Memory r, const UserContext&, const MutationPlan& plan) const override {
        plans.emplace_back(debug_string(r, plan));
        const std::array<String, 1> fields{String("name", r)};
        return deny ? MutationDecision::deny(r, "KEYWORD-DENIED", "keyword policy denial", fields)
                    : MutationDecision::allow(r);
    }
};

static void execute(sqlite3* database, const std::string& sql) {
    if (sqlite3_exec(database, sql.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
}

template <Entity T>
static Json native_graph(Memory r, const T& entity) {
    return std::move(detail::clone_in(r, entity)).into_json(r);
}

static Json graph_ledger(Memory r, const EntityRuntimeState& root) {
    Json::Array created(r), deleted(r);
    for (const auto& key : root.new_keys()) {
        created.emplace_back(debug_string(r, key));
    }
    for (const auto& key : root.deleted_keys()) {
        deleted.emplace_back(debug_string(r, key));
    }
    Json::Object output(r);
    output.emplace("changes", Json(r, debug_string(r, root.current_change_set())));
    output.emplace("new", Json(r, created));
    output.emplace("deleted", Json(r, deleted));
    return Json(r, output);
}

static Json database_snapshot(Memory r, sqlite3* database) {
    Json::Object output(r);
    for (std::string_view table : {"platform_data", "owner_data", "entry_data", "profile_data"}) {
        auto sql = "SELECT * FROM " + std::string(table) + " ORDER BY id";
        sqlite3_stmt* statement = nullptr;
        if (sqlite3_prepare_v2(database, sql.c_str(), -1, &statement, nullptr) != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(database));
        }
        std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> cleanup(statement,
                                                                           sqlite3_finalize);
        Json::Array rows(r);
        for (auto status = sqlite3_step(statement); status != SQLITE_DONE;
             status = sqlite3_step(statement)) {
            if (status != SQLITE_ROW) {
                throw std::runtime_error(sqlite3_errmsg(database));
            }
            Json::Object row(r);
            for (int index = 0; index < sqlite3_column_count(statement); ++index) {
                Json data(r);
                switch (sqlite3_column_type(statement, index)) {
                case SQLITE_NULL:
                    break;
                case SQLITE_INTEGER:
                    data = Json(r, std::int64_t(sqlite3_column_int64(statement, index)));
                    break;
                case SQLITE_TEXT:
                    data = Json(
                        r, reinterpret_cast<const char*>(sqlite3_column_text(statement, index)));
                    break;
                default:
                    throw std::runtime_error("unexpected fixture SQLite type");
                }
                row.emplace(String(sqlite3_column_name(statement, index), r), std::move(data));
            }
            rows.emplace_back(row);
        }
        output.emplace(String(table, r), Json(r, rows));
    }
    return Json(r, output);
}

template <Entity T, LedgerEntity Child>
static void compose_graph(T& root, Child& child, const Json& input) {
    auto composition = text(field(input, "composition"));
    if (composition == "attach") {
        child.attachRuntimeStateRecursive(*root.entity_runtime_state());
    } else if (composition != "none") {
        if (!root.includePendingMutationsFrom(child)) {
            throw std::runtime_error("Unexpected fixture ledger composition failure");
        }
    }
}

template <Entity T>
static Json finish_save(UserContext& user_context,
                        sqlite3* database,
                        T& entity,
                        const Json& input,
                        const Vector<String>& events,
                        const Vector<String>& plans) {
    auto r = user_context.memory_resource();
    auto root = *entity.entity_runtime_state();
    auto before = native_graph(r, entity);
    auto pending_before = graph_ledger(r, root);
    auto fault = text(field(input, "fault"));
    std::string table = "entry_data";
    int identity = fault.starts_with("second") ? 21 : 20;
    if (fault.starts_with("root")) {
        if (text(field(input, "target")) == "owner") {
            table = "owner_data";
            identity = 10;
        }
    }
    if (fault.ends_with("conflict")) {
        execute(database,
                "UPDATE " + table + " SET version=version+1 WHERE id=" + std::to_string(identity));
    }
    if (fault.ends_with("trigger")) {
        execute(database,
                "CREATE TRIGGER keyword_failure BEFORE UPDATE ON " + table +
                    " WHEN OLD.id=" + std::to_string(identity) +
                    " BEGIN SELECT RAISE(ABORT,'keyword write failure'); END;");
    }
    if (fault == "insert_trigger") {
        execute(
            database,
            "CREATE TRIGGER keyword_insert_failure BEFORE INSERT ON entry_data BEGIN SELECT RAISE(ABORT,'keyword insert failure'); END;");
    }
    auto db_before = database_snapshot(r, database);
    user_context.clear_sql_logs();
    Json saved(r), error(r);
    if (text(field(input, "op")) != "read") {
        auto& copy = user_context.make<T>(entity);
        const auto& result =
            AuditedSaveExt<T>::save(std::move(copy).audit_as("Save canonical keyword graph"),
                                    user_context)
                .wait();
        if (result) {
            saved = native_graph(r, result.value());
        } else {
            error = Json(r, result.error().to_string(r));
        }
    }
    Json::Array event_output(r), plan_output(r), logs(r);
    for (const auto& event : events) {
        event_output.emplace_back(event);
    }
    for (const auto& plan : plans) {
        plan_output.emplace_back(plan);
    }
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
        log.emplace("operation", Json(r, debug_string(r, entry.operation)));
        log.emplace("comment", entry.comment ? Json(r, *entry.comment) : Json(r));
        log.emplace("purpose", entry.purpose ? Json(r, *entry.purpose) : Json(r));
        log.emplace("audit_reason", entry.audit_reason ? Json(r, *entry.audit_reason) : Json(r));
        log.emplace("context", Json(r, debug_string(r, entry.log_context)));
        log.emplace("debug_sql", Json(r, entry.debug_sql));
        log.emplace("pretty_sql", Json(r, entry.pretty_sql));
        log.emplace("result_count",
                    entry.result_count ? Json(r, std::uint64_t(*entry.result_count)) : Json(r));
        log.emplace("result_type", entry.result_type ? Json(r, *entry.result_type) : Json(r));
        log.emplace("affected_rows", entry.affected_rows ? Json(r, *entry.affected_rows) : Json(r));
        log.emplace("result_summary", Json(r, entry.result_summary));
        log.emplace("trace", Json(r, trace));
        logs.emplace_back(log);
    }
    Json::Object output(r);
    output.emplace("before", std::move(before));
    output.emplace("pending_before", std::move(pending_before));
    output.emplace("saved", std::move(saved));
    output.emplace("error", std::move(error));
    output.emplace("db_before", std::move(db_before));
    output.emplace("db_after", database_snapshot(r, database));
    output.emplace("pending_after", graph_ledger(r, root));
    output.emplace("events", Json(r, event_output));
    output.emplace("plans", Json(r, plan_output));
    output.emplace("logs", Json(r, logs));
    return Json(r, output);
}

static Json observe_save(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto connection = user_context.make_resource_ref<SqliteConnection>();
    auto* database = user_context.get(connection).native_handle();
    std::ifstream file(std::string(TEAQL_SOURCE_DIR) + "/tests/support/relation_identity_sql.sql");
    std::ostringstream sql;
    sql << file.rdbuf();
    if (!file) {
        throw std::runtime_error("Missing keyword SQLite fixture");
    }
    execute(database, sql.str());
    require_value(serviceRuntimeFromConnection(user_context, connection));
    user_context.with_active_root("Platform", 1);
    user_context.set_trace_id("keyword-graph");
    user_context.set_user_identifier("fixture");
    auto& generator = user_context.make_resource<AtomicCounterIdGenerator>(1000);
    user_context.set_internal_id_generator(user_context, generator);
    auto& events = user_context.make<Vector<String>>();
    auto& sink = user_context.make_resource<GraphSink>(
        events, std::get<bool>(field(input, "audit_fail").data));
    user_context.set_custom_event_sink(user_context, sink);
    auto& plans = user_context.make<Vector<String>>();
    if (text(field(input, "policy")) != "none") {
        auto& policy =
            user_context.make_resource<GraphPolicy>(plans, text(field(input, "policy")) == "deny");
        auto registry = user_context.make_ref<FunctionMutationPolicyRegistry>(
            [&policy, r](Memory target, std::string_view) -> std::optional<MutationPolicyBorrow> {
                return MutationPolicyBorrow(
                    std::allocator_arg, Allocator(target), policy, detail::resource_arena(r));
            });
        user_context.set_mutation_policy_registry(registry);
    }
    auto Q = user_context.queries<query_factory>();
    auto sparse = std::get<bool>(field(input, "minimal").data);
    auto selection = text(field(input, "selection"));
    auto op = text(field(input, "op"));
    if (text(field(input, "target")) == "owner") {
        auto request = sparse ? Q.ownersMinimal() : Q.owners();
        if (selection == "first" || selection == "both" || selection == "minimal_child") {
            request.selectClassWith(selection == "minimal_child" ? Q.entriesMinimal()
                                                                 : Q.entries());
        }
        if (selection == "second" || selection == "both") {
            request.selectTemplate().selectOperator();
        }
        const auto& selected = require_value(std::move(request)
                                                 .withIdIs(10)
                                                 .comment("Load keyword graph")
                                                 .purpose("Review graph change boundary")
                                                 .executeForOne(user_context)
                                                 .wait());
        if (!selected) {
            throw std::runtime_error("Missing keyword Owner");
        }
        auto& entity = user_context.make<Owner>(*selected);
        if (op != "read") {
            entity.updateName("Updated owner");
        }
        if (op == "related" || op == "mixed" || op == "delete" || op == "foreign") {
            auto view = entity.modelClassHandle();
            if (view.value()) {
                auto& children = user_context.make<SmartList<Entry>>(*view.value());
                for (std::size_t index = 0; index < children.data.size(); ++index) {
                    auto& child = children.data[index];
                    if (op == "delete" || (op == "mixed" && index == 1)) {
                        child.markForDeletion();
                    } else if (op == "foreign") {
                        child.updateNamespaceId(11);
                    } else {
                        child.updateName("Updated class member");
                    }
                    compose_graph(entity, child, input);
                }
            }
            if (op == "related" || op == "mixed") {
                auto second = entity.modelTemplateHandle();
                if (second.value()) {
                    auto& children = user_context.make<SmartList<Entry>>(*second.value());
                    for (auto& child : children.data) {
                        child.updateName("Updated template member");
                        compose_graph(entity, child, input);
                    }
                }
                auto profile = entity.modelOperatorHandle();
                if (profile.value()) {
                    auto& child = user_context.make<Profile>(*profile.value());
                    child.updateName("Updated profile");
                    compose_graph(entity, child, input);
                }
            }
        }
        if (op == "create" || op == "mixed") {
            auto& child = Q.entries()
                              .comment("Construct keyword child")
                              .purpose("Extend reviewed aggregate")
                              .newEntity(user_context);
            child.updateName("New class member")
                .updateNamespaceId(10)
                .updateOperatorId(std::optional<std::uint64_t>(11));
            compose_graph(entity, child, input);
        }
        return finish_save(user_context, database, entity, input, events, plans);
    }
    auto request = sparse ? Q.entriesMinimal() : Q.entries();
    if (selection == "first" || selection == "both" || selection == "minimal_child") {
        request.selectNamespaceWith(selection == "minimal_child" ? Q.ownersMinimal() : Q.owners());
    }
    if (selection == "second" || selection == "both") {
        request.selectOperator();
    }
    const auto& selected = require_value(std::move(request)
                                             .withIdIs(20)
                                             .comment("Load keyword graph")
                                             .purpose("Review graph change boundary")
                                             .executeForOne(user_context)
                                             .wait());
    if (!selected) {
        throw std::runtime_error("Missing keyword Entry");
    }
    auto& entity = user_context.make<Entry>(*selected);
    if (op == "delete") {
        entity.markForDeletion();
    } else if (op != "read") {
        entity.updateName("Updated entry");
    }
    if (op == "related" || op == "mixed") {
        if (auto* value = entity.modelNamespace()) {
            auto& child = user_context.make<Owner>(*value);
            child.updateName("Updated namespace owner");
            compose_graph(entity, child, input);
        }
        if (auto* value = entity.modelOperator()) {
            auto& child = user_context.make<Owner>(*value);
            child.updateName("Updated operator owner");
            compose_graph(entity, child, input);
        }
    }
    if (op == "foreign") {
        entity.updateNamespaceId(11).updateOperatorId(std::optional<std::uint64_t>{});
    }
    if (op == "create" || op == "mixed") {
        auto& child = Q.owners()
                          .comment("Construct keyword owner")
                          .purpose("Extend reviewed aggregate")
                          .newEntity(user_context);
        child.updateName("New namespace owner");
        entity.updateNamespaceId(child.id());
        compose_graph(entity, child, input);
    }
    return finish_save(user_context, database, entity, input, events, plans);
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
                    throw std::runtime_error("Invalid keyword graph input");
                }
                std::cout
                    << observe_save(*user_context, *input).dump(user_context->memory_resource())
                    << '\n';
            }
            user_context.reset();
            if (global.live_user_bytes() != 0) {
                throw std::runtime_error("User Context storage leaked");
            }
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
