#pragma once
#include <teaql/runtime/context.hpp>
#include <teaql/runtime/event.hpp>

namespace teaql::detail {
struct PendingAuditBatch {
    using allocator_type = Allocator;
    const UserContext* context;
    Vector<RawAuditEvent> events;

    PendingAuditBatch(std::allocator_arg_t, Allocator a, const UserContext& user_context)
        : context(&user_context), events(a.resource()) {
        if (a.resource() != user_context.memory_resource()) {
            throw std::invalid_argument(
                "Pending audit facts must belong to their operation Context");
        }
    }

    Memory memory_resource() const {
        return events.get_allocator().resource();
    }
};

// Borrowed application activation exists only during a synchronous poll. Every
// transaction/future owns its own batch; neither UserContext nor TLS owns a queue.
class AuditActivation final {
public:
    AuditActivation(const UserContext& user_context, PendingAuditBatch& batch)
        : global_(user_context.global_), previous_(global_.active_audit_batch_) {
        user_context.ensure_open();
        user_context.memory_.check_thread();
        if (batch.context != &user_context ||
            batch.memory_resource() != user_context.memory_resource()) {
            throw std::invalid_argument("Audit activation requires the batch's own UserContext");
        }
        global_.active_audit_batch_ = &batch;
    }

    ~AuditActivation() {
        global_.active_audit_batch_ = previous_;
    }

    AuditActivation(const AuditActivation&) = delete;
    AuditActivation& operator=(const AuditActivation&) = delete;

    static PendingAuditBatch* current(const UserContext& user_context) {
        user_context.ensure_open();
        auto* batch = user_context.global_.active_audit_batch_;
        return batch && batch->context == &user_context ? batch : nullptr;
    }

private:
    GlobalContext& global_;
    PendingAuditBatch* previous_;
};

struct ContextAuditAccess {
    static Result<std::monostate, RuntimeError> deliver(const UserContext& user_context,
                                                        const RawAuditEvent& event) {
        return user_context.deliver_audit_event(event);
    }
};

bool has_audit_scope(const UserContext&);
bool enqueue_audit(const UserContext&, const RawAuditEvent&);
Result<std::monostate, RuntimeError> enrich_dynamic_audit(const UserContext&,
                                                          std::string_view owner,
                                                          std::uint64_t id,
                                                          std::span<const EntityPropertyChange>);
Result<std::monostate, RuntimeError> deliver_committed_audits(const UserContext&,
                                                              Vector<RawAuditEvent>);
}
