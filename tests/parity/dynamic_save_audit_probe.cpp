#include "../generated_dynamic_json/include/cpp_dynamic_json.hpp"
#include <teaql/runtime/event_debug.hpp>
#include <teaql/runtime/mutation_policy_debug.hpp>
#include "fixture_value.hpp"
#include <sqlite3.h>
#include <fstream>
#include <sstream>
#include <iostream>
using namespace cpp_dynamic_json;

struct Sink final : SafeAuditEventSink {
    Vector<String>& events;
    bool fail;

    Sink(Vector<String>& events, bool fail) : events(events), fail(fail) {}

    Result<std::monostate, RuntimeError>
    on_safe_event(Memory r, const UserContext&, const SafeAuditEvent& event) const override {
        events.emplace_back(debug_string(r, event));
        return fail ? Result<std::monostate, RuntimeError>::err(
                          r, RuntimeError::event(r, "fixture audit failure"))
                    : Result<std::monostate, RuntimeError>::ok(r, {});
    }
};

struct FixturePolicy final : MutationPolicy {
    Vector<String>& plans;
    bool deny;

    FixturePolicy(Vector<String>& plans, bool deny) : plans(plans), deny(deny) {}

    MutationPolicyIdentity identity(Memory r) const override {
        return MutationPolicyIdentity(
            std::allocator_arg, Allocator(r), "dynamic-policy", "1", "fingerprint");
    }

    MutationDecision review(Memory r, const UserContext&, const MutationPlan& plan) const override {
        plans.emplace_back(debug_string(r, plan));
        const std::array<String, 1> fields{String("#note", r)};
        return deny ? MutationDecision::deny(r, "DYNAMIC-DENIED", "fixture policy denial", fields)
                    : MutationDecision::allow(r);
    }
};

static void exec(sqlite3* db, const std::string& sql) {
    if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(db));
    }
}

