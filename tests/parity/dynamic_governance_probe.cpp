#include "../support/dynamic_hydration_fixture.hpp"
#include <teaql/runtime/detail/data_service/audited_auto_save.hpp>
#include "fixture_value.hpp"
#include <iostream>

using dynamic_hydration_fixture::School;
using dynamic_hydration_fixture::Executor;

struct Deny final : MutationPolicy {
    using allocator_type = Allocator;
    mutable std::optional<Json> captured;
    Memory resource;

    Deny(std::allocator_arg_t, Allocator a) : resource(a.resource()) {}

    Memory memory_resource() const {
        return resource;
    }

    MutationPolicyIdentity identity(Memory r) const override {
        return MutationPolicyIdentity(
            std::allocator_arg, Allocator(r), "dynamic", "5.0.6", "fixture");
    }

    MutationDecision review(Memory r, const UserContext&, const MutationPlan& plan) const override {
        Json::Object output(r);
        output.emplace(String("request", r), Json(r, plan.request_key));
        output.emplace(String("root", r), Json(r, plan.root_entity_type));
        output.emplace(String("reason", r),
                       plan.audit_reason ? Json(r, *plan.audit_reason) : Json(r));
        Json::Array operations(r);
        for (const auto& operation : plan.operations) {
            Json::Object item(r);
            const auto names = std::array{"Create", "Update", "Delete", "Recover"};
            item.emplace(String("kind", r), Json(r, names[std::size_t(operation.kind)]));
            item.emplace(String("entity", r), Json(r, operation.entity.entity));
            item.emplace(String("id", r), operation.entity.id.to_json_value(r));
            item.emplace(String("version", r),
                         operation.original_version ? Json(r, *operation.original_version)
                                                    : Json(r));
            Json::Object values(r);
            for (const auto& [name, value] : operation.changed_values) {
                values.emplace(String(name, r), value.to_json_value(r));
            }
            item.emplace(String("values", r), Json(r, values));
            operations.emplace_back(item);
        }
        output.emplace(String("operations", r), Json(r, operations));
        captured.emplace(r, output);
        MutationDecision decision(std::allocator_arg, Allocator(r));
        decision.verdict = MutationVerdict::Deny;
        decision.code.emplace("DYNAMIC-DENIED", r);
        decision.message.emplace("extension denied", r);
        decision.field_paths.emplace_back("#title");
        return decision;
    }
};

static School row(Memory r, std::uint64_t id, const DynamicFieldDefinitions& definitions) {
    Record values(r);
    values.emplace(String("id", r), Value(r, id));
    values.emplace(String("name", r), Value(r, "original"));
    values.emplace(String("version", r), Value(r, std::int64_t(3)));
    auto entity = School::from_compact_row(r, CompactRow(std::allocator_arg, Allocator(r), values));
    Value::Object extensions(r);
    extensions.emplace(String("title", r), Value(r, "old"));
    extensions.emplace(String("count", r), Value(r, std::int64_t(9)));
    auto fields = DynamicFieldValues::from_values(r, definitions, extensions);
    if (!detail::install_saved_dynamic_fields(r, entity.value(), &fields.value())) {
        throw std::runtime_error("invalid governance carrier");
    }
    return std::move(entity).value();
}

static Json observe(Memory r, UserContext& user_context, const Json& input) {
    auto& executor = dynamic_hydration_fixture::setup(user_context, nullptr, false);
    user_context.register_executor(user_context, executor);
    const std::array types{DynamicFieldDefinitions::Field{"title", DataType::Text},
                           DynamicFieldDefinitions::Field{"count", DataType::I64}};
    auto& definitions = user_context.make<DynamicFieldDefinitions>(
        std::move(DynamicFieldDefinitions::from_fields(r, "School", "v1", types)).value());
    const std::array defs{&definitions};
    auto& provider = user_context.make<DatabaseDynamicFieldsProvider<Executor>>(
        std::move(DatabaseDynamicFieldsProvider<Executor>::create(r, "profile", defs)).value());
    user_context.set_dynamic_fields_provider(user_context, provider);
    auto& policy = user_context.make_resource<Deny>();
    auto& registry = user_context.make<FunctionMutationPolicyRegistry>(
        [&](Memory r, std::string_view) -> std::optional<MutationPolicyBorrow> {
            return MutationPolicyBorrow(
                std::allocator_arg, Allocator(r), policy, detail::resource_arena(r));
        });
    user_context.set_mutation_policy_registry(user_context, registry);
    auto parent = row(r, 7, definitions);
    bool native = std::get<bool>(field(input, "native").data),
         other = std::get<bool>(field(input, "other").data);
    EntityKey parent_key(std::allocator_arg, Allocator(r), "School", Value(r, std::uint64_t(7)));
    if (native) {
        parent.root.set(parent_key, "name", Value(r, "native-pending"));
    }
    auto child = row(r, other ? 8 : 7, definitions);
    EntityKey child_key(
        std::allocator_arg, Allocator(r), "School", Value(r, std::uint64_t(other ? 8 : 7)));
    if (text(field(input, "mode")) == "delete") {
        if (!child.root.delete_dynamic_field(child_key, "title")) {
            throw std::runtime_error("delete intent rejected");
        }
    } else {
        if (!child.root.update_dynamic_field(child_key, "title", value(r, field(input, "value")))) {
            throw std::runtime_error("set intent rejected");
        }
    }
    if (other) {
        parent.root.adopt_mutations_from(child.root);
    } else {
        parent = std::move(child);
        if (native) {
            parent.root.set(parent_key, "name", Value(r, "native-pending"));
        }
    }
    auto root = std::make_obj_using_allocator<EntityRuntimeState>(Allocator(r), parent.root);
    auto& result = save_audited_ledger_entity(Audited<School>(std::allocator_arg,
                                                              Allocator(r),
                                                              std::move(parent),
                                                              "review dynamic input"),
                                              user_context)
                       .wait();
    if (result) {
        throw std::runtime_error("denied governance save succeeded");
    }
    Json::Object output(r);
    output.emplace(String("plan", r), policy.captured ? Json(r, *policy.captured) : Json(r));
    output.emplace(String("error", r), Json(r, result.error().to_string(r)));
    output.emplace(String("pending", r), Json(r, root.has_pending_dynamic_mutations()));
    return Json(r, output);
}

int main() {
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            auto r = user_context->memory_resource();
            auto input = Json::parse(r, line);
            if (!input) {
                throw std::runtime_error("invalid governance fixture");
            }
            std::cout << observe(r, *user_context, *input).dump(r) << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
