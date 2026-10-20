#include "../generated_inheritance_variants/include/cpp_inheritance_variants.hpp"
#include "fixture_value.hpp"
#include "../support/generated_schema_only_fixture.hpp"
#include <iostream>
#include <sqlite3.h>

using namespace cpp_inheritance_variants;
using Note = TEAQL_CHECK_MODEL;
using NoteChecker = TEAQL_CHECK_CHECKER;
using NoteCheckerLogic = TEAQL_CHECK_LOGIC;

static bool boolean(const Json& input, std::string_view key) {
    return std::get<bool>(field(input, key).data);
}

static std::uint64_t integer(const Json& input) {
    return std::get<std::uint64_t>(input.data);
}

template <class T, class F>
static Json state(Memory r, const eval::EvalResult<T>& result, F&& encode) {
    Json::Object output(r);
    if (result.is_value()) {
        output.emplace(String("state", r), Json(r, "Value"));
        output.emplace(String("value", r), encode(result.get()));
    } else if (result.is_null()) {
        output.emplace(String("state", r), Json(r, "Null"));
    } else {
        output.emplace(String("state", r), Json(r, "NotLoaded"));
        output.emplace(String("node", r), Json(r, result.missing().failed_node));
        output.emplace(String("path", r), Json(r, result.missing().attempted_path));
    }
    return Json(r, output);
}

static Json changed(Memory r, const std::optional<Value>& value) {
    Json::Object output(r);
    output.emplace(String("present", r), Json(r, bool(value)));
    output.emplace(String("value", r), value ? value->to_json_value(r) : Json(r));
    return Json(r, output);
}