static Json observe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto ref = configureRuntime(user_context);
    auto& executor = user_context.get(ref);
    auto* db = executor.transport.connection().native_handle();
    for (const char* name : {"dynamic_json_schema.sql", "dynamic_json_dataset.sql"}) {
        std::ifstream file(std::string(TEAQL_SOURCE_DIR) + "/tests/support/" + name);
        std::ostringstream sql;
        sql << file.rdbuf();
        if (!file) {
            throw std::runtime_error("fixture missing");
        }
        exec(db, sql.str());
    }
    user_context.with_active_root("Platform", 1);
    user_context.set_trace_id("fixture");
    user_context.set_user_identifier("fixture");
    const auto* type = optional(input, "type");
    const auto declared = type ? DataType(std::get<std::uint64_t>(type->data)) : DataType::Text;
    const std::array fields{DynamicFieldDefinitions::Field{"note", declared}};
    auto& definitions = user_context.make<DynamicFieldDefinitions>(
        DynamicFieldDefinitions::from_fields(r, "Product", "v1", fields).value());
    const std::array defs{&definitions};
    auto& provider =
        user_context.make<DatabaseDynamicFieldsProvider<cpp_dynamic_json::DataServiceExecutor>>(
            DatabaseDynamicFieldsProvider<cpp_dynamic_json::DataServiceExecutor>::create(
                r, "profile", defs)
                .value());
    user_context.set_dynamic_fields_provider(user_context, provider);
    require_value(provider.ensure_schema(user_context).wait());
    if (const auto* stored = optional(input, "stored")) {
        DynamicFieldWrite::Changes changes(r);
        changes.emplace(String("note", r), DynamicFieldMutation::set(r, value(r, *stored)));
        Vector<DynamicFieldWrite> writes(r);
        writes.emplace_back("profile", definitions, 7, changes);
        auto reason = MutationIntent::create(r, "seed typed durable extension");
        auto& tx = executor.transport.begin_sql(r).wait().value().get();
        require_value(tx.dynamic_field_store()->apply_writes(r, writes, reason.value()).wait());
        require_value(tx.commit_sql(r).wait());
    } else {
        exec(
            db,
            R"sql(INSERT INTO teaql_dynamic_field_storage_v1 VALUES('profile','Product','7','note','v1','Text',0,'"PRIVATE-OLD"'))sql");
    }
    auto Q = user_context.queries<query_factory>();
    auto request = Q.products()
                       .withIdIs(7)
                       .selectSelfFields()
                       .selectDynamicFields(DynamicFieldSelection::all(r))
                       .comment("load PRIVATE-OLD")
                       .purpose("edit durable extensions");
    const auto& loaded = require_value(request.executeForOne(user_context).wait());
    if (!loaded) {
        throw std::runtime_error("missing fixture product");
    }
    auto& entity = user_context.make<Product>(*loaded);
    auto op = text(field(input, "op"));
    auto changed = op == "delete" ? entity.deleteDynamicField("note")
                                  : entity.updateDynamicField("note",
                                                              op == "null" ? Value(r)
                                                              : optional(input, "value")
                                                                  ? value(r, field(input, "value"))
                                                                  : Value(r, "PRIVATE-NEW"));
    require_value(changed);
    if (std::get<bool>(field(input, "trigger").data)) {
        exec(
            db,
            "CREATE TRIGGER deny_extension BEFORE INSERT ON teaql_dynamic_field_storage_v1 BEGIN SELECT RAISE(ABORT,'fixture failure'); END");
    }
    if (std::get<bool>(field(input, "conflict").data)) {
        exec(db, "UPDATE product_data SET version=3 WHERE id=7");
    }
    auto& events = user_context.make<Vector<String>>();
    auto& sink =
        user_context.make_resource<Sink>(events, std::get<bool>(field(input, "audit_fail").data));
    user_context.set_custom_event_sink(user_context, sink);
    auto& plans = user_context.make<Vector<String>>();
    const auto* mode = optional(input, "policy");
    if (mode && text(*mode) != "none") {
        auto& policy = user_context.make_resource<FixturePolicy>(plans, text(*mode) == "deny");
        auto registry = user_context.make_ref<FunctionMutationPolicyRegistry>(
            [&policy, r](Memory target, std::string_view) -> std::optional<MutationPolicyBorrow> {
                return MutationPolicyBorrow(
                    std::allocator_arg, Allocator(target), policy, detail::resource_arena(r));
            });
        user_context.set_mutation_policy_registry(registry);
        if (text(*mode) == "approved") {
            auto approval = user_context.make_ref<FunctionMutationPolicyApprovalProvider>(
                [](Memory target, const MutationPolicyIdentity& identity)
                    -> std::optional<MutationPolicyApproval> {
                    return MutationPolicyApproval(std::allocator_arg,
                                                  Allocator(target),
                                                  identity,
                                                  "reviewer",
                                                  SystemTime::from_unix_seconds(0));
                });
            user_context.set_mutation_policy_approval_provider(approval);
        }
    }
    user_context.clear_sql_logs();
    auto& saved = AuditedSaveExt<Product>::save(
                      Audited<Product>(
                          std::allocator_arg, Allocator(r), entity, "save PRIVATE-OLD PRIVATE-NEW"),
                      user_context)
                      .wait();
    Json::Object out(r);
    out.emplace("error", saved ? Json(r) : Json(r, saved.error().to_string(r)));
    Json::Array event_output(r), logs(r);
    for (const auto& event : events) {
        event_output.emplace_back(event);
    }
    for (const auto& log : user_context.sql_logs()) {
        Json::Object row(r);
        row.emplace("sql", Json(r, log.sql));
        Json::Array params(r);
        for (const auto& v : log.params) {
            params.emplace_back(v.to_json_value(r));
        }
        row.emplace("params", Json(r, params));
        row.emplace("comment", log.comment ? Json(r, *log.comment) : Json(r));
        row.emplace("purpose", log.purpose ? Json(r, *log.purpose) : Json(r));
        logs.emplace_back(row);
    }
    out.emplace("events", Json(r, event_output));
    out.emplace("logs", Json(r, logs));
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(
            db, "SELECT version FROM product_data WHERE id=7", -1, &statement, nullptr) !=
        SQLITE_OK) {
        throw std::runtime_error("version inspection failed");
    }
    if (sqlite3_step(statement) != SQLITE_ROW) {
        throw std::runtime_error("version row missing");
    }
    out.emplace("version", Json(r, std::int64_t(sqlite3_column_int64(statement, 0))));
    sqlite3_finalize(statement);
    statement = nullptr;
    if (sqlite3_prepare_v2(
            db,
            "SELECT is_null,payload FROM teaql_dynamic_field_storage_v1 WHERE owner_id='7' AND code='note'",
            -1,
            &statement,
            nullptr) != SQLITE_OK) {
        throw std::runtime_error("storage inspection failed");
    }
    Json storage(r);
    if (sqlite3_step(statement) == SQLITE_ROW) {
        Json::Object row(r);
        row.emplace("is_null", Json(r, std::int64_t(sqlite3_column_int64(statement, 0))));
        auto* payload = sqlite3_column_text(statement, 1);
        row.emplace("payload",
                    payload ? Json(r, std::string_view(reinterpret_cast<const char*>(payload)))
                            : Json(r));
        storage = Json(r, row);
    }
    sqlite3_finalize(statement);
    out.emplace("storage", std::move(storage));
    out.emplace("pending", Json(r, entity.hasPendingDynamicMutations()));
    if (mode) {
        Json::Array reviewed(r);
        for (const auto& plan : plans) {
            reviewed.emplace_back(plan);
        }
        out.emplace("plans", Json(r, reviewed));
    }
    return Json(r, out);
}

int main() {
    setenv("TEAQL_LOG_ENDPOINT", "off", 1);
    unsetenv("TEAQL_ALLOW_SENSITIVE_PLAINTEXT_LOGS");
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            auto r = user_context->memory_resource();
            auto input = Json::parse(r, line);
            if (!input) {
                throw std::runtime_error("invalid fixture");
            }
            std::cout << observe(*user_context, *input).dump(r) << '\n';
        }
    } catch (const std::exception& e) {
        std::pmr::set_default_resource(previous);
        std::cerr << e.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
