#include <teaql/runtime/context.hpp>
#include <teaql/core/detail/text.hpp>
#include <charconv>

namespace teaql {
String ContextRootError::to_string(Memory r) const {
    String text("active root ", r);
    if (actual_root) {
        text += "type is ";
        text += actual_root->entity_type;
        text += ", expected ";
        text += expected_entity_type;
    } else {
        text += expected_entity_type;
        text += " is missing from UserContext";
    }
    return text;
}

UserContext& UserContext::with_active_root(std::string_view entity_type, std::uint64_t id) {
    ensure_mutable();
    auto canonical = canonical_id_space_entity(memory_resource(), entity_type);
    auto trimmed = std::string_view(canonical);
    detail::trim(trimmed);
    if (trimmed.empty()) {
        throw std::invalid_argument("active root entity type is required");
    }
    if (!id) {
        throw std::invalid_argument("active root id must be positive");
    }
    if (active_root_) {
        throw std::logic_error("active root may only be assigned once");
    }
    active_root_ = &make<ContextEntityRef>(canonical, id);
    return *this;
}

RuntimeHookResult UserContext::set_generated_bootstrap_active_root(std::string_view entity_type,
                                                                   std::uint64_t id) const {
    // Generated bootstraps install their root while ensure_schema borrows Context.
    // This OnceLock-equivalent assignment does not replace borrowed providers.
    ensure_open();
    if (memory_.closing()) {
        throw std::logic_error("Context is closing");
    }
    auto r = memory_resource();
    auto canonical = canonical_id_space_entity(r, entity_type);
    auto trimmed = std::string_view(canonical);
    detail::trim(trimmed);
    if (trimmed.empty() || !id) {
        return RuntimeHookResult::err(r, RuntimeError::schema(r, "invalid generated active root"));
    }
    if (active_root_) {
        if (active_root_->entity_type == canonical && active_root_->id == id) {
            return RuntimeHookResult::ok(r, {});
        }
        String message("active root already set to ", r);
        message += active_root_->entity_type;
        message += ':';
        char digits[20];
        auto result = std::to_chars(digits, digits + sizeof(digits), active_root_->id);
        message.append(digits, result.ptr);
        return RuntimeHookResult::err(r, RuntimeError::schema(r, message));
    }
    active_root_ = &make<ContextEntityRef>(canonical, id);
    return RuntimeHookResult::ok(r, {});
}

Result<ResourceBorrow<const ContextEntityRef>, ContextRootError>
UserContext::require_active_root(std::string_view expected_entity_type) const {
    ensure_open();
    auto r = memory_resource();
    using R = Result<ResourceBorrow<const ContextEntityRef>, ContextRootError>;
    auto canonical = canonical_id_space_entity(r, expected_entity_type);
    if (active_root_ && active_root_->entity_type == canonical) {
        return R::ok(
            r,
            ResourceBorrow<const ContextEntityRef>(
                std::allocator_arg, Allocator(r), *active_root_, detail::resource_arena(r)));
    }
    return R::err(r, ContextRootError(std::allocator_arg, Allocator(r), canonical, active_root_));
}
}