static Json view(Memory r, const Note& entity) {
    Json::Object output(r);
    output.emplace(String("tenant", r), Json(r, entity.tenantId()));
    output.emplace(String("name", r), Json(r, entity.name()));
    output.emplace(String("created", r), Json(r, entity.createdAt().as_millis()));
    output.emplace(String("updated", r), Json(r, entity.updatedAt().as_millis()));
    output.emplace(String("active", r), Json(r, entity.active()));
    output.emplace(String("note", r), entity.note() ? Json(r, *entity.note()) : Json(r));
    output.emplace(String("changed_name", r), changed(r, entity.changedName()));
    output.emplace(String("changed_note", r), changed(r, entity.changedNote()));
    output.emplace(String("changed_tenant", r), changed(r, entity.changedTenantId()));
    output.emplace(String("eval_name", r), state(r, entity.evalName(), [&](const auto& v) {
                       return Json(r, v);
                   }));
    output.emplace(String("eval_note", r), state(r, entity.evalNote(), [&](const auto& v) {
                       return v ? Json(r, *v) : Json(r);
                   }));
    output.emplace(String("eval_tenant", r), state(r, entity.evalTenantId(), [&](auto v) {
                       return Json(r, v);
                   }));
    Json::Array loaded(r);
    for (auto name : {"id",
                      "name",
                      "tenant_id",
                      "created_at",
                      "updated_at",
                      "version",
                      "active",
                      "note",
                      "amount",
                      "status",
                      "reference"}) {
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

    void TEAQL_CHECK_CALLBACK(const UserContext& user_context,
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

static Json observe(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
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
    row.emplace(String("name", r), Value(r, text(field(input, "name"))));
    row.emplace(String("tenant_id", r), Value(r, integer(field(input, "tenant"))));
    row.emplace(String("created_at", r), Value(r, Timestamp(111)));
    row.emplace(String("updated_at", r), Value(r, Timestamp(222)));
    row.emplace(String("version", r),
                Value(r, std::int64_t(status == CheckObjectStatus::Create ? 0 : 3)));
    row.emplace(String("active", r), Value(r, true));
    row.emplace(String("note", r), Value(r, "stored note"));
    row.emplace(String("amount", r), Value(r, Decimal::parse("12.50").value()));
    row.emplace(String("status", r), Value(r, "open"));
    row.emplace(String("reference", r), Value(r, "ref"));
    auto decoded = Note::from_compact_row(r, CompactRow(std::allocator_arg, Allocator(r), row));
    auto& entity = user_context.make<Note>(decoded.value());
    Vector<std::string_view> fields(r);
    auto loaded = text(field(input, "loaded"));
    for (auto name : {"id",
                      "name",
                      "tenant_id",
                      "created_at",
                      "updated_at",
                      "version",
                      "active",
                      "note",
                      "amount",
                      "status",
                      "reference"}) {
        if ((loaded == "identity" && name != std::string_view("id") &&
             name != std::string_view("version")) ||
            (loaded == "no_times" &&
             (name == std::string_view("created_at") || name == std::string_view("updated_at"))) ||
            (loaded == "no_tenant" && name == std::string_view("tenant_id"))) {
            continue;
        }
        if (Note::field_layout(r).index(name).has_value()) {
            fields.emplace_back(name);
        }
    }
    entity.load_state = eval::LoadState::partial(r, fields)
                            .into_indexed(detail::ContextArena::from_memory(r).loaded_snapshots(),
                                          Note::field_layout(r))
                            .value();
    if (status == CheckObjectStatus::Create) {
        entity.root.mark_as_new(entity.entity_key());
    }
    auto& policy = user_context.make<Deny>(false);
    CheckResults errors(r);
    std::optional<String> error;
    auto before = view(r, entity);
    auto& copy = user_context.make<Note>(entity);
    auto op = text(field(input, "op"));
    if (op == "update") {
        entity.updateName("  updated  ").updateNote(std::nullopt).updateActive(false);
    } else if (op == "overlay") {
        entity.root.set(entity.entity_key(), "name", Value(r, "  overlay  "));
        entity.root.set(entity.entity_key(), "note", Value(r));
    } else if (op == "self") {
        entity.updateName(entity.name());
        entity.updateNote(entity.note());
    }
    if (text(field(input, "mode")) == "direct") {
        auto location = ObjectLocation::array_root(r, 2);
        location.member("note_info");
        checker.check_and_fix_typed(user_context, entity, status, location, errors);
    } else {
        auto& module = user_context.make<RuntimeModule>();
        module.entity<Tenant>();
        module.entity<Note>();
        auto executor =
            teaql::test::configure_schema_only<cpp_inheritance_variants::DataServiceExecutor>(
                user_context, module);
        auto& registry = user_context.make<InMemoryCheckerRegistry>();
        auto& adapter =
            user_context.make<TypedEntityChecker<Note, NoteChecker>>(user_context, checker);
        registry.register_checker(user_context, adapter);
        user_context.set_checker_registry(user_context, registry);
        auto& policies = user_context.make<FunctionMutationPolicyRegistry>(
            [r, &policy](Memory owner, std::string_view) -> std::optional<MutationPolicyBorrow> {
                return MutationPolicyBorrow(
                    std::allocator_arg, Allocator(owner), policy, detail::resource_arena(r));
            });
        user_context.set_mutation_policy_registry(user_context, policies);
        auto& copied = user_context.make<Note>(entity);
        auto& result = AuditedSaveExt<Note>::save(
                           std::move(copied).audit_as("Observe inherited fixes"), user_context)
                           .wait();
        if (result) {
            throw std::runtime_error("policy must prevent writes");
        }
        if (result.error().kind() == RuntimeError::Kind::Check) {
            errors = detail::clone_in(r, result.error().check_results());
            error.emplace("Check", r);
        } else {
            error.emplace(result.error().to_string(r));
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
    output.emplace(String("wall_millis", r), Json(r, Timestamp::now().as_millis()));
    output.emplace(String("before", r), before);
    output.emplace(String("copy", r), view(r, copy));
    output.emplace(String("view", r), view(r, entity));
    output.emplace(String("hooks", r), Json(r, hooks));
    output.emplace(String("diagnostics", r), Json(r, diagnostics));
    output.emplace(String("error", r), error ? Json(r, *error) : Json(r));
    output.emplace(String("plan", r),
                   policy.captured ? detail::clone_in(r, *policy.captured) : Json(r));
    output.emplace(String("evidence", r), Json(r, evidence));
    return Json(r, output);
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            Json retained(global.memory_resource());
            {
                auto input = Json::parse(user_context->memory_resource(), line);
                if (!input) {
                    throw std::runtime_error("invalid inherited Checker fixture");
                }
                auto result = observe(*user_context, *input);
                retained = detail::clone_in(global.memory_resource(), result);
            }
            user_context.reset();
            std::cout << retained.dump(global.memory_resource()) << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
