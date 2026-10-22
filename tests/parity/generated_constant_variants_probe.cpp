#include "../generated_constants_numeric/include/cpp_constants_numeric.hpp"
#include "../generated_constants_catalog/include/cpp_constants_catalog.hpp"
#include "fixture_value.hpp"
#include <teaql/runtime/event_debug.hpp>
#include <iostream>
#include <sqlite3.h>

using namespace teaql;

static void fixture_sql(sqlite3* database, const std::string& statement) {
    if (sqlite3_exec(database, statement.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
}

static Json table(Memory r, sqlite3* database, std::string_view name) {
    std::string query = "SELECT * FROM " + std::string(name) + " ORDER BY id";
    sqlite3_stmt* raw = nullptr;
    if (sqlite3_prepare_v2(database, query.c_str(), -1, &raw, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
    std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> statement(raw, sqlite3_finalize);
    Json::Array rows(r);
    int status = SQLITE_OK;
    while ((status = sqlite3_step(statement.get())) == SQLITE_ROW) {
        Json::Object row(r);
        for (int index = 0; index < sqlite3_column_count(statement.get()); ++index) {
            Json value(r);
            switch (sqlite3_column_type(statement.get(), index)) {
            case SQLITE_NULL:
                break;
            case SQLITE_INTEGER:
                value = Json(
                    r, static_cast<std::int64_t>(sqlite3_column_int64(statement.get(), index)));
                break;
            case SQLITE_FLOAT:
                value = Json(r, sqlite3_column_double(statement.get(), index));
                break;
            default:
                value = Json(r,
                             std::string_view(reinterpret_cast<const char*>(
                                 sqlite3_column_text(statement.get(), index))));
                break;
            }
            row.emplace(String(sqlite3_column_name(statement.get(), index), r), std::move(value));
        }
        rows.emplace_back(row);
    }
    if (status != SQLITE_DONE) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
    return Json(r, rows);
}

static Json database_view(Memory r, sqlite3* database, bool scalar) {
    Json::Object out(r);
    if (scalar) {
        for (auto name : {"platform_data", "priority_data", "label_data"}) {
            out.emplace(String(name, r), table(r, database, name));
        }
    } else {
        out.emplace(String("category_data", r), table(r, database, "category_data"));
        out.emplace(String("platform_data", r), table(r, database, "platform_data"));
    }
    sqlite3_stmt* raw = nullptr;
    if (sqlite3_prepare_v2(
            database,
            "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='teaql_id_space'",
            -1,
            &raw,
            nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
    std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> exists(raw, sqlite3_finalize);
    if (sqlite3_step(exists.get()) != SQLITE_ROW) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
    Json::Array levels(r);
    if (sqlite3_column_int64(exists.get(), 0)) {
        sqlite3_stmt* ids_raw = nullptr;
        if (sqlite3_prepare_v2(
                database,
                "SELECT type_name,current_level FROM teaql_id_space ORDER BY type_name",
                -1,
                &ids_raw,
                nullptr) != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(database));
        }
        std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> ids(ids_raw, sqlite3_finalize);
        int status = SQLITE_OK;
        while ((status = sqlite3_step(ids.get())) == SQLITE_ROW) {
            Json::Array row(r);
            row.emplace_back(
                std::string_view(reinterpret_cast<const char*>(sqlite3_column_text(ids.get(), 0))));
            row.emplace_back(static_cast<std::int64_t>(sqlite3_column_int64(ids.get(), 1)));
            levels.emplace_back(row);
        }
        if (status != SQLITE_DONE) {
            throw std::runtime_error(sqlite3_errmsg(database));
        }
    }
    out.emplace(String("ids", r), Json(r, levels));
    return Json(r, out);
}

struct BootstrapSink final : SafeAuditEventSink {
    Json::Array& events;
    bool fail;

    BootstrapSink(Json::Array& events, bool fail) : events(events), fail(fail) {}

    Result<std::monostate, RuntimeError>
    on_safe_event(Memory r, const UserContext&, const SafeAuditEvent& event) const override {
        Json::Object out(r);
        out.emplace("kind", Json(r, debug_string(r, event.kind)));
        out.emplace("entity", Json(r, event.entity));
        out.emplace("id", event.entity_id ? Json(r, *event.entity_id) : Json(r));
        Json::Array fields(r);
        for (const auto& field : event.fields) {
            Json::Object item(r);
            item.emplace("name", Json(r, field.name));
            item.emplace("value", field.value ? Json(r, *field.value) : Json(r));
            item.emplace("masked", Json(r, field.masked));
            item.emplace("truncated", Json(r, field.truncated));
            item.emplace("raw_length",
                         field.raw_length ? Json(r, std::uint64_t(*field.raw_length)) : Json(r));
            item.emplace("output_length",
                         field.output_length ? Json(r, std::uint64_t(*field.output_length))
                                             : Json(r));
            item.emplace("mask_reason", field.mask_reason ? Json(r, *field.mask_reason) : Json(r));
            item.emplace("truncate_reason",
                         field.truncate_reason ? Json(r, *field.truncate_reason) : Json(r));
            fields.emplace_back(item);
        }
        out.emplace("fields", Json(r, fields));
        String trace(r);
        DebugWriter trace_writer(trace, {});
        detail::write_policy_list(trace_writer, event.trace_chain);
        out.emplace("trace", Json(r, trace));
        out.emplace("governance",
                    event.mutation_governance ? Json(r, debug_string(r, *event.mutation_governance))
                                              : Json(r));
        Json bootstrap(r);
        if (event.bootstrap_audit) {
            const auto& identity = *event.bootstrap_audit;
            Json::Object item(r);
            item.emplace("actor", Json(r, identity.actor));
            item.emplace("category", Json(r, identity.category));
            item.emplace("reason", Json(r, identity.reason));
            item.emplace("version",
                         identity.resulting_version ? Json(r, *identity.resulting_version)
                                                    : Json(r));
            bootstrap = Json(r, item);
        }
        out.emplace("bootstrap", std::move(bootstrap));
        events.emplace_back(out);
        using Output = Result<std::monostate, RuntimeError>;
        return fail && (event.kind == RawAuditEventKind::Created ||
                        event.kind == RawAuditEventKind::Updated)
                   ? Output::err(r, RuntimeError::event(r, "fixture bootstrap audit failure"))
                   : Output::ok(r, {});
    }
};

struct BootstrapPolicy final : MutationPolicy {
    Vector<String>& plans;
    String mode, target;
    using allocator_type = Allocator;

    BootstrapPolicy(std::allocator_arg_t,
                    Allocator a,
                    Vector<String>& plans,
                    std::string_view mode,
                    std::string_view target)
        : plans(plans), mode(mode, a.resource()), target(target, a.resource()) {}

    MutationPolicyIdentity identity(Memory r) const override {
        return MutationPolicyIdentity(
            std::allocator_arg, Allocator(r), "bootstrap-policy", "1", "fingerprint");
    }

    MutationDecision review(Memory r, const UserContext&, const MutationPlan& plan) const override {
        plans.emplace_back(debug_string(r, plan));
        return mode == "deny" && plan.root_entity_type == target
                   ? MutationDecision::deny(
                         r, "BOOTSTRAP-DENIED", "fixture bootstrap policy denial", {})
                   : MutationDecision::allow(r);
    }
};

static Json observe(GlobalContext& global, const Json& input, Memory r) {
    bool scalar = text(field(input, "fixture")) == "numeric";
    auto& connection = global.make_resource<SqliteConnection>();
    auto& transport = global.make<SqliteMutationExecutor>(connection);
    auto& ids = global.make<SqliteIdSpaceGenerator>(transport);
    auto create = [&](UserContext& user_context) {
        auto selected = optional(input, "module");
        auto mode = selected ? text(*selected) : "combined";
        auto& module =
            scalar              ? mode == "plain"     ? cpp_constants_numeric::module(user_context)
                                  : mode == "checker" ? cpp_constants_numeric::moduleWithCheckers(user_context)
                                  : mode == "behavior"
                                      ? cpp_constants_numeric::moduleWithBehaviors(user_context)
                                      : cpp_constants_numeric::moduleWithBehaviorsAndCheckers(user_context)
                         : mode == "plain" ? cpp_constants_catalog::module(user_context)
            : mode == "checker" ? cpp_constants_catalog::moduleWithCheckers(user_context)
            : mode == "behavior"
                ? cpp_constants_catalog::moduleWithBehaviors(user_context)
                : cpp_constants_catalog::moduleWithBehaviorsAndCheckers(user_context);
        module.apply_to(user_context);
        use_sqlite_provider(user_context, global, transport);
        user_context.set_internal_id_generator(global, ids);
        auto dialect = user_context.require_resource<SqliteDialect>();
        auto& state = user_context.make<SqlExecutorState>();
        static_assert(!std::same_as<cpp_constants_numeric::DataServiceExecutor,
                                    cpp_constants_catalog::DataServiceExecutor>);
        auto register_domain = [&]<class Executor>() {
            auto executor = user_context.make_ref<Executor>(
                dialect.value().get(), transport, module.metadata, state);
            user_context.register_executor(executor);
        };
        if (scalar) {
            register_domain.template operator()<cpp_constants_numeric::DataServiceExecutor>();
        } else {
            register_domain.template operator()<cpp_constants_catalog::DataServiceExecutor>();
        }
    };
    auto warmup = global.make_user_context();
    create(*warmup);
    const auto& initial = warmup->ensure_schema().wait();
    if (!initial) {
        Json::Object out(r);
        out.emplace(String("warmup_error", r), Json(r, initial.error().to_string(r)));
        return Json(r, out);
    }
    warmup.reset();
    auto* database = connection.native_handle();
    auto state = text(field(input, "state"));
    if (scalar) {
        if (state == "empty") {
            fixture_sql(
                database,
                "DELETE FROM label_data;DELETE FROM priority_data;DELETE FROM platform_data;");
        } else if (state == "corrupt") {
            fixture_sql(
                database,
                "UPDATE priority_data SET rank=55,weight=8.5,ratio=0.25,cost='9.00',day='2001-01-01',note='stale',optional_rank=NULL WHERE id=1;");
        } else if (state == "missing") {
            fixture_sql(
                database,
                "DELETE FROM label_data WHERE id=32;DELETE FROM priority_data WHERE id=22;");
        } else if (state == "deleted") {
            fixture_sql(database, "UPDATE priority_data SET version=-1 WHERE id=1;");
        } else if (state == "extra") {
            fixture_sql(
                database,
                "INSERT INTO label_data(id,code,platform,priority,version) VALUES(99,'CUSTOM',1,1,7);");
        }
    } else {
        if (state == "empty") {
            fixture_sql(database, "DELETE FROM category_data;DELETE FROM platform_data;");
        } else if (state == "corrupt") {
            fixture_sql(database, "UPDATE category_data SET name='stale',code='OLD' WHERE id=1;");
        } else if (state == "missing") {
            fixture_sql(database, "DELETE FROM category_data WHERE id=22;");
        } else if (state == "deleted") {
            fixture_sql(database, "UPDATE category_data SET version=-1 WHERE id=22;");
        } else if (state == "extra") {
            fixture_sql(
                database,
                "INSERT INTO category_data(id,name,code,version) VALUES(99,'Custom','CUSTOM',7);");
        }
    }
    fixture_sql(database, "DROP TABLE teaql_id_space;");
    auto user_context = global.make_user_context();
    create(*user_context);
    const auto* governance = optional(input, "governance");
    auto rc = user_context->memory_resource();
    auto& events = user_context->make<Json::Array>();
    auto& plans = user_context->make<Vector<String>>();
    if (governance) {
        user_context->set_trace_id("bootstrap");
        user_context->set_user_identifier("fixture");
        auto& sink = user_context->make_resource<BootstrapSink>(
            events, std::get<bool>(field(input, "audit_fail").data));
        user_context->set_custom_event_sink(*user_context, sink);
        if (text(*governance) != "none") {
            auto mode = text(*governance);
            auto target = mode == "deny_root" ? "Platform" : scalar ? "Priority" : "Category";
            auto& policy = user_context->make<BootstrapPolicy>(
                plans, mode == "deny_root" || mode == "deny_constant" ? "deny" : "allow", target);
            auto registry = user_context->make_ref<FunctionMutationPolicyRegistry>(
                [&policy, rc](Memory target,
                              std::string_view) -> std::optional<MutationPolicyBorrow> {
                    return MutationPolicyBorrow(
                        std::allocator_arg, Allocator(target), policy, detail::resource_arena(rc));
                });
            user_context->set_mutation_policy_registry(registry);
            if (mode == "approved") {
                auto approval = user_context->make_ref<FunctionMutationPolicyApprovalProvider>(
                    [](Memory target, const MutationPolicyIdentity& identity)
                        -> std::optional<MutationPolicyApproval> {
                        return MutationPolicyApproval(std::allocator_arg,
                                                      Allocator(target),
                                                      identity,
                                                      "reviewer",
                                                      SystemTime::from_unix_seconds(0));
                    });
                user_context->set_mutation_policy_approval_provider(approval);
            }
        }
    }
    Json::Array rounds(r);
    auto count = std::get<std::uint64_t>(field(input, "rounds").data);
    for (std::uint64_t index = 0; index < count; ++index) {
        auto before = database_view(r, database, scalar);
        const auto& result = user_context->ensure_schema().wait();
        Json::Object round(r);
        round.emplace(String("before", r), std::move(before));
        round.emplace(String("after", r), database_view(r, database, scalar));
        round.emplace(String("error", r), result ? Json(r) : Json(r, result.error().to_string(r)));
        auto root = user_context->require_active_root("Platform");
        round.emplace(String("root", r), root ? Json(r, root.value().get().id) : Json(r));
        rounds.emplace_back(round);
    }
    Json::Object out(r);
    out.emplace(String("rounds", r), Json(r, rounds));
    if (governance) {
        out.emplace("events", Json(r, events));
        Json::Array reviewed(r);
        for (const auto& plan : plans) {
            reviewed.emplace_back(plan);
        }
        out.emplace("plans", Json(r, reviewed));
    }
    return Json(r, out);
}

int main() {
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto fixture_context = global.make_user_context();
            auto input = Json::parse(fixture_context->memory_resource(), line);
            if (!input) {
                throw std::runtime_error("Invalid constant variant fixture");
            }
            std::cout << observe(global, *input, fixture_context->memory_resource())
                             .dump(fixture_context->memory_resource())
                      << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
