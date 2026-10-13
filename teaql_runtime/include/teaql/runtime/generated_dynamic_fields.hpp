#pragma once
#include <teaql/runtime/entity_runtime.hpp>

namespace teaql {
// Shared counterpart of the indexed entity carrier emitted by Rust TeaqlEntity.
class GeneratedDynamicFields {
public:
    static Result<std::monostate, EntityError> install(EntityRuntimeState&,
                                                       eval::LoadState&,
                                                       const FieldLayout&,
                                                       const DynamicFieldValues&,
                                                       LoadedSnapshot::Borrow,
                                                       std::string_view entity);
    static Result<std::monostate, DynamicFieldError> update(EntityRuntimeState&,
                                                            eval::LoadState&,
                                                            const FieldLayout&,
                                                            const EntityKey&,
                                                            std::string_view code,
                                                            const Value&);
    static Result<std::monostate, DynamicFieldError> erase(EntityRuntimeState&,
                                                           eval::LoadState&,
                                                           const FieldLayout&,
                                                           const EntityKey&,
                                                           std::string_view code);

private:
    static Result<LoadedSnapshot::Borrow, DynamicFieldError>
    validate(EntityRuntimeState&, const eval::LoadState&, const FieldLayout&, std::string_view);
    static Result<std::monostate, DynamicFieldError> finish(EntityRuntimeState&,
                                                            eval::LoadState&,
                                                            LoadedSnapshot::Borrow,
                                                            Result<bool, DynamicFieldError>);
};
}
