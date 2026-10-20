#pragma once
#include <teaql/runtime/entity_graph_decoder.hpp>
#include <teaql/runtime/behavior.hpp>
#include <array>

namespace indexed_query_fixture {
using namespace teaql;

// Handwritten indexed carrier for caller qualification. Layout construction,
// snapshot merging and dynamic value installation delegate to the runtime.
struct Item : EntityDefaults<Item> {
    using allocator_type = Allocator;
    static constexpr std::string_view ENTITY_NAME = "Item";
    CompactRow row;
    std::optional<EntityRuntimeState> state;
    eval::LoadState load_state;

    Item(std::allocator_arg_t, Allocator a, CompactRow value)
        : row(std::allocator_arg, a, std::move(value)),
          load_state(row.indexed_load_state(
              detail::ContextArena::from_memory(a.resource()).loaded_snapshots(),
              field_layout(a.resource()))) {}

    Item(std::allocator_arg_t, Allocator a, const Item& other)
        : row(std::allocator_arg, a, other.row), state(detail::clone_in(a.resource(), other.state)),
          load_state(std::allocator_arg, a, other.load_state) {}

    Item(std::allocator_arg_t, Allocator a, Item&& other)
        : Item(detail::move_to(a.resource(), std::move(other))) {}

    Item(const Item&) = delete;
    Item(Item&&) noexcept = default;

    Memory memory_resource() const {
        return row.memory_resource();
    }

    static EntityDescriptor entity_descriptor(Memory r) {
        EntityDescriptor e(std::allocator_arg, Allocator(r), "Item");
        e.table("items");
        return e;
    }

    static Result<Item, EntityError> from_compact_row(Memory r, CompactRow row) {
        using R = Result<Item, EntityError>;
        if (auto* v = row.get("fail"); v && v->try_bool().value_or(false)) {
            return R::err(r,
                          EntityError(std::allocator_arg,
                                      Allocator(r),
                                      ENTITY_NAME,
                                      "handwritten model decode failed"));
        }
        return R::ok(r, Item(std::allocator_arg, Allocator(r), std::move(row)));
    }

    static constexpr bool supports_dynamic_field_load() {
        return true;
    }

    static Result<FieldLayout, String> create_field_layout(Memory r) {
        const std::array<FieldLayout::Index, 2> indexes{{{"id", 0}, {"version", 1}}};
        const std::array<FieldLayout::Mapping, 2> mappings{
            {{"id", "id", "id"}, {"version", "version", "version"}}};
        const std::array<std::string_view, 2> members{{"id", "version"}};
        return FieldLayout::from_generated(
            r, ENTITY_NAME, "indexed-query-v1", indexes, mappings, {}, members);
    }

    static const FieldLayout& field_layout(Memory r) {
        return detail::ContextArena::from_memory(r).field_layout<Item>();
    }

    std::optional<LoadedSnapshot::Borrow> loaded_state_snapshot() const {
        auto* snapshot = load_state.indexed_snapshot();
        return snapshot ? std::optional(*snapshot) : std::nullopt;
    }

    Result<std::monostate, EntityError>
    install_loaded_dynamic_fields(const DynamicFieldValues& fields,
                                  LoadedSnapshot::Borrow snapshot) {
        state->install_loaded_dynamic_fields(fields);
        load_state = eval::LoadState::indexed(memory_resource(), snapshot);
        return Result<std::monostate, EntityError>::ok(memory_resource(), {});
    }

    const DynamicFieldValues* dynamic_field_values() const {
        return state ? state->loaded_dynamic_fields() : nullptr;
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

}
