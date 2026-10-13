#pragma once
#include <teaql/runtime.hpp>
#include <teaql/runtime/detail/read_hydration.hpp>

namespace json_decoder_fixture {
using namespace teaql;

inline EntityDescriptor descriptor(Memory r) {
    EntityDescriptor d(std::allocator_arg, Allocator(r), "Child");
    d.property("id", DataType::U64).is_id = true;
    d.property("label", DataType::Text);
    d.property("version", DataType::I64).is_version = true;
    for (auto [name, many] : {std::pair("detail", false), std::pair("tags", true)}) {
        auto& relation = d.relation(name, "Leaf");
        relation.local_key = "id";
        relation.foreign_key = "owner";
        relation.many = many;
    }
    return d;
}

template <unsigned K>
struct Model : EntityDefaults<Model<K>> {
    using allocator_type = Allocator;
    static constexpr std::string_view ENTITY_NAME = "Child";
    CompactRow row;
    std::optional<EntityRuntimeState> state;

    Model(std::allocator_arg_t, Allocator a, CompactRow value)
        : row(std::allocator_arg, a, std::move(value)) {}

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
        return descriptor(r);
    }

    static Result<std::optional<FieldLayout>, EntityError> field_layout(Memory r) {
        using R = Result<std::optional<FieldLayout>, EntityError>;
        if constexpr (K == 0) {
            return R::ok(r, std::nullopt);
        }
        if constexpr (K == 3) {
            return R::err(
                r, EntityError(std::allocator_arg, Allocator(r), "Child", "layout fixture failed"));
        }
        const std::array<FieldLayout::Index, 3> indexes{{{"id", 0}, {"label", 1}, {"version", 2}}};
        const std::array<FieldLayout::Mapping, 3> mappings{
            {{"id", "id", "id"}, {"label", "label", "label"}, {"version", "version", "version"}}};
        const std::array<std::string_view, 2> relations{{"detail", "tags"}};
        const std::array<std::string_view, 3> members{{"id", "label", "version"}};
        auto layout = FieldLayout::from_generated(
            r, "Child", "json_decoder_fixture", indexes, mappings, relations, members);
        if (!layout) {
            throw std::logic_error("Invalid field layout fixture");
        }
        return R::ok(r, std::optional<FieldLayout>(std::in_place, std::move(layout).value()));
    }

    static constexpr bool supports_dynamic_property_load() {
        return K >= 2;
    }

    static Result<Model, EntityError> from_compact_row(Memory r, CompactRow row) {
        if (const auto* failed = row.get("fail"); failed && failed->try_bool().value_or(false)) {
            return Result<Model, EntityError>::err(
                r, EntityError(std::allocator_arg, Allocator(r), "Child", "model decode failed"));
        }
        return Result<Model, EntityError>::ok(
            r, Model(std::allocator_arg, Allocator(r), std::move(row)));
    }

    Value id_value() const {
        return row.get("id") ? Value(memory_resource(), *row.get("id")) : Value(memory_resource());
    }

    bool is_field_loaded(std::string_view field) const {
        return row.contains_key(field);
    }

    MutationValues into_values() && {
        MutationValues values(memory_resource());
        values.fields = row.into_map(memory_resource());
        return values;
    }

    void on_loaded(AnyRef input) {
        if (const auto* root = input.get<EntityRuntimeState>()) {
            state.emplace(std::allocator_arg, Allocator(memory_resource()), *root);
            state->set_original_compact_row("Child", row);
        }
    }
};
}
