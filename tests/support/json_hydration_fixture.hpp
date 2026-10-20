#pragma once
#include <teaql/runtime.hpp>
#include <teaql/runtime/detail/read_hydration.hpp>

namespace json_hydration_fixture {
using namespace teaql;

inline EntityDescriptor descriptor(Memory r) {
    EntityDescriptor d(std::allocator_arg, Allocator(r), "JsonNode");
    d.property("id", DataType::U64).is_id = true;
    d.property("label", DataType::Text).column_name = "stored_label";
    d.property("version", DataType::I64).is_version = true;
    d.property("count", DataType::I64);
    d.property("active", DataType::Bool);
    d.property("ratio", DataType::F64);
    d.property("price", DataType::Decimal);
    d.property("birthday", DataType::Date);
    d.property("happened_at", DataType::Timestamp);
    d.property("raw", DataType::Json);
    d.property("parent_id", DataType::U64);
    d.property("nonnull", DataType::Text).nullable = false;
    d.property("fail", DataType::Bool);
    for (auto [name, many] :
         {std::pair("parent", false), std::pair("alternate", false), std::pair("children", true)}) {
        auto& relation = d.relation(name, "JsonNode");
        relation.local_key = many ? "id" : "parent_id";
        relation.foreign_key = many ? "parent_id" : "id";
        relation.many = many;
    }
    return d;
}

template <unsigned K>
struct Model : EntityDefaults<Model<K>> {
    using allocator_type = Allocator;
    static constexpr std::string_view ENTITY_NAME = "JsonNode";
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
                r,
                EntityError(std::allocator_arg, Allocator(r), "JsonNode", "layout fixture failed"));
        }
        constexpr std::array<std::string_view, 13> names{"id",
                                                         "label",
                                                         "version",
                                                         "count",
                                                         "active",
                                                         "ratio",
                                                         "price",
                                                         "birthday",
                                                         "happened_at",
                                                         "raw",
                                                         "parent_id",
                                                         "nonnull",
                                                         "fail"};
        Vector<FieldLayout::Index> indexes(r);
        Vector<FieldLayout::Mapping> mappings(r);
        for (std::size_t i = 0; i < names.size(); ++i) {
            indexes.emplace_back(names[i], i);
            mappings.emplace_back(
                names[i], names[i], names[i] == "label" ? "stored_label" : names[i]);
        }
        constexpr std::array<std::string_view, 3> relations{"parent", "alternate", "children"};
        const auto& members = names;
        auto layout = FieldLayout::from_generated(
            r, "JsonNode", "json_hydration_fixture", indexes, mappings, relations, members);
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
                r,
                EntityError(std::allocator_arg, Allocator(r), "JsonNode", "model decode failed"));
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
            state->set_original_compact_row("JsonNode", row);
        }
    }
};
}
