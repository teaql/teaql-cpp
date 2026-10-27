#include <cpp_inheritance_graph.hpp>
#include <teaql/runtime.hpp>

#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <sstream>
#include <sqlite3.h>
#include <stdexcept>
#include <string>

using namespace teaql;
using namespace cpp_inheritance_graph;

struct Sink final : SafeAuditEventSink {
    Json& safe_field;
    String& entity;
    std::size_t& event_count;
    Vector<String>& event_entities;
    Vector<String>& approved_event_entities;
    bool& approved_event;
    bool fail;

    Sink(Json& safe_field,
         String& entity,
         std::size_t& event_count,
         Vector<String>& event_entities,
         Vector<String>& approved_event_entities,
         bool& approved_event,
         bool fail)
        : safe_field(safe_field), entity(entity), event_count(event_count),
          event_entities(event_entities), approved_event_entities(approved_event_entities),
          approved_event(approved_event), fail(fail) {}

    Result<std::monostate, RuntimeError>
    on_safe_event(Memory resource, const UserContext&, const SafeAuditEvent& event) const override {
        ++event_count;
        if (event.kind != RawAuditEventKind::Updated) {
            throw std::runtime_error("Expected an updated audit event");
        }
        event_entities.emplace_back(event.entity);
        const bool approved =
            event.mutation_governance &&
            event.mutation_governance->approval_status == MutationPolicyApprovalStatus::Approved;
        if (approved) {
            approved_event_entities.emplace_back(event.entity);
        }
        if (event.entity != "School") {
            return Result<std::monostate, RuntimeError>::ok(resource, {});
        }
        entity = event.entity;
        approved_event = approved;
        for (const auto& field : event.fields) {
            if (field.name != "name") {
                continue;
            }
            Json::Object output(resource);
            output.emplace("name", Json(resource, field.name));
            output.emplace("value", field.value ? Json(resource, *field.value) : Json(resource));
            output.emplace("masked", Json(resource, field.masked));
            output.emplace("truncated", Json(resource, field.truncated));
            output.emplace("raw_length",
                           field.raw_length
                               ? Json(resource, static_cast<std::uint64_t>(*field.raw_length))
                               : Json(resource));
            output.emplace("output_length",
                           field.output_length
                               ? Json(resource, static_cast<std::uint64_t>(*field.output_length))
                               : Json(resource));
            output.emplace("mask_reason",
                           field.mask_reason ? Json(resource, *field.mask_reason) : Json(resource));
            output.emplace("truncate_reason",
                           field.truncate_reason ? Json(resource, *field.truncate_reason)
                                                 : Json(resource));
            safe_field = Json(resource, std::move(output));
        }
        return fail
                   ? Result<std::monostate, RuntimeError>::err(
                         resource, RuntimeError::event(resource, "synthetic audit delivery failed"))
                   : Result<std::monostate, RuntimeError>::ok(resource, {});
    }
};

struct SchoolPolicy final : MutationPolicy {
    Vector<String>& reviews;
    Vector<std::size_t>& operation_counts;
    bool deny;

    SchoolPolicy(Vector<String>& reviews, Vector<std::size_t>& operation_counts, bool deny)
        : reviews(reviews), operation_counts(operation_counts), deny(deny) {}

    MutationPolicyIdentity identity(Memory resource) const override {
        return MutationPolicyIdentity(
            std::allocator_arg, Allocator(resource), "school-audit", "1", "source-derived");
    }

    MutationDecision
    review(Memory resource, const UserContext&, const MutationPlan& plan) const override {
        reviews.emplace_back(plan.request_key);
        operation_counts.emplace_back(plan.operations.size());
        if (!deny) {
            return MutationDecision::allow(resource);
        }
        const std::array<String, 1> fields{String("name", resource)};
        return MutationDecision::deny(resource, "SCHOOL-DENIED", "synthetic policy denial", fields);
    }
};

static sqlite3* initialize(UserContext& user_context) {
    auto& module = moduleWithBehaviorsAndCheckers(user_context);
    module.apply_to(user_context);
    user_context.set_metadata(user_context, module.metadata);
    auto& connection = user_context.make_resource<SqliteConnection>();
    auto& transport = user_context.make<SqliteMutationExecutor>(connection);
    use_sqlite_provider(user_context, user_context, transport);
    auto dialect = user_context.require_resource<SqliteDialect>();
    auto& state = user_context.make<SqlExecutorState>();
    auto executor = user_context.make_ref<cpp_inheritance_graph::DataServiceExecutor>(
        require_value(dialect).get(), transport, module.metadata, state);
    user_context.register_executor(executor);
    std::ifstream file(std::string(TEAQL_SOURCE_DIR) +
                       "/tests/support/dynamic_json_inherited_rows.sql");
    std::ostringstream sql;
    sql << file.rdbuf();
    if (!file ||
        sqlite3_exec(connection.native_handle(), sql.str().c_str(), nullptr, nullptr, nullptr) !=
            SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(connection.native_handle()));
    }
    user_context.with_active_root("Platform", 1);
    return connection.native_handle();
}

