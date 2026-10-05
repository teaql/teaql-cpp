#pragma once
#include <teaql/runtime/entity_graph_decoder.hpp>
#include <teaql/runtime/behavior.hpp>

namespace decoder_fixture {
using namespace teaql;

// Handwritten model: the same model contract is used by the Rust oracle. No
// generator or entity-runtime algorithms are reproduced in this fixture.
template <unsigned Kind = 0>
struct Item : EntityDefaults<Item<Kind>> {
    using allocator_type = Allocator;
    static constexpr std::string_view ENTITY_NAME = "Item";
    CompactRow row;
    std::optional<EntityRuntimeState> state;

    Item(std::allocator_arg_t, Allocator a, CompactRow value)
        : row(std::allocator_arg, a, std::move(value)) {}

    Item(std::allocator_arg_t, Allocator a, const Item& other)
        : row(std::allocator_arg, a, other.row),
          state(detail::clone_in(a.resource(), other.state)) {}

    Item(std::allocator_arg_t, Allocator a, Item&& other)
        : Item(detail::move_to(a.resource(), std::move(other))) {}

    Item(const Item&) = delete;
    Item(Item&&) noexcept = default;

    Memory memory_resource() const {
        return row.memory_resource();
    }

    static EntityDescriptor entity_descriptor(Memory r) {
        EntityDescriptor e(std::allocator_arg, Allocator(r), Kind == 2 ? "Alias" : "Item");
        e.table(Kind == 1 ? "alternate_items" : "items");
        return e;
    }

    static Result<Item, EntityError> from_compact_row(Memory r, CompactRow row) {
        using R = Result<Item, EntityError>;
        if (auto* v = row.get("fail"); v && v->try_bool().value_or(false))
            return R::err(r,
                          EntityError(std::allocator_arg,
                                      Allocator(r),
                                      ENTITY_NAME,
                                      "handwritten model decode failed"));
        return R::ok(r, Item(std::allocator_arg, Allocator(r), std::move(row)));
    }

    Value id_value() const {
        auto* id = row.get("id");
        return id ? Value(memory_resource(), *id) : Value(memory_resource());
    }

    void on_loaded(AnyRef context) {
        if (auto* root = context.get<EntityRuntimeState>()) {
            state.emplace(std::allocator_arg, Allocator(memory_resource()), *root);
            state->set_original_compact_row(ENTITY_NAME, row);
        }
    }

    MutationValues into_values() && {
        MutationValues values(memory_resource());
        values.fields = row.into_map(memory_resource());
        return values;
    }
};

static_assert(IdentifiableEntity<Item<>>);

struct NamedBehavior : EntityDataServiceBehavior {
    using allocator_type = Allocator;
    String name;

    NamedBehavior(std::allocator_arg_t, Allocator a, std::string_view name)
        : name(name, a.resource()) {}

    Memory memory_resource() const {
        return name.get_allocator().resource();
    }

    Vector<String> relation_loads(Memory r, const UserContext&) const override {
        Vector<String> out(r);
        out.emplace_back(name);
        return out;
    }
};
}
