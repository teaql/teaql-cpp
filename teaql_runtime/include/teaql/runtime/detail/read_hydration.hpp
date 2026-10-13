#pragma once
#include <teaql/runtime/entity_runtime.hpp>
#include <teaql/runtime/resources.hpp>

namespace teaql::detail {
// One bounded decode pass. Arena identities are never reused, so no row payload
// or raw source pointer is retained to protect a shape cache key.
class ReadHydrationShapes {
public:
    explicit ReadHydrationShapes(Memory r) : snapshots_(r) {}

    Result<LoadedSnapshot::Borrow, EntityError> snapshot(Memory r,
                                                         const LoadedSnapshot::Borrow& base,
                                                         const DynamicFieldValues& fields,
                                                         std::string_view entity) {
        auto key = std::pair(base.identity(), fields.selection_identity());
        if (auto found = snapshots_.find(key); found != snapshots_.end()) {
            return Result<LoadedSnapshot::Borrow, EntityError>::ok(r, found->second);
        }
        auto& pool = ContextArena::from_memory(r).loaded_snapshots();
        auto state = LoadedSnapshot::with_dynamic_fields(pool, base, fields);
        if (!state) {
            return Result<LoadedSnapshot::Borrow, EntityError>::err(
                r, EntityError(std::allocator_arg, Allocator(r), entity, state.error()));
        }
        snapshots_.emplace(key, state.value());
        return Result<LoadedSnapshot::Borrow, EntityError>::ok(r, state.value());
    }

private:
    std::pmr::map<std::pair<std::uint64_t, std::uint64_t>, LoadedSnapshot::Borrow> snapshots_;
};

template <Entity T>
Result<T, EntityError> decode_compact_row_with_read_metadata(Memory r,
                                                             CompactRow row,
                                                             const EntityRuntimeState& root,
                                                             ReadHydrationShapes& shapes) {
    using Output = Result<T, EntityError>;
    auto fields = row.take_loaded_dynamic_fields();
    if (fields && !T::supports_dynamic_field_load()) {
        return Output::err(
            r,
            EntityError(std::allocator_arg,
                        Allocator(r),
                        T::ENTITY_NAME,
                        "dynamic fields require a runtime-owned indexed entity carrier"));
    }
    auto entity = T::from_compact_row_with_context(r, std::move(row), AnyRef(root));
    if (!entity || !fields) {
        return entity;
    }
    auto base = entity.value().loaded_state_snapshot();
    if (!base) {
        return Output::err(r,
                           EntityError(std::allocator_arg,
                                       Allocator(r),
                                       T::ENTITY_NAME,
                                       "indexed load state missing during dynamic hydration"));
    }
    auto state = shapes.snapshot(r, *base, *fields, T::ENTITY_NAME);
    if (!state) {
        return Output::err(r, std::move(state).error());
    }
    auto installed = entity.value().install_loaded_dynamic_fields(*fields, state.value());
    if (!installed) {
        return Output::err(r, std::move(installed).error());
    }
    return entity;
}
}

namespace teaql {
template <Entity T>
Result<Vector<T>, EntityError> decode_compact_rows_with_read_metadata(
    Memory r, Vector<CompactRow> rows, const EntityRuntimeState& root) {
    CompactRow::share_layouts(rows);
    Vector<T> entities(r);
    entities.reserve(rows.size());
    detail::ReadHydrationShapes shapes(r);
    for (auto& row : rows) {
        auto decoded =
            detail::decode_compact_row_with_read_metadata<T>(r, std::move(row), root, shapes);
        if (!decoded) {
            return Result<Vector<T>, EntityError>::err(r, std::move(decoded).error());
        }
        entities.emplace_back(std::move(decoded).value());
    }
    return Result<Vector<T>, EntityError>::ok(r, std::move(entities));
}
}
