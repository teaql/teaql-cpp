#pragma once
#include "runtime_mutation_fixture.hpp"

namespace audited_fixture {
using namespace teaql;

struct Model : EntityDefaults<Model> {
    using allocator_type = Allocator;
    static constexpr std::string_view ENTITY_NAME = "Order";
    MutationValues values;
    std::optional<EntityRuntimeState> runtime;
    std::optional<String> comment;
    std::optional<FieldSet> loaded, dirty;
    std::optional<EntitySnapshot> original;
    bool fresh = false, removed = false, restored = false;

    Model(std::allocator_arg_t, Allocator a) : values(a.resource()) {}

    Model(std::allocator_arg_t, Allocator a, const Model& other)
        : values(std::allocator_arg, a, other.values),
          runtime(detail::clone_in(a.resource(), other.runtime)),
          comment(detail::clone_in(a.resource(), other.comment)),
          loaded(detail::clone_in(a.resource(), other.loaded)),
          dirty(detail::clone_in(a.resource(), other.dirty)),
          original(detail::clone_in(a.resource(), other.original)), fresh(other.fresh),
          removed(other.removed), restored(other.restored) {}

    Model(const Model&) = delete;
    Model(Model&&) noexcept = default;

    Memory memory_resource() const {
        return values.memory_resource();
    }

    static EntityDescriptor entity_descriptor(Memory r) {
        EntityDescriptor d(std::allocator_arg, Allocator(r), ENTITY_NAME);
        d.property("id", DataType::U64).id();
        d.property("version", DataType::I64).version();
        d.property("name", DataType::Text);
        return d;
    }

    static Result<Model, EntityError> from_compact_row(Memory r, CompactRow row) {
        if (auto* name = row.get("name"); name && name->try_text() == "decode error") {
            return Result<Model, EntityError>::err(
                r, EntityError(std::allocator_arg, Allocator(r), "Order", "decode failed"));
        }
        Model m(std::allocator_arg, Allocator(r));
        m.values.fields = row.into_map(r);
        return Result<Model, EntityError>::ok(r, std::move(m));
    }

    MutationValues into_values() && {
        return std::move(values);
    }

    std::optional<EntityRuntimeState> entity_runtime_state() const {
        return detail::clone_in(memory_resource(), runtime);
    }

    void set_comment(std::string_view reason) {
        comment.emplace(reason, memory_resource());
        if (runtime) {
            runtime->set_comment(reason);
        }
    }

    std::optional<String> get_comment() const {
        return detail::clone_in(memory_resource(), comment);
    }

    bool is_field_loaded(std::string_view field) const {
        return !loaded || loaded->contains(field);
    }

    std::optional<FieldSet> dirty_fields() const {
        return detail::clone_in(memory_resource(), dirty);
    }

    std::optional<EntitySnapshot> original_values() const {
        return detail::clone_in(memory_resource(), original);
    }

    bool is_new() const {
        return fresh;
    }

    bool is_marked_as_delete() const {
        return removed;
    }

    void on_loaded(AnyRef source) {
        if (auto* root = source.get<EntityRuntimeState>()) {
            runtime.emplace(std::allocator_arg, Allocator(memory_resource()), *root);
            restored = true;
        }
    }
};

static_assert(LedgerEntity<Model>);

struct Checker final : teaql::Checker {
    mutation_fixture::State& state;
    std::string_view action;
    mutable unsigned calls = 0;

    Checker(mutation_fixture::State& state, std::string_view action)
        : state(state), action(action) {}

    std::string_view entity() const override {
        return "Order";
    }

    void check_and_fix(const UserContext& context,
                       EntityValues& values,
                       const ObjectLocation& location,
                       CheckResults& results) const override {
        ++calls;
        state.note("checker");
        auto r = values.memory_resource();
        if (action == "fill" || action == "error") {
            auto path = detail::clone_in(r, location).member("name");
            FixEvidence evidence(std::allocator_arg,
                                 Allocator(r),
                                 "Order",
                                 path.model_path(r),
                                 FixEvidenceSource::Context,
                                 "fixture");
            context.record_fix_evidence(evidence);
        }
        if (action == "fill") {
            values.fields.insert_or_assign(String("name", r), Value(r, "fixed"));
        }
        if (action == "error") {
            results.emplace_back(
                CheckResult::required(r, detail::clone_in(r, location).member("name")));
        }
    }
};
}
