#include <teaql/runtime/detail/commit_audit.hpp>

namespace teaql::detail {
bool has_audit_scope(const UserContext& user_context) {
    return AuditActivation::current(user_context) != nullptr;
}

bool enqueue_audit(const UserContext& user_context, const RawAuditEvent& event) {
    auto* batch = AuditActivation::current(user_context);
    if (!batch) {
        return false;
    }
    batch->events.emplace_back(event);
    return true;
}

Result<std::monostate, RuntimeError>
enrich_dynamic_audit(const UserContext& user_context,
                     std::string_view owner,
                     std::uint64_t id,
                     std::span<const EntityPropertyChange> changes) {
    auto r = user_context.memory_resource();
    using Output = Result<std::monostate, RuntimeError>;
    auto* batch = AuditActivation::current(user_context);
    RawAuditEvent* event = nullptr;
    if (batch) {
        for (auto current = batch->events.rbegin(); current != batch->events.rend(); ++current) {
            auto found = current->values.find("id");
            if (current->entity == owner && found != current->values.end() &&
                found->second.try_u64() == id) {
                event = &*current;
                break;
            }
        }
    }
    if (!event) {
        return Output::err(
            r,
            RuntimeError::dynamic_field(
                r, DynamicFieldError(r, "DYNAMIC_FIELD_AUDIT_SCOPE_REQUIRED", owner)));
    }
    for (const auto& change : changes) {
        if (change.new_value) {
            event->values.insert_or_assign(String(change.field, r), Value(r, *change.new_value));
            if (event->new_values) {
                event->new_values->insert_or_assign(String(change.field, r),
                                                    Value(r, *change.new_value));
            }
        }
        event->updated_fields.emplace_back(change.field);
        event->changes.emplace_back(change);
    }
    return Output::ok(r, {});
}

Result<std::monostate, RuntimeError> deliver_committed_audits(const UserContext& user_context,
                                                              Vector<RawAuditEvent> events) {
    auto r = user_context.memory_resource();
    std::optional<RuntimeError> first_error;
    for (const auto& event : events) {
        auto delivered = ContextAuditAccess::deliver(user_context, event);
        if (!delivered && !first_error) {
            first_error.emplace(std::allocator_arg, Allocator(r), delivered.error());
        }
    }
    if (first_error) {
        return Result<std::monostate, RuntimeError>::err(
            r, RuntimeError::audit_after_commit(r, *first_error));
    }
    return Result<std::monostate, RuntimeError>::ok(r, {});
}
}
