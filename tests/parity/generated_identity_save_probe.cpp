#include <cpp_privacy_contracts.hpp>
#include "fixture_value.hpp"
#include <teaql/runtime/error_debug.hpp>
#include <teaql/runtime/context_model_debug.hpp>
#include <teaql/runtime/event_debug.hpp>
#include <teaql/runtime/log_model_debug.hpp>
#include <teaql/runtime/mutation_policy_debug.hpp>
#include <iostream>
#include <sqlite3.h>

using namespace teaql;
namespace domain = cpp_privacy_contracts;

static constexpr std::string_view schema = R"sql(
CREATE TABLE platform_data(id INTEGER PRIMARY KEY,version INTEGER,name TEXT);
INSERT INTO platform_data VALUES(100,1,'platform');
CREATE TABLE business_items(id INTEGER PRIMARY KEY,version INTEGER,platform INTEGER,display_name TEXT,note TEXT);
INSERT INTO business_items VALUES(11,1,100,'PRIVATE-OLD-SENTENCE','PUBLIC-OLD-NOTE');
CREATE TABLE other_table(id INTEGER PRIMARY KEY,version INTEGER,platform INTEGER,display_name TEXT,note TEXT);
INSERT INTO other_table VALUES(22,1,100,'PRIVATE-OLD-SENTENCE','PUBLIC-OLD-NOTE');
CREATE TABLE business_children(id INTEGER PRIMARY KEY,version INTEGER,platform INTEGER,display_name TEXT,note TEXT,password TEXT);
)sql";

class Ids final : public InternalIdGenerator {
public:
    using allocator_type = Allocator;
    bool fail;
    mutable Vector<String> calls;

    Ids(std::allocator_arg_t, Allocator a, bool failure) : fail(failure), calls(a.resource()) {}

    Result<std::uint64_t, RuntimeError> generate_id(Memory r,
                                                    std::string_view entity) const override {
        calls.emplace_back(entity);
        if (fail) {
            return Result<std::uint64_t, RuntimeError>::err(
                r, RuntimeError::id_generation(r, "fixture ID failure"));
        }
        return Result<std::uint64_t, RuntimeError>::ok(r, 1001);
    }
};

struct Policy final : MutationPolicy {
    Vector<String>& plans;
    bool deny;

    Policy(Vector<String>& values, bool denial) : plans(values), deny(denial) {}

    MutationPolicyIdentity identity(Memory r) const override {
        return MutationPolicyIdentity(
            std::allocator_arg, Allocator(r), "identity-policy", "1", "identity-fingerprint");
    }

    MutationDecision review(Memory r, const UserContext&, const MutationPlan& plan) const override {
        plans.emplace_back(debug_string(r, plan));
        if (deny) {
            const std::array<String, 1> fields{String("name", r)};
            return MutationDecision::deny(r, "IDENTITY-DENIED", "deny protected identity", fields);
        }
        return MutationDecision::allow(r);
    }
};

struct Sink final : SafeAuditEventSink {
    Vector<String>& events;
    bool fail;

    Sink(Vector<String>& values, bool failure) : events(values), fail(failure) {}

    Result<std::monostate, RuntimeError>
    on_safe_event(Memory r, const UserContext&, const SafeAuditEvent& event) const override {
        events.emplace_back(debug_string(r, event));
        if (fail) {
            return Result<std::monostate, RuntimeError>::err(
                r, RuntimeError::event(r, "fixture audit failure"));
        }
        return Result<std::monostate, RuntimeError>::ok(r, {});
    }
};

