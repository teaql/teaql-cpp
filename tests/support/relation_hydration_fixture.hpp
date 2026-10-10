#pragma once
#include <teaql/runtime/detail/data_service/relation_hydration.hpp>

namespace hydration_fixture {
using namespace teaql;

template <unsigned K>
struct Model : EntityDefaults<Model<K>> {
    using allocator_type = Allocator;
    static constexpr std::string_view ENTITY_NAME = K == 0 ? "Parent" : K == 1 ? "Child" : "Leaf";
    CompactRow row;
    std::optional<EntityRuntimeState> state;

    Model(std::allocator_arg_t, Allocator a, CompactRow row)
        : row(std::allocator_arg, a, std::move(row)) {}

    Model(std::allocator_arg_t, Allocator a, const Model& other)
        : row(std::allocator_arg, a, other.row),
          state(detail::clone_in(a.resource(), other.state)) {}

    Model(std::allocator_arg_t, Allocator a, Model&& other)
        : Model(detail::move_to(a.resource(), std::move(other))) {}

    Model(const Model&) = delete;
    Model(Model&&) noexcept = default;

    Memory memory_resource() const {
        return row.memory_resource();
    }

    static EntityDescriptor entity_descriptor(Memory r) {
        EntityDescriptor e(std::allocator_arg, Allocator(r), ENTITY_NAME);
        e.property("id", DataType::U64).is_id = true;
        return e;
    }

    static Result<Model, EntityError> from_compact_row(Memory r, CompactRow row) {
        using R = Result<Model, EntityError>;
        if (auto* fail = row.get("fail"); fail && fail->try_bool().value_or(false)) {
            return R::err(
                r,
                EntityError(std::allocator_arg, Allocator(r), ENTITY_NAME, "model decode failed"));
        }
        return R::ok(r, Model(std::allocator_arg, Allocator(r), std::move(row)));
    }

    Value id_value() const {
        auto* id = row.get("id");
        return id ? Value(memory_resource(), *id) : Value(memory_resource());
    }

    MutationValues into_values() && {
        MutationValues values(memory_resource());
        values.fields = row.into_map(memory_resource());
        return values;
    }

    void on_loaded(AnyRef context) {
        if (auto* root = context.get<EntityRuntimeState>()) {
            state.emplace(std::allocator_arg, Allocator(memory_resource()), *root);
            state->set_original_compact_row(ENTITY_NAME, row);
        }
    }
};

struct UnusedExecutor : QueryExecutor<String>, MutationExecutor<String> {
    using allocator_type = Allocator;
    Memory r;
    bool probes = false;
    mutable unsigned capability_calls = 0;

    UnusedExecutor(std::allocator_arg_t, Allocator a) : r(a.resource()) {}

    Memory memory_resource() const {
        return r;
    }

    DataServiceCapabilities capabilities() const override {
        ++capability_calls;
        DataServiceCapabilities c;
        c.small_parent_relation_probes = probes;
        return c;
    }

    DataServiceOperationResult<QueryResult, String>& query(Memory, QueryRequest) override {
        throw std::logic_error("hydration bypassed entity fetch");
    }

    DataServiceOperationResult<MutationResult, String>& mutate(Memory, MutationRequest) override {
        throw std::logic_error("unexpected mutation");
    }
};
}
