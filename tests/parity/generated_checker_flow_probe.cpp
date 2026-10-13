#include "../generated_checkers/include/cpp_checker_fixture.hpp"
#include "fixture_value.hpp"
#include "../support/generated_schema_only_fixture.hpp"
#include <iostream>
#include <sqlite3.h>

using namespace cpp_checker_fixture;

static bool boolean(const Json& input, std::string_view key) {
    return std::get<bool>(field(input, key).data);
}

static std::uint64_t integer(const Json& input) {
    return std::get<std::uint64_t>(input.data);
}

static Json view(Memory r, const Note& entity) {
    Json::Object output(r);
    output.emplace(String("tenant", r), Json(r, entity.tenantId()));
    output.emplace(String("name", r), Json(r, entity.name()));
    output.emplace(String("created_changed", r), Json(r, entity.createdAt() != Timestamp(111)));
    output.emplace(String("updated_changed", r), Json(r, entity.updatedAt() != Timestamp(222)));
    Json::Array loaded(r);
    for (auto name : {"id", "name", "tenant_id", "created_at", "updated_at", "version"}) {
        loaded.emplace_back(entity.is_loaded(name));
    }
    output.emplace(String("loaded", r), Json(r, loaded));
    return Json(r, output);
}

struct Directive {
    bool reject, fix;
};

class Logic final : public NoteCheckerLogic {
public:
    using allocator_type = Allocator;
    mutable Vector<Json> hooks;

    Logic(std::allocator_arg_t, Allocator a)
        : NoteCheckerLogic(std::allocator_arg, a), hooks(a.resource()) {}

    void checkAndFixNote(const UserContext& user_context,
                         Note& entity,
                         CheckObjectStatus,
                         const ObjectLocation& location,
                         CheckResults& results) const override {
        auto r = user_context.memory_resource();
        Json::Object hook(r);
        hook.emplace(String("view", r), view(r, entity));
        hook.emplace(String("before_logic", r), Json(r, std::uint64_t(results.size())));
        hook.emplace(String("location", r), Json(r, location.model_path(r)));
        hooks.emplace_back(hook);
        const auto& directive = user_context.require_resource<Directive>().value().get();
        requiredText(entity.name(), "name", location, results);
        if (directive.reject) {
            required(false, "custom_field", location, results);
        }
        if (directive.fix) {
            entity.updateName("custom fixed");
        }
    }
};

class Deny final : public MutationPolicy {
public:
    using allocator_type = Allocator;
    Memory resource;
    mutable std::optional<Json> captured;
    bool allow;

    Deny(std::allocator_arg_t, Allocator a, bool permit = false)
        : resource(a.resource()), allow(permit) {}

    Memory memory_resource() const {
        return resource;
    }

    MutationPolicyIdentity identity(Memory r) const override {
        return MutationPolicyIdentity(
            std::allocator_arg, Allocator(r), "checker-flow", "5.0.6", "fixture");
    }

    MutationDecision review(Memory r, const UserContext&, const MutationPlan& plan) const override {
        Json::Object output(r);
        output.emplace(String("request", r), Json(r, plan.request_key));
        output.emplace(String("root", r), Json(r, plan.root_entity_type));
        output.emplace(String("reason", r),
                       plan.audit_reason ? Json(r, *plan.audit_reason) : Json(r));
        Json::Array operations(r);
        for (const auto& operation : plan.operations) {
            Json::Object item(r), values(r);
            for (const auto& [name, value] : operation.changed_values) {
                if (name != "created_at" && name != "updated_at") {
                    values.emplace(String(name, r), value.to_json_value(r));
                }
            }
            const auto names = std::array{"Create", "Update", "Delete", "Recover"};
            item.emplace(String("kind", r), Json(r, names[std::size_t(operation.kind)]));
            item.emplace(String("entity", r), Json(r, operation.entity.entity));
            item.emplace(String("id", r), operation.entity.id.to_json_value(r));
            item.emplace(String("version", r),
                         operation.original_version ? Json(r, *operation.original_version)
                                                    : Json(r));
            item.emplace(String("values", r), Json(r, values));
            Json::Array clock(r);
            auto a = operation.changed_values.find("created_at"),
                 b = operation.changed_values.find("updated_at");
            clock.emplace_back(a != operation.changed_values.end());
            clock.emplace_back(b != operation.changed_values.end());
            item.emplace(String("clock_fields", r), Json(r, clock));
            item.emplace(String("common_clock", r),
                         a != operation.changed_values.end() && b != operation.changed_values.end()
                             ? Json(r, a->second == b->second)
                             : Json(r));
            operations.emplace_back(item);
        }
        output.emplace(String("operations", r), Json(r, operations));
        captured.emplace(r, output);
        MutationDecision decision(std::allocator_arg, Allocator(r));
        if (allow) {
            return decision;
        }
        decision.verdict = MutationVerdict::Deny;
        decision.code.emplace("CHECKER-DENIED", r);
        decision.message.emplace("preflight observed", r);
        decision.field_paths.emplace_back("name");
        return decision;
    }
};