static void execute(sqlite3* database, std::string_view sql) {
    if (sqlite3_exec(database, std::string(sql).c_str(), nullptr, nullptr, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
}

static Json snapshot(Memory r, sqlite3* database) {
    Json::Object output(r);
    for (std::string_view table :
         {"platform_data", "business_items", "other_table", "business_children"}) {
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
            for (int index = 0; index != sqlite3_column_count(statement); ++index) {
                Json value(r);
                switch (sqlite3_column_type(statement, index)) {
                case SQLITE_NULL:
                    break;
                case SQLITE_INTEGER:
                    value = Json(r, std::int64_t(sqlite3_column_int64(statement, index)));
                    break;
                case SQLITE_TEXT:
                    value = Json(
                        r, reinterpret_cast<const char*>(sqlite3_column_text(statement, index)));
                    break;
                default:
                    throw std::runtime_error("Unexpected fixture database value");
                }
                row.emplace(String(sqlite3_column_name(statement, index), r), std::move(value));
            }
            rows.emplace_back(Json(r, std::move(row)));
        }
        output.emplace(String(table, r), Json(r, std::move(rows)));
    }
    return Json(r, std::move(output));
}

template <class T>
static Json optional_json(Memory r, const std::optional<T>& value) {
    return value ? Json(r, *value) : Json(r);
}

static Json strings_json(Memory r, const Vector<String>& values) {
    Json::Array array(r);
    for (const auto& value : values) {
        array.emplace_back(Json(r, value));
    }
    return Json(r, std::move(array));
}

static String sequence_debug(Memory r, const auto& values) {
    String output(r);
    DebugWriter writer(output, {});
    writer.sequence(values, DefaultDebugElement{});
    return output;
}

static Json logs(const UserContext& user_context) {
    auto r = user_context.memory_resource();
    Json::Array output(r);
    for (const auto& entry : user_context.sql_logs()) {
        auto row = detail::model_json(r);
        detail::model_put(row, "log_context", debug_string(r, entry.log_context));
        detail::model_put(row, "operation", debug_string(r, entry.operation));
        detail::model_put(row, "comment", optional_json(r, entry.comment));
        detail::model_put(row, "purpose", optional_json(r, entry.purpose));
        detail::model_put(row, "audit_reason", optional_json(r, entry.audit_reason));
        detail::model_put(row, "trace_path", sequence_debug(r, entry.trace_path));
        detail::model_put(row, "sql", entry.sql);
        Json::Array params(r);
        for (const auto& value : entry.params) {
            params.emplace_back(value.to_json_value(r));
        }
        detail::model_put(row, "params", Json(r, std::move(params)));
        detail::model_put(row, "params_debug", sequence_debug(r, entry.params));
        detail::model_put(row, "debug_sql", entry.debug_sql);
        detail::model_put(row, "pretty_sql", entry.pretty_sql);
        detail::model_put(row, "result_count", optional_json(r, entry.result_count));
        detail::model_put(row, "result_type", optional_json(r, entry.result_type));
        detail::model_put(row, "affected_rows", optional_json(r, entry.affected_rows));
        detail::model_put(row, "result_summary", entry.result_summary);
        output.emplace_back(std::move(row));
    }
    return Json(r, std::move(output));
}

static Json failure(Memory r, const RuntimeError& error) {
    auto output = detail::model_json(r);
    detail::model_put(output, "ok", false);
    detail::model_put(output, "error", error.to_string(r));
    detail::model_put(output, "debug", debug_string(r, error));
    return output;
}

template <Entity T>
static Json save(UserContext& user_context, T& entity) {
    auto r = user_context.memory_resource();
    auto& result = domain::AuditedSaveExt<T>::save(
                       std::move(entity).audit_as("save PRIVATE-NEW-SENTENCE"), user_context)
                       .wait();
    if (!result) {
        return failure(r, result.error());
    }
    auto output = detail::model_json(r);
    detail::model_put(output, "ok", true);
    detail::model_put(output, "entity", detail::clone_in(r, result.value()).into_json(r));
    return output;
}

static Json observe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto connection = user_context.make_resource_ref<SqliteConnection>(":memory:");
    auto* database = user_context.get(connection).native_handle();
    execute(database, schema);
    auto fail = text(field(input, "failure"));
    if (fail == "provider") {
        for (std::string_view table : {"business_items", "other_table", "business_children"}) {
            for (std::string_view operation : {"INSERT", "UPDATE"}) {
                execute(database,
                        "CREATE TRIGGER fail_" + std::string(table) + "_" + std::string(operation) +
                            " BEFORE " + std::string(operation) + " ON " + std::string(table) +
                            " BEGIN SELECT RAISE(ABORT,'fixture provider failure'); END");
            }
        }
    }
    auto assembled = domain::serviceRuntimeFromConnection(user_context, connection);
    if (!assembled) {
        throw std::runtime_error("Fixture runtime assembly failed");
    }
    user_context.with_active_root("Platform", 100);
    user_context.set_trace_id("identity");
    user_context.set_user_identifier("identity");
    bool logging = std::get<bool>(field(input, "logging").data);
    user_context.set_sql_log_options(SqlLogOptions{.select = logging, .mutation = logging});
    auto& ids = user_context.make<Ids>(fail == "id");
    user_context.set_internal_id_generator(user_context, ids);
    auto& plans = user_context.make<Vector<String>>();
    auto& events = user_context.make<Vector<String>>();
    auto policy_mode = text(field(input, "policy"));
    if (policy_mode != "none") {
        auto& policy = user_context.make_resource<Policy>(plans, policy_mode == "deny");
        auto registry = user_context.make_ref<FunctionMutationPolicyRegistry>(
            [&policy, r](Memory destination,
                         std::string_view) -> std::optional<MutationPolicyBorrow> {
                return MutationPolicyBorrow(
                    std::allocator_arg, Allocator(destination), policy, detail::resource_arena(r));
            });
        user_context.set_mutation_policy_registry(registry);
    }
    auto& sink = user_context.make_resource<Sink>(events, fail == "audit");
    user_context.set_custom_event_sink(user_context, sink);
    auto Q = user_context.queries<domain::query_factory>();
    Json outcome(r);
    auto initialize = [r](auto& entity) {
        entity.updateName("PRIVATE-NEW-SENTENCE");
        entity.updateNote(std::optional<String>(std::in_place, "PUBLIC-NEW-LONG-NOTE", r));
    };
    auto operation = text(field(input, "operation"));
    if (operation == "create_child") {
        auto& entity = Q.children()
                           .comment("prepare protected creation")
                           .purpose("qualify generated identity save")
                           .newEntity(user_context);
        initialize(entity);
        entity.updateSecret("CREDENTIAL-SENTINEL");
        outcome = save(user_context, entity);
    } else if (operation == "create") {
        auto& entity = Q.items()
                           .comment("prepare protected creation")
                           .purpose("qualify generated identity save")
                           .newEntity(user_context);
        initialize(entity);
        outcome = save(user_context, entity);
    } else {
        auto& result = Q.items()
                           .selectSelfFields()
                           .orderByIdAsc()
                           .comment("load protected identity")
                           .purpose("qualify generated identity save")
                           .executeForOne(user_context)
                           .wait();
        if (!result) {
            using Error = std::remove_cvref_t<decltype(result.error())>;
            if (result.error().kind() != Error::Kind::Runtime) {
                throw std::runtime_error("Expected the generated runtime error boundary");
            }
            outcome = failure(r, result.error().runtime_error());
        } else {
            if (!result.value()) {
                throw std::runtime_error("Missing fixture entity");
            }
            auto& entity = user_context.make<domain::Item>(std::move(*result.value()));
            entity.updateName("PRIVATE-NEW-SENTENCE");
            entity.updateNote(std::optional<String>(std::in_place, "PUBLIC-NEW-LONG-NOTE", r));
            outcome = save(user_context, entity);
        }
    }
    auto output = detail::model_json(r);
    detail::model_put(output, "outcome", std::move(outcome));
    detail::model_put(output, "calls", strings_json(r, ids.calls));
    detail::model_put(output, "plans", strings_json(r, plans));
    detail::model_put(output, "events", strings_json(r, events));
    detail::model_put(output, "logs", logs(user_context));
    detail::model_put(output, "fixes", sequence_debug(r, user_context.last_fix_evidence()));
    detail::model_put(output, "database", snapshot(r, database));
    return output;
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto owner = global.make_user_context();
        auto& user_context = *owner;
        {
            auto parsed = Json::parse(user_context.memory_resource(), line);
            if (!parsed) {
                throw std::runtime_error("Invalid fixture input");
            }
            std::cout << observe(user_context, *parsed).dump(user_context.memory_resource())
                      << '\n';
        }
        owner.reset();
        if (global.live_user_bytes() != 0) {
            throw std::runtime_error("Generated Save retains UserContext memory");
        }
    }
}