static Json observe(UserContext& user_context, std::string_view mode) {
    auto resource = user_context.memory_resource();
    auto* database = initialize(user_context);
    Json safe_field(resource);
    String entity(resource);
    std::size_t event_count = 0;
    auto& event_entities = user_context.make<Vector<String>>();
    auto& approved_event_entities = user_context.make<Vector<String>>();
    bool approved_event = false;
    auto& sink = user_context.make_resource<Sink>(safe_field,
                                                  entity,
                                                  event_count,
                                                  event_entities,
                                                  approved_event_entities,
                                                  approved_event,
                                                  mode == "sink_failure");
    user_context.set_custom_event_sink(user_context, sink);
    auto& reviews = user_context.make<Vector<String>>();
    auto& operation_counts = user_context.make<Vector<std::size_t>>();
    if (mode == "policy_denied" || mode == "policy_approved" || mode == "graph_policy_denied" ||
        mode == "graph_policy_approved") {
        auto& policy = user_context.make_resource<SchoolPolicy>(
            reviews,
            operation_counts,
            mode != "policy_approved" && mode != "graph_policy_approved");
        auto registry = user_context.make_ref<FunctionMutationPolicyRegistry>(
            [&policy, resource](Memory target,
                                std::string_view) -> std::optional<MutationPolicyBorrow> {
                return MutationPolicyBorrow(std::allocator_arg,
                                            Allocator(target),
                                            policy,
                                            detail::resource_arena(resource));
            });
        user_context.set_mutation_policy_registry(registry);
        if (mode == "policy_approved" || mode == "graph_policy_approved") {
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

    auto Q = user_context.queries<query_factory>();
    const auto& selected =
        require_value(Q.schools()
                          .withIdIs(7)
                          .comment("Load School for audited update")
                          .purpose("Verify generated audit policy on persistence")
                          .executeForOne(user_context)
                          .wait());
    if (!selected) {
        throw std::runtime_error("School fixture row is absent");
    }
    auto& school = user_context.make<School>(*selected);
    school.updateName("PRIVATE-SCHOOL-CANARY");
    if (mode == "graph_success" || mode == "graph_policy_denied" ||
        mode == "graph_policy_approved" || mode == "graph_write_failure" ||
        mode == "graph_stale_child" || mode == "graph_stale_root") {
        const auto& selected_enrollment =
            require_value(Q.enrollments()
                              .withIdIs(21)
                              .comment("Load Enrollment for audited graph update")
                              .purpose("Verify generated graph audit policy")
                              .executeForOne(user_context)
                              .wait());
        if (!selected_enrollment) {
            throw std::runtime_error("Enrollment fixture row is absent");
        }
        auto& enrollment = user_context.make<Enrollment>(*selected_enrollment);
        enrollment.updateNote("PRIVATE-NOTE-CANARY");
        if (!school.includePendingMutationsFrom(enrollment)) {
            throw std::runtime_error("Failed to include Enrollment pending mutation");
        }
    }
    if (mode == "write_failure" &&
        sqlite3_exec(database,
                     "CREATE TRIGGER fixture_failure BEFORE UPDATE ON school_data "
                     "BEGIN SELECT RAISE(ABORT,'fixture write failed'); END;",
                     nullptr,
                     nullptr,
                     nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
    if (mode == "graph_write_failure" &&
        sqlite3_exec(database,
                     "CREATE TRIGGER fixture_graph_failure BEFORE UPDATE ON school_data "
                     "BEGIN SELECT RAISE(ABORT,'fixture root write failed'); END;",
                     nullptr,
                     nullptr,
                     nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
    if (mode == "stale_version" &&
        sqlite3_exec(database,
                     "UPDATE school_data SET version=version+1 WHERE id=7",
                     nullptr,
                     nullptr,
                     nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
    if (mode == "graph_stale_child" &&
        sqlite3_exec(database,
                     "UPDATE enrollment_data SET version=version+1 WHERE id=21",
                     nullptr,
                     nullptr,
                     nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
    if (mode == "graph_stale_root" &&
        sqlite3_exec(database,
                     "UPDATE school_data SET version=version+1 WHERE id=7",
                     nullptr,
                     nullptr,
                     nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
    const auto& saved =
        AuditedSaveExt<School>::save(std::move(school).audit_as("Update synthetic School name"),
                                     user_context)
            .wait();
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(
            database, "SELECT name,version FROM school_data WHERE id=7", -1, &statement, nullptr) !=
        SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
    std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> cleanup(statement, sqlite3_finalize);
    if (sqlite3_step(statement) != SQLITE_ROW) {
        throw std::runtime_error("Saved School row is absent");
    }
    sqlite3_stmt* enrollment_statement = nullptr;
    if (sqlite3_prepare_v2(database,
                           "SELECT note,version FROM enrollment_data WHERE id=21",
                           -1,
                           &enrollment_statement,
                           nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
    std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> enrollment_cleanup(
        enrollment_statement, sqlite3_finalize);
    if (sqlite3_step(enrollment_statement) != SQLITE_ROW) {
        throw std::runtime_error("Saved Enrollment row is absent");
    }
    Json::Object output(resource);
    output.emplace("mode", Json(resource, mode));
    output.emplace("save_ok", Json(resource, static_cast<bool>(saved)));
    output.emplace(
        "optimistic_conflict",
        Json(resource,
             !saved && saved.error().kind() == RuntimeError::Kind::OptimisticLockConflict));
    output.emplace("conflict_entity",
                   !saved && saved.error().kind() == RuntimeError::Kind::OptimisticLockConflict
                       ? Json(resource, saved.error().optimistic_lock_conflict_error().entity)
                       : Json(resource));
    output.emplace("policy_error",
                   Json(resource, !saved && saved.error().kind() == RuntimeError::Kind::Policy));
    bool fixture_write_failure = false;
    if (!saved) {
        const auto message = saved.error().to_string(resource);
        fixture_write_failure = message.find("fixture write failed") != String::npos ||
                                message.find("fixture root write failed") != String::npos;
    }
    output.emplace("fixture_write_failure", Json(resource, fixture_write_failure));
    Json::Array policy_reviews(resource);
    for (const auto& key : reviews) {
        policy_reviews.emplace_back(key);
    }
    output.emplace("policy_reviews", Json(resource, std::move(policy_reviews)));
    Json::Array policy_operation_counts(resource);
    for (auto count : operation_counts) {
        policy_operation_counts.emplace_back(Json(resource, static_cast<std::uint64_t>(count)));
    }
    output.emplace("policy_operation_counts", Json(resource, std::move(policy_operation_counts)));
    output.emplace("approved_event", Json(resource, approved_event));
    std::sort(approved_event_entities.begin(), approved_event_entities.end());
    Json::Array approved_event_names(resource);
    for (const auto& name : approved_event_entities) {
        approved_event_names.emplace_back(name);
    }
    output.emplace("approved_event_entities", Json(resource, std::move(approved_event_names)));
    std::sort(event_entities.begin(), event_entities.end());
    Json::Array event_entity_names(resource);
    for (const auto& name : event_entities) {
        event_entity_names.emplace_back(name);
    }
    output.emplace("event_entities", Json(resource, std::move(event_entity_names)));
    output.emplace(
        "db_name",
        Json(resource, reinterpret_cast<const char*>(sqlite3_column_text(statement, 0))));
    output.emplace("version", Json(resource, std::int64_t(sqlite3_column_int64(statement, 1))));
    output.emplace(
        "enrollment_note",
        Json(resource,
             reinterpret_cast<const char*>(sqlite3_column_text(enrollment_statement, 0))));
    output.emplace("enrollment_version",
                   Json(resource, std::int64_t(sqlite3_column_int64(enrollment_statement, 1))));
    output.emplace("event_count", Json(resource, static_cast<std::uint64_t>(event_count)));
    output.emplace("entity", event_count ? Json(resource, entity) : Json(resource));
    output.emplace("safe_field", std::move(safe_field));
    return Json(resource, std::move(output));
}

int main(int argc, char** argv) {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        const std::string_view mode = argc == 2 ? argv[1] : "success";
        if (mode != "success" && mode != "write_failure" && mode != "sink_failure" &&
            mode != "policy_denied" && mode != "stale_version" && mode != "policy_approved" &&
            mode != "graph_success" && mode != "graph_policy_denied" &&
            mode != "graph_policy_approved" && mode != "graph_write_failure" &&
            mode != "graph_stale_child" && mode != "graph_stale_root") {
            throw std::invalid_argument("Unknown Save mode");
        }
        GlobalContext global;
        auto user_context = global.make_user_context();
        {
            auto result = observe(*user_context, mode);
            std::cout << result.dump(user_context->memory_resource()) << '\n';
        }
        user_context.reset();
        if (global.live_user_bytes() != 0) {
            throw std::runtime_error("Audited Save retained UserContext memory");
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