static void execute_fixture(sqlite3* database, const char* sql) {
    char* message = nullptr;
    auto status = sqlite3_exec(database, sql, nullptr, nullptr, &message);
    if (status != SQLITE_OK) {
        std::string error = message ? message : sqlite3_errmsg(database);
        sqlite3_free(message);
        throw std::runtime_error(error);
    }
}

static Json database_view(Memory r, sqlite3* database) {
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(
            database,
            "SELECT id,name,tenant,version,created_at=111,updated_at=222,created_at=updated_at FROM note_data ORDER BY id",
            -1,
            &statement,
            nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
    std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> cleanup(statement, sqlite3_finalize);
    Json::Array rows(r);
    int status;
    while ((status = sqlite3_step(statement)) == SQLITE_ROW) {
        Json::Object item(r);
        item.emplace(String("id", r), Json(r, sqlite3_column_int64(statement, 0)));
        item.emplace(
            String("name", r),
            Json(r,
                 std::string_view(reinterpret_cast<const char*>(sqlite3_column_text(statement, 1)),
                                  sqlite3_column_bytes(statement, 1))));
        item.emplace(String("tenant", r), Json(r, sqlite3_column_int64(statement, 2)));
        item.emplace(String("version", r), Json(r, sqlite3_column_int64(statement, 3)));
        item.emplace(String("created_old", r), Json(r, bool(sqlite3_column_int(statement, 4))));
        item.emplace(String("updated_old", r), Json(r, bool(sqlite3_column_int(statement, 5))));
        item.emplace(String("common_clock", r), Json(r, bool(sqlite3_column_int(statement, 6))));
        rows.emplace_back(item);
    }
    if (status != SQLITE_DONE) {
        throw std::runtime_error(sqlite3_errmsg(database));
    }
    return Json(r, rows);
}

static Json observe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    const bool commit = text(field(input, "mode")) == "commit";
    auto status_name = text(field(input, "status"));
    auto status = status_name == "create"   ? CheckObjectStatus::Create
                  : status_name == "update" ? CheckObjectStatus::Update
                                            : CheckObjectStatus::Unknown;
    if (!std::holds_alternative<std::monostate>(field(input, "root").data)) {
        user_context.with_active_root("Tenant", integer(field(input, "root")));
    }
    auto& directive = user_context.make<Directive>(boolean(input, "reject"), boolean(input, "fix"));
    user_context.insert_resource(user_context, directive);
    auto& logic = user_context.make<Logic>();
    auto& checker = user_context.make<NoteChecker>(logic);
    Record row(r);
    row.emplace(String("id", r), Value(r, std::uint64_t(7)));
    row.emplace(String("name", r),
                Value(r,
                      commit && status == CheckObjectStatus::Update ? "stored"
                                                                    : text(field(input, "name"))));
    row.emplace(String("tenant_id", r), Value(r, integer(field(input, "tenant"))));
    row.emplace(String("created_at", r), Value(r, Timestamp(111)));
    row.emplace(String("updated_at", r), Value(r, Timestamp(222)));
    row.emplace(String("version", r),
                Value(r, std::int64_t(status == CheckObjectStatus::Create ? 0 : 3)));
    auto decoded = Note::from_compact_row(r, CompactRow(std::allocator_arg, Allocator(r), row));
    auto& entity = commit && status == CheckObjectStatus::Create
                       ? user_context.make<Note>()
                       : user_context.make<Note>(decoded.value());
    if (commit) {
        if (status == CheckObjectStatus::Create) {
            entity.updateId(7)
                .updateVersion(0)
                .updateCreatedAt(Timestamp(111))
                .updateUpdatedAt(Timestamp(222));
        }
        entity.updateName(text(field(input, "name")));
    }
    Vector<std::string_view> fields(r);
    auto loaded = text(field(input, "loaded"));
    for (auto name : {"id", "name", "tenant_id", "created_at", "updated_at", "version"}) {
        if ((loaded == "identity" && name != std::string_view("id") &&
             name != std::string_view("version")) ||
            (loaded == "no_times" &&
             (name == std::string_view("created_at") || name == std::string_view("updated_at"))) ||
            (loaded == "no_tenant" && name == std::string_view("tenant_id"))) {
            continue;
        }
        fields.emplace_back(name);
    }
    entity.load_state = eval::LoadState::partial(r, fields)
                            .into_indexed(detail::ContextArena::from_memory(r).loaded_snapshots(),
                                          Note::field_layout(r))
                            .value();
    if (status == CheckObjectStatus::Create) {
        entity.root.mark_as_new(entity.entity_key());
    }
    auto& policy = user_context.make<Deny>(commit);
    CheckResults errors(r);
    std::optional<String> error;
    std::optional<std::string_view> error_kind;
    std::optional<Json> saved, db_before;
    sqlite3* database = nullptr;
    if (text(field(input, "mode")) == "direct") {
        auto location = ObjectLocation::array_root(r, 2);
        location.member("note_info");
        checker.check_and_fix_typed(user_context, entity, status, location, errors);
    } else {
        auto& module = user_context.make<RuntimeModule>();
        module.entity<Tenant>();
        module.entity<Note>();
        auto executor =
            teaql::test::configure_schema_only<cpp_checker_fixture::DataServiceExecutor>(
                user_context, module);
        if (commit) {
            database = user_context.get(executor).transport.connection().native_handle();
            execute_fixture(
                database,
                "INSERT INTO tenant_data(id,name,version) VALUES(42,'trusted',3),(43,'other',3);");
            if (status == CheckObjectStatus::Update) {
                execute_fixture(
                    database,
                    "INSERT INTO note_data(id,name,tenant,created_at,updated_at,version) VALUES(7,'stored',42,111,222,3);");
            }
            if (text(field(input, "fault")) == "trigger") {
                execute_fixture(
                    database,
                    status == CheckObjectStatus::Create
                        ? "CREATE TRIGGER fixture_failure BEFORE INSERT ON note_data BEGIN SELECT RAISE(ABORT,'fixture write failed'); END;"
                        : "CREATE TRIGGER fixture_failure BEFORE UPDATE ON note_data BEGIN SELECT RAISE(ABORT,'fixture write failed'); END;");
            }
            if (text(field(input, "fault")) == "conflict" && status == CheckObjectStatus::Update) {
                execute_fixture(database, "UPDATE note_data SET version=4 WHERE id=7;");
            }
            db_before.emplace(database_view(r, database));
        }
        auto& registry = user_context.make<InMemoryCheckerRegistry>();
        auto& adapter =
            user_context.make<TypedEntityChecker<Note, NoteChecker>>(user_context, checker);
        registry.register_checker(user_context, adapter);
        user_context.set_checker_registry(user_context, registry);
        auto& policies = user_context.make<FunctionMutationPolicyRegistry>(
            [&](Memory owner, std::string_view) -> std::optional<MutationPolicyBorrow> {
                return MutationPolicyBorrow(
                    std::allocator_arg, Allocator(owner), policy, detail::resource_arena(r));
            });
        user_context.set_mutation_policy_registry(user_context, policies);
        auto& copied = user_context.make<Note>(entity);
        auto& result = AuditedSaveExt<Note>::save(
                           std::move(copied).audit_as("Observe generated fixes"), user_context)
                           .wait();
        if (result) {
            if (!commit) {
                throw std::runtime_error("policy must prevent writes");
            }
            Json::Object item(r);
            item.emplace(String("view", r), view(r, result.value()));
            item.emplace(String("version", r), Json(r, result.value().version()));
            item.emplace(String("common_clock", r),
                         Json(r, result.value().createdAt() == result.value().updatedAt()));
            saved.emplace(r, item);
        } else if (result.error().kind() == RuntimeError::Kind::Check) {
            errors = detail::clone_in(r, result.error().check_results());
            error.emplace("Check", r);
            error_kind = "Check";
        } else {
            switch (result.error().kind()) {
            case RuntimeError::Kind::Graph:
                error_kind = "Graph";
                break;
            case RuntimeError::Kind::OptimisticLockConflict:
                error_kind = "OptimisticLockConflict";
                break;
            case RuntimeError::Kind::Transaction:
                error_kind = "Transaction";
                break;
            case RuntimeError::Kind::Schema:
                error_kind = "Schema";
                break;
            case RuntimeError::Kind::Policy:
                error_kind = "Policy";
                break;
            case RuntimeError::Kind::Event:
                error_kind = "Event";
                break;
            case RuntimeError::Kind::SqlCompile:
                error_kind = "SqlCompile";
                break;
            default:
                error_kind = "Other";
                break;
            }
            auto message = result.error().to_string(r);
            if (commit && result.error().kind() == RuntimeError::Kind::OptimisticLockConflict) {
                error.emplace("OptimisticLockConflict", r);
            } else if (commit && message.find("fixture write failed") != String::npos) {
                error.emplace("FixtureWriteFailure", r);
            } else {
                error.emplace(std::move(message));
            }
        }
    }
    Json::Array diagnostics(r), evidence(r), hooks(r);
    for (auto profile : {JsonFieldNamingProfile::CamelCase,
                         JsonFieldNamingProfile::SnakeCase,
                         JsonFieldNamingProfile::PascalCase}) {
        Json::Array results(r);
        for (const auto& result : errors) {
            results.emplace_back(result.to_wire(r, profile).to_json(r));
        }
        diagnostics.emplace_back(std::move(results));
    }
    for (const auto& value : user_context.last_fix_evidence()) {
        Json::Object item(r);
        item.emplace(String("entity", r), Json(r, value.entity_type));
        item.emplace(String("path", r), Json(r, value.model_path));
        item.emplace(String("source", r),
                     Json(r, value.source == FixEvidenceSource::Clock ? "Clock" : "Context"));
        item.emplace(String("label", r), Json(r, value.source_label));
        evidence.emplace_back(item);
    }
    for (const auto& hook : logic.hooks) {
        hooks.emplace_back(detail::clone_in(r, hook));
    }
    Json::Object output(r);
    output.emplace(String("view", r), view(r, entity));
    output.emplace(String("hooks", r), Json(r, hooks));
    output.emplace(String("diagnostics", r), Json(r, diagnostics));
    output.emplace(String("error", r), error ? Json(r, *error) : Json(r));
    output.emplace(String("plan", r),
                   policy.captured ? detail::clone_in(r, *policy.captured) : Json(r));
    output.emplace(String("evidence", r), Json(r, evidence));
    if (commit) {
        output.emplace(String("error_kind", r), error_kind ? Json(r, *error_kind) : Json(r));
        output.emplace(String("saved", r), saved ? detail::clone_in(r, *saved) : Json(r));
        output.emplace(String("db_before", r), detail::clone_in(r, *db_before));
        output.emplace(String("db_after", r), database_view(r, database));
        auto changes = entity.root.current_change_set();
        FieldSet fields(r);
        for (const auto& [key, values] : changes.changes()) {
            for (const auto& [name, value] : values) {
                fields.emplace(String(name, r));
            }
        }
        Json::Array pending(r);
        for (const auto& field : fields) {
            pending.emplace_back(field);
        }
        output.emplace(String("pending_fields", r), Json(r, pending));
        output.emplace(String("new_keys", r),
                       Json(r, std::uint64_t(entity.root.new_keys().size())));
    }
    return Json(r, output);
}

int main() {
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            auto input = Json::parse(user_context->memory_resource(), line);
            if (!input) {
                throw std::runtime_error("invalid Checker flow fixture");
            }
            std::cout << observe(*user_context, *input).dump(user_context->memory_resource())
                      << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
