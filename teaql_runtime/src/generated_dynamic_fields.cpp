#include <teaql/runtime/generated_dynamic_fields.hpp>

namespace teaql {
Result<LoadedSnapshot::Borrow, DynamicFieldError>
GeneratedDynamicFields::validate(EntityRuntimeState& root,
                                 const eval::LoadState& state,
                                 const FieldLayout& layout,
                                 std::string_view code) {
    auto r = root.memory_resource();
    if (state.memory_resource() != r) {
        throw std::invalid_argument("Generated dynamic carrier must share its Context");
    }
    using R = Result<LoadedSnapshot::Borrow, DynamicFieldError>;
    auto* snapshot = state.indexed_snapshot();
    if (!snapshot) {
        return R::err(r, DynamicFieldError(r, "DYNAMIC_FIELD_INDEXED_STATE_MISSING", code));
    }
    if (&snapshot->get().layout() != &layout) {
        return R::err(r, DynamicFieldError(r, "DYNAMIC_FIELD_LAYOUT_MISMATCH", code));
    }
    return R::ok(r, *snapshot);
}

Result<std::monostate, EntityError>
GeneratedDynamicFields::install(EntityRuntimeState& root,
                                eval::LoadState& state,
                                const FieldLayout& layout,
                                const DynamicFieldValues& fields,
                                LoadedSnapshot::Borrow snapshot,
                                std::string_view entity) {
    auto r = root.memory_resource();
    if (state.memory_resource() != r) {
        throw std::invalid_argument("Generated dynamic carrier must share its Context");
    }
    using R = Result<std::monostate, EntityError>;
    if (&snapshot.get().layout() != &layout || fields.definitions().owner_type() != entity) {
        return R::err(r,
                      EntityError(std::allocator_arg,
                                  Allocator(r),
                                  entity,
                                  "incompatible dynamic-field owner or loaded layout"));
    }
    // Validate the snapshot borrow before changing either part of the carrier.
    auto next = eval::LoadState::indexed(r, snapshot);
    root.install_loaded_dynamic_fields(fields);
    state = std::move(next);
    return R::ok(r, {});
}

Result<std::monostate, DynamicFieldError>
GeneratedDynamicFields::finish(EntityRuntimeState& root,
                               eval::LoadState& state,
                               LoadedSnapshot::Borrow previous,
                               Result<bool, DynamicFieldError> result) {
    auto r = root.memory_resource();
    using R = Result<std::monostate, DynamicFieldError>;
    if (!result) {
        return R::err(r, std::move(result).error());
    }
    if (result.value()) {
        auto& pool = detail::ContextArena::from_memory(r).loaded_snapshots();
        auto updated =
            LoadedSnapshot::with_dynamic_fields(pool, previous, *root.loaded_dynamic_fields());
        if (!updated) {
            throw std::logic_error("validated dynamic owner and layout");
        }
        state = eval::LoadState::indexed(r, updated.value());
    }
    return R::ok(r, {});
}

Result<std::monostate, DynamicFieldError> GeneratedDynamicFields::update(EntityRuntimeState& root,
                                                                         eval::LoadState& state,
                                                                         const FieldLayout& layout,
                                                                         const EntityKey& key,
                                                                         std::string_view code,
                                                                         const Value& value) {
    auto r = root.memory_resource();
    auto snapshot = validate(root, state, layout, code);
    if (!snapshot) {
        return Result<std::monostate, DynamicFieldError>::err(r, std::move(snapshot).error());
    }
    return finish(root, state, snapshot.value(), root.update_dynamic_field(key, code, value));
}

Result<std::monostate, DynamicFieldError> GeneratedDynamicFields::erase(EntityRuntimeState& root,
                                                                        eval::LoadState& state,
                                                                        const FieldLayout& layout,
                                                                        const EntityKey& key,
                                                                        std::string_view code) {
    auto r = root.memory_resource();
    auto snapshot = validate(root, state, layout, code);
    if (!snapshot) {
        return Result<std::monostate, DynamicFieldError>::err(r, std::move(snapshot).error());
    }
    return finish(root, state, snapshot.value(), root.delete_dynamic_field(key, code));
}
}
