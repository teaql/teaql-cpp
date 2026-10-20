#include "../generated_candidates/include/cpp_candidates.hpp"
#include "fixture_value.hpp"
#include <teaql/core/query_debug.hpp>
#include <teaql/runtime/log_model_debug.hpp>
#include <teaql/runtime/mutation_policy_debug.hpp>
#include <sqlite3.h>
#include <iostream>
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

static void execute(sqlite3* database, std::string_view query) {
    std::string text(query);
    if (sqlite3_exec(database, text.c_str(), nullptr, nullptr, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
}

static Json snapshot(Memory r, sqlite3* database, bool candidates) {
    auto output = detail::model_json(r);
    const std::array candidate_names{"platform", "order_status", "person", "record"};
    const std::array catalog_names{"platform", "category"};
    auto names = candidates ? std::span<const char* const>(candidate_names)
                            : std::span<const char* const>(catalog_names);
    for (auto name : names) {
        String physical(name, r);
        physical += "_data";
        detail::model_put(output, name, table(r, database, physical));
    }
    return output;
}

static String trace_string(Memory r, const TraceChain& trace) {
    String result(r);
    DebugWriter writer(result, {});
    writer.sequence(trace, DefaultDebugElement{});
    return result;
}

struct CandidateSampleDomain {
    using Executor = cpp_candidates::DataServiceExecutor;
    using Schema = cpp_candidates::LocalSchemaProvider;

    static RuntimeModule& module(UserContext& user_context, std::string_view mode) {
        return mode == "plain"      ? cpp_candidates::module(user_context)
               : mode == "checker"  ? cpp_candidates::moduleWithCheckers(user_context)
               : mode == "behavior" ? cpp_candidates::moduleWithBehaviors(user_context)
                                    : cpp_candidates::moduleWithBehaviorsAndCheckers(user_context);
    }

    static SampleDataFuture& sample(UserContext& user_context, SampleDataPlan plan) {
        return cpp_candidates::generateSampleData(user_context, plan);
    }
};

struct SamplePolicy final : MutationPolicy {
    using allocator_type = Allocator;
    Vector<String>& plans;
    String target;

    SamplePolicy(std::allocator_arg_t, Allocator a, Vector<String>& plans, std::string_view target)
        : plans(plans), target(target, a.resource()) {}

    MutationPolicyIdentity identity(Memory r) const override {
        return MutationPolicyIdentity(
            std::allocator_arg, Allocator(r), "sample-policy", "1", "fingerprint");
    }

    MutationDecision review(Memory r, const UserContext&, const MutationPlan& plan) const override {
        plans.emplace_back(debug_string(r, plan));
        return plan.root_entity_type == target
                   ? MutationDecision::deny(r, "SAMPLE-DENIED", "fixture sample policy denial", {})
                   : MutationDecision::allow(r);
    }
};

struct CustomState final : SampleDataLogSink {
    using allocator_type = Allocator;
    mutable Json::Array calls;
    mutable Vector<String> messages;
    String checker_mode, behavior_mode;
    bool active;

    CustomState(std::allocator_arg_t,
                Allocator a,
                std::string_view checker,
                std::string_view behavior,
                bool enabled)
        : calls(a.resource()), messages(a.resource()), checker_mode(checker, a.resource()),
          behavior_mode(behavior, a.resource()), active(enabled) {}

    Memory memory_resource() const noexcept override {
        return calls.get_allocator().resource();
    }

    bool enabled(const UserContext&) const override {
        return active;
    }

    void info(const UserContext&, std::string_view message) const override {
        messages.emplace_back(message);
    }

    template <class Model>
    void check(std::string_view entity,
               Model& value,
               CheckObjectStatus status,
               const ObjectLocation& location,
               const CheckResults& results) const {
        auto r = memory_resource();
        auto call = detail::model_json(r);
        detail::model_put(call, "hook", "checker");
        detail::model_put(call, "entity", entity);
        detail::model_put(call, "id", value.id());
        detail::model_put(call, "name", value.name());
        detail::model_put(call,
                          "status",
                          status == CheckObjectStatus::Create   ? "Create"
                          : status == CheckObjectStatus::Update ? "Update"
                                                                : "Unknown");
        detail::model_put(call, "before", std::uint64_t(results.size()));
        detail::model_put(call, "location", location.model_path(r));
        calls.emplace_back(call);
    }
};

class PersonRules final : public cpp_candidates::PersonCheckerLogic {
public:
    using allocator_type = Allocator;
    CustomState& state;

    PersonRules(std::allocator_arg_t, Allocator a, CustomState& state)
        : PersonCheckerLogic(std::allocator_arg, a), state(state) {}

    void checkAndFixPerson(const UserContext& user_context,
                           cpp_candidates::Person& value,
                           CheckObjectStatus status,
                           const ObjectLocation& location,
                           CheckResults& results) const override {
        state.check("Person", value, status, location, results);
        if (state.checker_mode == "fix_person") {
            String name("fixed ", user_context.memory_resource());
            name += std::to_string(value.id());
            value.updateName(name).updatePhase("CUSTOM");
        }
        if (state.checker_mode == "reject_person" && value.id() == 3) {
            required(false, "custom_field", location, results);
        }
    }
};

class RecordRules final : public cpp_candidates::RecordCheckerLogic {
public:
    using allocator_type = Allocator;
    CustomState& state;

    RecordRules(std::allocator_arg_t, Allocator a, CustomState& state)
        : RecordCheckerLogic(std::allocator_arg, a), state(state) {}

    void checkAndFixRecord(const UserContext&,
                           cpp_candidates::Record& value,
                           CheckObjectStatus status,
                           const ObjectLocation& location,
                           CheckResults& results) const override {
        state.check("Record", value, status, location, results);
        if (state.checker_mode == "reject_record" && value.id() == 3) {
            required(false, "custom_field", location, results);
        }
    }
};

template <class Base>
class Behavior final : public Base {
public:
    using allocator_type = Allocator;
    CustomState& state;

    Behavior(std::allocator_arg_t, Allocator a, CustomState& state)
        : Base(std::allocator_arg, a), state(state) {}

    RuntimeHookResult
    before_select(Memory r, const UserContext&, SelectQuery& query) const override {
        auto call = detail::model_json(r);
        detail::model_put(call, "hook", "select");
        detail::model_put(call, "entity", query.entity);
        state.calls.emplace_back(call);
        if (query.entity == "OrderStatus") {
            if (state.behavior_mode == "reverse_constants") {
                query.order_by.emplace_back(OrderBy::desc(r, "id"));
            }
            if (state.behavior_mode == "deny_query") {
                return RuntimeHookResult::err(
                    r, RuntimeError::behavior(r, "fixture sample query denied"));
            }
        }
        return RuntimeHookResult::ok(r, {});
    }

    RuntimeHookResult
    before_insert(Memory r, const UserContext&, InsertCommand& command) const override {
        Json::Object values(r);
        for (const auto& [name, value] : command.values) {
            values.emplace(String(name, r), Json(r, debug_string(r, value)));
        }
        auto call = detail::model_json(r);
        detail::model_put(call, "hook", "insert");
        detail::model_put(call, "entity", command.entity);
        detail::model_put(call, "values", Json(r, values));
        state.calls.emplace_back(call);
        auto found = command.values.fields.find("id");
        auto id = found == command.values.fields.end() ? 0 : found->second.try_u64().value_or(0);
        if (id == 3 &&
            ((state.behavior_mode == "deny_insert_person" && command.entity == "Person") ||
             (state.behavior_mode == "deny_insert_record" && command.entity == "Record"))) {
            return RuntimeHookResult::err(
                r, RuntimeError::behavior(r, "fixture sample insert denied"));
        }
        return RuntimeHookResult::ok(r, {});
    }
};

template <class Domain>
static Json observe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto mode = text(field(input, "module"));
    bool candidates = text(field(input, "fixture")) == "candidates";
    auto& runtime_module = Domain::module(user_context, mode);
    auto connection = user_context.make_resource_ref<SqliteConnection>();
    auto& schema_provider = user_context.make<typename Domain::Schema>();
    assemble_generated_sqlite_runtime<typename Domain::Executor, false>(
        user_context, runtime_module, connection, schema_provider);
    auto& clock = user_context.make<FixedBusinessClock>(
        UtcDateTime::from_timestamp(1'700'000'000, 0).value());
    user_context.set_business_clock(user_context, clock);
    user_context.disable_sql_log();
    const auto& schema = user_context.ensure_schema().wait();
    if (!schema) {
        auto output = detail::model_json(r);
        detail::model_put(output, "schema_error", schema.error().to_string(r));
        return output;
    }
    auto* database = user_context.get(connection).native_handle();
    execute(database, "PRAGMA foreign_keys=OFF;");
    auto state = text(field(input, "state"));
    std::string constant = candidates ? "order_status_data" : "category_data";
    if (state == "missing_root") {
        execute(database, "DELETE FROM platform_data;");
    } else if (state == "missing_constant") {
        execute(database, "DELETE FROM " + constant + ";");
    } else if (state == "deleted_constant") {
        execute(database, "UPDATE " + constant + " SET version=-1;");
    } else if (state == "failed_constant_query") {
        execute(database, "DROP TABLE " + constant + ";");
    } else if (state != "normal") {
        throw std::logic_error("Invalid sample state");
    }
    if (auto* fault = optional(input, "fault")) {
        auto name = text(*fault);
        if (!candidates || (name != "person" && name != "record")) {
            throw std::logic_error("Invalid sample fault table");
        }
        execute(
            database,
            "CREATE TRIGGER sample_failure BEFORE INSERT ON " + std::string(name) +
                "_data WHEN NEW.id=3 BEGIN SELECT RAISE(ABORT,'sample catalog fixture failed'); END;");
    }
    execute(database, "PRAGMA foreign_keys=ON;");
    auto& custom = user_context.make<CustomState>(text(field(input, "checker")),
                                                  text(field(input, "behavior")),
                                                  std::get<bool>(field(input, "enabled").data));
    RuntimeModule& application_module = Domain::module(user_context, mode);
    if (custom.checker_mode != "none") {
        auto& person_logic = user_context.make<PersonRules>(custom);
        auto& person_checker = user_context.make<cpp_candidates::PersonChecker>(person_logic);
        auto& person_adapter =
            user_context
                .make<TypedEntityChecker<cpp_candidates::Person, cpp_candidates::PersonChecker>>(
                    user_context, person_checker);
        application_module.checker(user_context, person_adapter);
        auto& record_logic = user_context.make<RecordRules>(custom);
        auto& record_checker = user_context.make<cpp_candidates::RecordChecker>(record_logic);
        auto& record_adapter =
            user_context
                .make<TypedEntityChecker<cpp_candidates::Record, cpp_candidates::RecordChecker>>(
                    user_context, record_checker);
        application_module.checker(user_context, record_adapter);
    }
    if (custom.behavior_mode != "none") {
        application_module.entity_with_behavior<cpp_candidates::Platform>(
            user_context, user_context.make<Behavior<cpp_candidates::PlatformBehavior>>(custom));
        application_module.entity_with_behavior<cpp_candidates::OrderStatus>(
            user_context, user_context.make<Behavior<cpp_candidates::OrderStatusBehavior>>(custom));
        application_module.entity_with_behavior<cpp_candidates::Person>(
            user_context, user_context.make<Behavior<cpp_candidates::PersonBehavior>>(custom));
        application_module.entity_with_behavior<cpp_candidates::Record>(
            user_context, user_context.make<Behavior<cpp_candidates::RecordBehavior>>(custom));
    }
    application_module.apply_to(user_context);
    user_context.insert_resource(user_context, static_cast<SampleDataLogSink&>(custom));
    user_context.set_trace_id("sample");
    user_context.set_user_identifier("fixture");
    auto& plans = user_context.make<Vector<String>>();
    auto governance = text(field(input, "governance"));
    if (governance != "default") {
        auto target = governance == "deny_person"   ? "Person"
                      : governance == "deny_record" ? "Record"
                                                    : "";
        auto& policy = user_context.make<SamplePolicy>(plans, target);
        auto registry = user_context.make_ref<FunctionMutationPolicyRegistry>(
            [&policy, r](Memory target, std::string_view) -> std::optional<MutationPolicyBorrow> {
                return MutationPolicyBorrow(
                    std::allocator_arg, Allocator(target), policy, detail::resource_arena(r));
            });
        user_context.set_mutation_policy_registry(registry);
        if (governance == "approved") {
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
    user_context.enable_all_sql_log();
    Json::Array rounds(r);
    for (std::uint64_t i = 0; i < std::get<std::uint64_t>(field(input, "rounds").data); ++i) {
        user_context.clear_sql_logs();
        plans.clear();
        custom.calls.clear();
        custom.messages.clear();
        auto before = snapshot(r, database, candidates);
        auto scale =
            text(field(input, "scale")) == "Small" ? SampleDataScale::Small : SampleDataScale::Tiny;
        const auto& result =
            Domain::sample(user_context,
                           {scale, std::get<std::uint64_t>(field(input, "seed").data)})
                .wait();
        auto report = detail::model_json(r);
        if (result) {
            Json::Object counts(r);
            for (const auto& [name, count] : result.value().generated) {
                counts.emplace(String(name, r), Json(r, std::uint64_t(count)));
            }
            Json::Array skipped(r);
            for (const auto& skip : result.value().skipped) {
                auto item = detail::model_json(r);
                detail::model_put(item, "entity", skip.entity);
                detail::model_put(item, "reason", skip.reason);
                skipped.emplace_back(item);
            }
            detail::model_put(report, "generated", Json(r, counts));
            detail::model_put(report, "skipped", Json(r, skipped));
            detail::model_put(report, "error", Json(r));
        } else {
            detail::model_put(report, "error", result.error().to_string(r));
        }
        Json::Array logs(r);
        for (const auto& e : user_context.sql_logs()) {
            auto item = detail::model_json(r);
            detail::model_put(item, "context", debug_string(r, e.log_context));
            detail::model_put(item, "operation", debug_string(r, e.operation));
            detail::model_put(item, "sql", e.sql);
            Json::Array params(r);
            for (const auto& v : e.params) {
                params.emplace_back(debug_string(r, v));
            }
            detail::model_put(item, "params", Json(r, params));
            detail::model_put(item, "comment", e.comment ? Json(r, *e.comment) : Json(r));
            detail::model_put(item, "purpose", e.purpose ? Json(r, *e.purpose) : Json(r));
            detail::model_put(
                item, "audit_reason", e.audit_reason ? Json(r, *e.audit_reason) : Json(r));
            detail::model_put(item, "trace", trace_string(r, e.trace_path));
            detail::model_put(item, "debug_sql", e.debug_sql);
            detail::model_put(item, "pretty_sql", e.pretty_sql);
            detail::model_put(item,
                              "result_count",
                              e.result_count ? Json(r, std::uint64_t(*e.result_count)) : Json(r));
            detail::model_put(
                item, "result_type", e.result_type ? Json(r, *e.result_type) : Json(r));
            detail::model_put(
                item, "affected_rows", e.affected_rows ? Json(r, *e.affected_rows) : Json(r));
            detail::model_put(item, "summary", e.result_summary);
            logs.emplace_back(item);
        }
        auto round = detail::model_json(r);
        detail::model_put(round, "before", before);
        detail::model_put(round, "report", report);
        detail::model_put(round, "after", snapshot(r, database, candidates));
        detail::model_put(round, "logs", Json(r, logs));
        Json::Array reviewed(r);
        for (const auto& plan : plans) {
            reviewed.emplace_back(plan);
        }
        detail::model_put(round, "plans", Json(r, reviewed));
        detail::model_put(round, "calls", Json(r, custom.calls));
        Json::Array messages(r);
        for (const auto& message : custom.messages) {
            messages.emplace_back(message);
        }
        detail::model_put(round, "messages", Json(r, messages));
        rounds.emplace_back(round);
    }
    auto output = detail::model_json(r);
    detail::model_put(output, "schema_error", Json(r));
    detail::model_put(output, "rounds", Json(r, rounds));
    return output;
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        for (std::string line; std::getline(std::cin, line);) {
            ContextLimits limits;
            limits.user.max_bytes = 512ull * 1024 * 1024;
            limits.max_live_user_bytes = limits.user.max_bytes;
            GlobalContext global(std::pmr::new_delete_resource(), 4096, limits);
            auto user_context = global.make_user_context();
            {
                auto input = Json::parse(user_context->memory_resource(), line);
                std::cout << observe<CandidateSampleDomain>(*user_context, *input)
                                 .dump(user_context->memory_resource())
                          << '\n';
            }
            user_context.reset();
            if (global.live_user_bytes() != 0) {
                throw std::logic_error("Sample observer retained User memory");
            }
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
