#include "compiled_scalar_storage.hpp"
#include "fixture_value.hpp"
#include <array>
#include <iostream>
#include <sqlite3.h>
#include <type_traits>

using namespace compiled_scalar_storage;

static_assert(std::is_same_v<std::remove_cvref_t<decltype(std::declval<const Reading&>().value())>,
                             TEAQL_EXPECTED_VALUE_TYPE>);

static void execute_sql(sqlite3* database, std::string_view sql) {
    if (sqlite3_exec(database, std::string(sql).c_str(), nullptr, nullptr, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
}

static Json rows(Memory resource, sqlite3* database, std::string_view sql) {
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(database, std::string(sql).c_str(), -1, &statement, nullptr) !=
        SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
    std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> cleanup(statement, sqlite3_finalize);
    Json::Array result(resource);
    for (auto status = sqlite3_step(statement); status != SQLITE_DONE;
         status = sqlite3_step(statement)) {
        if (status != SQLITE_ROW) {
            throw std::runtime_error(sqlite3_errmsg(database));
        }
        Json::Object row(resource);
        for (int index = 0; index != sqlite3_column_count(statement); ++index) {
            Json item(resource);
            switch (sqlite3_column_type(statement, index)) {
            case SQLITE_NULL:
                break;
            case SQLITE_INTEGER:
                item = Json(resource, std::int64_t(sqlite3_column_int64(statement, index)));
                break;
            case SQLITE_FLOAT:
                item = Json(resource, sqlite3_column_double(statement, index));
                break;
            case SQLITE_TEXT:
                item = Json(resource,
                            reinterpret_cast<const char*>(sqlite3_column_text(statement, index)));
                break;
            default:
                throw std::runtime_error("Unexpected scalar fixture SQLite storage");
            }
            row.emplace(String(sqlite3_column_name(statement, index), resource), std::move(item));
        }
        result.emplace_back(Json(resource, std::move(row)));
    }
    return Json(resource, std::move(result));
}

static Json database_snapshot(Memory resource, sqlite3* database) {
    Json::Object result(resource);
    result.emplace(
        "gateways",
        rows(resource, database, "SELECT id,name,version FROM gateway_data ORDER BY id"));
    result.emplace(
        "readings",
        rows(resource, database, "SELECT id,gateway,value,version FROM reading_data ORDER BY id"));
    return Json(resource, std::move(result));
}

static Json schema_snapshot(Memory resource, sqlite3* database) {
    Json::Object result(resource);
    result.emplace(
        "gateway",
        rows(
            resource,
            database,
            "SELECT name,type,\"notnull\",dflt_value,pk FROM pragma_table_info('gateway_data') ORDER BY cid"));
    result.emplace(
        "reading",
        rows(
            resource,
            database,
            "SELECT name,type,\"notnull\",dflt_value,pk FROM pragma_table_info('reading_data') ORDER BY cid"));
    result.emplace(
        "foreign_keys",
        rows(
            resource,
            database,
            "SELECT \"table\",\"from\",\"to\",on_update,on_delete FROM pragma_foreign_key_list('reading_data') ORDER BY id,seq"));
    return Json(resource, std::move(result));
}

static Json trace_json(Memory resource, const TraceChain& trace) {
    static constexpr std::array names{"Operation",
                                      "Request",
                                      "Relation",
                                      "Entity",
                                      "Provider",
                                      "Sql",
                                      "Comment",
                                      "Purpose",
                                      "AuditReason"};
    Json::Array result(resource);
    for (const auto& node : trace) {
        Json::Object item(resource);
        item.emplace("kind", Json(resource, names.at(static_cast<std::size_t>(node.kind))));
        item.emplace("entity", Json(resource, node.entity_type));
        item.emplace("id", node.entity_id ? Json(resource, *node.entity_id) : Json(resource));
        item.emplace("comment", Json(resource, node.comment));
        result.emplace_back(Json(resource, std::move(item)));
    }
    return Json(resource, std::move(result));
}

struct Ids final : InternalIdGenerator {
    using allocator_type = Allocator;
    mutable Vector<String> calls;
    mutable std::pmr::map<String, std::uint64_t, std::less<>> counters;

    Ids(std::allocator_arg_t, Allocator allocator)
        : calls(allocator.resource()), counters(allocator.resource()) {}

    Result<std::uint64_t, RuntimeError> generate_id(Memory resource,
                                                    std::string_view entity) const override {
        calls.emplace_back(entity);
        auto key = canonical_id_space_entity(resource, entity);
        auto initial = key == "gateway" ? std::uint64_t{0} : std::uint64_t{6};
        auto position = counters.try_emplace(std::move(key), initial).first;
        return Result<std::uint64_t, RuntimeError>::ok(resource, ++position->second);
    }

protected:
    Result<std::monostate, RuntimeError>
    do_ensure_floor(Memory resource, std::string_view entity, std::uint64_t floor) const override {
        auto position = counters.try_emplace(canonical_id_space_entity(resource, entity), 0).first;
        position->second = std::max(position->second, floor);
        return Result<std::monostate, RuntimeError>::ok(resource, {});
    }
};

struct Policy final : MutationPolicy {
    Json::Array& reviews;
    bool deny;

    Policy(Json::Array& values, bool denial) : reviews(values), deny(denial) {}

    MutationPolicyIdentity identity(Memory resource) const override {
        return MutationPolicyIdentity(
            std::allocator_arg, Allocator(resource), "scalar-policy", "1", "scalar-policy-v1");
    }

    MutationDecision
    review(Memory resource, const UserContext&, const MutationPlan& plan) const override {
        Json::Array operations(resource);
        for (const auto& operation : plan.operations) {
            Json::Object item(resource), fields(resource);
            for (const auto& [name, value] : operation.changed_values) {
                fields.emplace(String(name, resource), value.to_json_value(resource));
            }
            item.emplace(
                "kind",
                Json(resource,
                     operation.kind == MutationOperationKind::Create ? "Create" : "Update"));
            item.emplace("entity", Json(resource, operation.entity.entity));
            item.emplace("id", operation.entity.id.to_json_value(resource));
            item.emplace("original_version",
                         operation.original_version ? Json(resource, *operation.original_version)
                                                    : Json(resource));
            item.emplace("fields", Json(resource, std::move(fields)));
            operations.emplace_back(Json(resource, std::move(item)));
        }
        Json::Object review(resource);
        review.emplace("request", Json(resource, plan.request_key));
        review.emplace("root", Json(resource, plan.root_entity_type));
        review.emplace("reason",
                       plan.audit_reason ? Json(resource, *plan.audit_reason) : Json(resource));
        review.emplace("operations", Json(resource, std::move(operations)));
        reviews.emplace_back(Json(resource, std::move(review)));
        if (deny) {
            return MutationDecision::deny(
                resource, "SCALAR-DENIED", "fixture scalar policy denial", {});
        }
        return MutationDecision::allow(resource);
    }
};

struct Sink final : SafeAuditEventSink {
    Json::Array& events;
    bool fail;

    Sink(Json::Array& values, bool failure) : events(values), fail(failure) {}

    Result<std::monostate, RuntimeError>
    on_safe_event(Memory resource, const UserContext&, const SafeAuditEvent& event) const override {
        Json::Array fields(resource);
        for (const auto& field : event.fields) {
            Json::Object item(resource);
            item.emplace("name", Json(resource, field.name));
            item.emplace("value", field.value ? Json(resource, *field.value) : Json(resource));
            item.emplace("masked", Json(resource, field.masked));
            item.emplace("truncated", Json(resource, field.truncated));
            fields.emplace_back(Json(resource, std::move(item)));
        }
        Json::Object item(resource);
        item.emplace("entity", Json(resource, event.entity));
        item.emplace("id", event.entity_id ? Json(resource, *event.entity_id) : Json(resource));
        item.emplace(
            "kind",
            Json(resource, event.kind == RawAuditEventKind::Created ? "Created" : "Updated"));
        item.emplace("fields", Json(resource, std::move(fields)));
        item.emplace("trace", trace_json(resource, event.trace_chain));
        item.emplace("approved",
                     Json(resource,
                          event.mutation_governance && event.mutation_governance->approval_status ==
                                                           MutationPolicyApprovalStatus::Approved));
        events.emplace_back(Json(resource, std::move(item)));
        if (fail) {
            return Result<std::monostate, RuntimeError>::err(
                resource, RuntimeError::event(resource, "fixture scalar audit delivery failed"));
        }
        return Result<std::monostate, RuntimeError>::ok(resource, {});
    }
};

static Json fixes(const UserContext& user_context) {
    auto resource = user_context.memory_resource();
    Json::Array result(resource);
    for (const auto& evidence : user_context.last_fix_evidence()) {
        Json::Object item(resource);
        item.emplace("entity", Json(resource, evidence.entity_type));
        item.emplace("path", Json(resource, evidence.model_path));
        item.emplace(
            "source",
            Json(resource, evidence.source == FixEvidenceSource::Context ? "Context" : "Clock"));
        item.emplace("label", Json(resource, evidence.source_label));
        result.emplace_back(Json(resource, std::move(item)));
    }
    return Json(resource, std::move(result));
}

template <class Model>
static Json save(UserContext& user_context, Model& entity, std::string_view reason) {
    auto resource = user_context.memory_resource();
    const auto& result =
        AuditedSaveExt<Model>::save(std::move(entity).audit_as(reason), user_context).wait();
    Json::Object outcome(resource);
    outcome.emplace("ok", Json(resource, bool(result)));
    outcome.emplace("policy_error",
                    Json(resource, !result && result.error().kind() == RuntimeError::Kind::Policy));
    outcome.emplace(
        "conflict",
        Json(resource,
             !result && result.error().kind() == RuntimeError::Kind::OptimisticLockConflict));
    outcome.emplace(
        "committed_error",
        Json(resource, !result && result.error().kind() == RuntimeError::Kind::AuditAfterCommit));
    if (!result) {
        Json::Object diagnostic(resource);
        diagnostic.emplace("error", Json(resource, result.error().to_string(resource)));
        std::cerr << Json(resource, std::move(diagnostic)).dump(resource) << '\n';
    }
    return Json(resource, std::move(outcome));
}

static Json observe(UserContext& user_context, const Json& input) {
    auto resource = user_context.memory_resource();
    auto connection = user_context.make_resource_ref<SqliteConnection>();
    require_value(serviceRuntimeFromConnection(user_context, connection));
    auto* database = user_context.get(connection).native_handle();
    auto& ids = user_context.make<Ids>();
    user_context.set_internal_id_generator(user_context, ids);
    user_context.with_active_root("Gateway", 1);
    require_value(user_context.ensure_schema().wait());
    auto first = database_snapshot(resource, database);
    require_value(user_context.ensure_schema().wait());
    auto second = database_snapshot(resource, database);
    auto Q = user_context.queries<query_factory>();
    auto mode = text(field(input, "mode"));
    auto decoded_input = value(resource, field(input, "typed"));
    auto decoded = GeneratedScalarCodec::decode<TEAQL_EXPECTED_VALUE_TYPE>(
        resource, &decoded_input, "Reading", "value");
    const auto& desired = require_value(decoded);
    Json::Object output(resource);
    output.emplace("schema", schema_snapshot(resource, database));
    output.emplace("bootstrap_first", std::move(first));
    output.emplace("bootstrap_second", std::move(second));
    auto& reviews = user_context.make<Json::Array>();
    auto& events = user_context.make<Json::Array>();
    auto log_start = user_context.sql_logs().size();
    ids.calls.clear();
    if (mode != "create") {
        auto& seed = Q.readings()
                         .comment("Stage typed scalar seed")
                         .purpose("Prepare governed scalar fixture")
                         .newEntity(user_context);
        seed.updateValue(desired);
        output.emplace("seed", save(user_context, seed, "Create typed scalar seed"));
        output.emplace("seed_fixes", fixes(user_context));
        ids.calls.clear();
        log_start = user_context.sql_logs().size();
    }
    auto& policy = user_context.make_resource<Policy>(reviews, mode == "deny");
    auto registry = user_context.make_ref<FunctionMutationPolicyRegistry>(
        [&policy, resource](Memory destination,
                            std::string_view) -> std::optional<MutationPolicyBorrow> {
            return MutationPolicyBorrow(std::allocator_arg,
                                        Allocator(destination),
                                        policy,
                                        detail::resource_arena(resource));
        });
    user_context.set_mutation_policy_registry(registry);
    if (mode == "approved") {
        auto approval = user_context.make_ref<FunctionMutationPolicyApprovalProvider>(
            [](Memory destination,
               const MutationPolicyIdentity& identity) -> std::optional<MutationPolicyApproval> {
                return MutationPolicyApproval(std::allocator_arg,
                                              Allocator(destination),
                                              identity,
                                              "fixture-reviewer",
                                              SystemTime::from_unix_seconds(0));
            });
        user_context.set_mutation_policy_approval_provider(approval);
    }
    auto& sink = user_context.make_resource<Sink>(events, mode == "sink_fail");
    user_context.set_custom_event_sink(user_context, sink);
    if (mode == "create") {
        auto& entity = Q.readings()
                           .comment("Stage typed scalar creation")
                           .purpose("Qualify generated scalar persistence")
                           .newEntity(user_context);
        entity.updateValue(desired);
        output.emplace("outcome", save(user_context, entity, "Save typed scalar creation"));
    } else {
        const auto& selected = Q.readings()
                                   .selectSelfFields()
                                   .withIdIs(7)
                                   .comment("Load typed scalar for update")
                                   .purpose("Qualify generated scalar persistence")
                                   .executeForOne(user_context)
                                   .wait();
        if (!selected) {
            Json::Object outcome(resource);
            outcome.emplace("stage", Json(resource, "load"));
            outcome.emplace("ok", Json(resource, false));
            output.emplace("outcome", Json(resource, std::move(outcome)));
            std::cerr << selected.error().to_string(resource) << '\n';
        } else {
            if (!selected.value()) {
                throw std::runtime_error("Saved scalar seed row is absent");
            }
            auto& entity = user_context.make<Reading>(*selected.value());
            auto changed_input = value(resource, field(input, "next_typed"));
            auto changed = GeneratedScalarCodec::decode<TEAQL_EXPECTED_VALUE_TYPE>(
                resource, &changed_input, "Reading", "value");
            entity.updateValue(require_value(changed));
            if (mode == "update") {
                output.emplace("outcome", save(user_context, entity, "Save typed scalar update"));
            } else {
                const auto& root = require_value(Q.gateways()
                                                     .selectSelfFields()
                                                     .withIdIs(1)
                                                     .comment("Load scalar aggregate root")
                                                     .purpose("Qualify generated scalar graph Save")
                                                     .executeForOne(user_context)
                                                     .wait());
                if (!root) {
                    throw std::runtime_error("Generated Gateway root is absent");
                }
                auto& gateway = user_context.make<Gateway>(*root);
                gateway.updateName("UPDATED-ROOT");
                if (!gateway.includePendingMutationsFrom(entity)) {
                    throw std::runtime_error("Scalar graph composition failed");
                }
                if (mode == "stale_root" || mode == "stale_child") {
                    execute_sql(database,
                                mode == "stale_root"
                                    ? "UPDATE gateway_data SET version=version+1 WHERE id=1"
                                    : "UPDATE reading_data SET version=version+1 WHERE id=7");
                }
                if (mode == "fail_root" || mode == "fail_child") {
                    execute_sql(
                        database,
                        mode == "fail_root"
                            ? "CREATE TRIGGER fixture_failure BEFORE UPDATE ON gateway_data BEGIN SELECT RAISE(ABORT,'fixture root update failed'); END"
                            : "CREATE TRIGGER fixture_failure BEFORE UPDATE ON reading_data BEGIN SELECT RAISE(ABORT,'fixture child update failed'); END");
                }
                output.emplace("outcome",
                               save(user_context, gateway, "Save typed scalar aggregate"));
            }
        }
    }
    Json::Array calls(resource), logs(resource);
    for (const auto& entity : ids.calls) {
        calls.emplace_back(Json(resource, entity));
    }
    const auto& sql_logs = user_context.sql_logs();
    for (std::size_t index = log_start; index != sql_logs.size(); ++index) {
        const auto& log = sql_logs[index];
        Json::Object item(resource);
        Json::Array params(resource);
        for (const auto& parameter : log.params) {
            params.emplace_back(parameter.to_json_value(resource));
        }
        item.emplace("sql", Json(resource, log.sql));
        item.emplace("comment", log.comment ? Json(resource, *log.comment) : Json(resource));
        item.emplace("purpose", log.purpose ? Json(resource, *log.purpose) : Json(resource));
        item.emplace("reason",
                     log.audit_reason ? Json(resource, *log.audit_reason) : Json(resource));
        item.emplace("params", Json(resource, std::move(params)));
        logs.emplace_back(Json(resource, std::move(item)));
    }
    output.emplace("fixes", fixes(user_context));
    output.emplace("ids", Json(resource, std::move(calls)));
    output.emplace("plans", Json(resource, reviews));
    output.emplace("events", Json(resource, events));
    output.emplace("logs", Json(resource, std::move(logs)));
    output.emplace("database", database_snapshot(resource, database));
    return Json(resource, std::move(output));
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    for (std::string line; std::getline(std::cin, line);) {
        auto user_context = global.make_user_context();
        {
            auto resource = user_context->memory_resource();
            auto input = Json::parse_result(resource, line);
            std::cout << observe(*user_context, require_value(input)).dump(resource) << '\n';
        }
        user_context.reset();
        if (global.live_user_bytes() != 0) {
            throw std::runtime_error("Governed scalar fixture retains UserContext memory");
        }
    }
}
