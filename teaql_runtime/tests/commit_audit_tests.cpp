#include <teaql/runtime/detail/commit_audit.hpp>
#include <teaql/runtime/error_debug.hpp>
#include <teaql/runtime/dynamic_fields.hpp>
#include <teaql/runtime/transaction.hpp>
#include "../../tests/support/graph_saver_fixture.hpp"
#include <array>
#include <iostream>

using namespace teaql;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

struct Sink final : RawAuditEventSink {
    using allocator_type = Allocator;
    mutable Vector<RawAuditEvent> events;
    mutable bool fail_first = false;

    Sink(std::allocator_arg_t, Allocator a) : events(a.resource()) {}

    Memory memory_resource() const {
        return events.get_allocator().resource();
    }

    Result<std::monostate, RuntimeError>
    on_event(Memory r, const UserContext&, const RawAuditEvent& event) const override {
        events.emplace_back(event);
        if (fail_first && event.values.at(String("id", r)).try_u64() == 7) {
            return Result<std::monostate, RuntimeError>::err(
                r, RuntimeError::event(r, "first sink failed"));
        }
        return Result<std::monostate, RuntimeError>::ok(r, {});
    }
};

static RawAuditEvent event(Memory r, std::uint64_t id) {
    Record values(r);
    values.emplace(String("id", r), Value(r, id));
    values.emplace(String("name", r), Value(r, "entity"));
    return RawAuditEvent::updated(r, "Item", values);
}

static void isolated_scopes_and_enrichment() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto next_user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto next_r = next_user_context->memory_resource();
    auto& sink = global.make<Sink>();
    user_context->set_event_sink(global, sink);
    next_user_context->set_event_sink(global, sink);
    check(!detail::has_audit_scope(*user_context), "no ambient audit queue");
    detail::PendingAuditBatch outer(std::allocator_arg, Allocator(r), *user_context);
    detail::PendingAuditBatch inner(std::allocator_arg, Allocator(r), *user_context);
    {
        detail::AuditActivation active(*user_context, outer);
        check(detail::has_audit_scope(*user_context) &&
                  !detail::has_audit_scope(*next_user_context),
              "scope checks the exact Context");
        check(bool(user_context->send_event(event(r, 7))), "outer fact queued");
        check(sink.events.empty(), "queued facts reach no sink before delivery");
        check(bool(next_user_context->send_event(event(next_r, 10))),
              "another Context delivers immediately");
        check(sink.events.size() == 1, "other Context does not join queue");
        {
            detail::AuditActivation nested(*user_context, inner);
            user_context->send_event(event(r, 8));
        }
        check(outer.events.size() == 1 && inner.events.size() == 1, "nested queues are isolated");
        check(bool(detail::deliver_committed_audits(*user_context, std::move(inner.events))),
              "committed inner facts bypass the restored outer queue");
        check(outer.events.size() == 1 && sink.events.size() == 2,
              "inner delivery cannot requeue in outer scope");
        user_context->send_event(event(r, 9));
        Value actual(r, "persisted");
        auto mutation = DynamicFieldMutation::set(r, Value(r, "proposed"));
        std::optional<Value> audit_value(detail::mutation_audit_value(r, mutation, &actual));
        Vector<EntityPropertyChange> changes(r);
        changes.emplace_back("#title", std::optional<Value>(Value(r)), audit_value);
        check(bool(detail::enrich_dynamic_audit(*user_context, "Item", 7, changes)),
              "extension enriches its native entity fact");
        check(outer.events.front().updated_fields.back() == "#title" &&
                  outer.events.front().values.at(String("#title", r)).to_json_value(r).dump(r) ==
                      "{\"operation\":\"SET\",\"value\":\"persisted\"}",
              "fact records actual extension value and update field");
        auto missing = detail::enrich_dynamic_audit(*user_context, "Item", 99, changes);
        check(!missing && missing.error().dynamic_field_error().code ==
                              "DYNAMIC_FIELD_AUDIT_SCOPE_REQUIRED",
              "native fact must exist before extension audit enrichment");
    }
    check(!detail::has_audit_scope(*user_context), "activation restored after poll");
    sink.fail_first = true;
    auto delivered = detail::deliver_committed_audits(*user_context, std::move(outer.events));
    check(!delivered && delivered.error().kind() == RuntimeError::Kind::AuditAfterCommit,
          "delivery failure is an after-commit error");
    check(sink.events.size() == 4 && sink.events.back().values.at(String("id", r)).try_u64() == 9,
          "failure does not hide later committed entity facts");
    check(delivered.error().audit_after_commit_error().message() == "first sink failed" &&
              delivered.error().source() == nullptr,
          "wrapper owns the first error while preserving the source Error protocol");
}

struct TransactionFact {
    using allocator_type = Allocator;
    const UserContext& user_context;

    TransactionFact(std::allocator_arg_t, Allocator, const UserContext& user_context)
        : user_context(user_context) {}

    std::optional<Result<std::uint64_t, RuntimeError>> poll(WakeSignal&) {
        auto r = user_context.memory_resource();
        check(bool(user_context.send_event(event(r, 7))), "enqueue first scope fact");
        check(bool(user_context.send_event(event(r, 9))), "enqueue second scope fact");
        return Result<std::uint64_t, RuntimeError>::ok(r, 2);
    }

    void finish_and_wait() noexcept {}

    void cancel_and_wait() noexcept {}
};

static void pending_commit_isolation() {
    for (unsigned mode : {0U, 1U, 2U}) {
        GlobalContext global;
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        auto& state = user_context->make<mutation_fixture::State>();
        auto& controls = user_context->make<saver_fixture::Controls>();
        controls.pending_terminal = true;
        controls.fail_commit = mode == 2;
        auto executor = user_context->make_resource_ref<saver_fixture::Ambient>(state, controls);
        user_context->insert_resource(executor);
        auto& sink = user_context->make<Sink>();
        user_context->set_event_sink(*user_context, sink);
        auto& work = user_context->execute_in_transaction<saver_fixture::Ambient, std::uint64_t>(
            [&](const TransactionScope<saver_fixture::Ambient>& scope)
                -> AsyncOperation<Result<std::uint64_t, RuntimeError>>& {
                return make_operation<Result<std::uint64_t, RuntimeError>, TransactionFact>(
                    r, scope.context());
            });
        check(work.poll() == nullptr && sink.events.empty() &&
                  !detail::has_audit_scope(*user_context),
              "pending commit retains private facts without an ambient activation");
        if (mode == 1) {
            work.cancel();
            check(sink.events.empty() && controls.abandoned == 1,
                  "cancelled pending commit discards facts and releases transaction");
        } else {
            controls.pending_terminal = false;
            auto& result = work.wait();
            check(bool(result) == (mode == 0) && sink.events.size() == (mode == 0 ? 2 : 0),
                  "only successful terminal commit delivers retained facts");
        }
    }
}

static void recursive_error_retention() {
    GlobalContext global;
    RuntimeError* retained = nullptr;
    {
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        auto source = RuntimeError::event(r, String(4096, 'x', r));
        auto wrapped =
            RuntimeError::audit_after_commit(r, RuntimeError::audit_after_commit(r, source));
        retained = &global.make<RuntimeError>(wrapped);
    }
    check(retained->audit_after_commit_error().audit_after_commit_error().message().size() == 4096,
          "nested errors explicitly copied to Global survive actual User deletion");
    auto text = retained->to_string(global.memory_resource());
    check(text.starts_with("AUDIT_DELIVERY_AFTER_COMMIT:") && text.find("xxxx") == text.npos,
          "public display excludes nested failure payload");
    check(debug_string(global.memory_resource(), *retained)
              .starts_with("AuditAfterCommit { source: AuditAfterCommit"),
          "raw Debug preserves the nested error structure");
}

struct InterleavedAudit {
    using allocator_type = Allocator;
    const UserContext& user_context;
    detail::PendingAuditBatch& batch;
    std::uint64_t id;
    bool suspended = false;

    InterleavedAudit(std::allocator_arg_t,
                     Allocator,
                     const UserContext& user_context,
                     detail::PendingAuditBatch& batch,
                     std::uint64_t id)
        : user_context(user_context), batch(batch), id(id) {}

    std::optional<Result<std::uint64_t, RuntimeError>> poll(WakeSignal& wake) {
        detail::AuditActivation active(user_context, batch);
        auto r = user_context.memory_resource();
        check(bool(user_context.send_event(event(r, id))), "Enqueue this operation's fact");
        if (!suspended) {
            suspended = true;
            wake.notify();
            return {};
        }
        return Result<std::uint64_t, RuntimeError>::ok(r, batch.events.size());
    }

    void finish_and_wait() noexcept {}

    void cancel_and_wait() noexcept {}
};

static void interleaved_operations_reusing_one_context() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    auto& left = user_context->make<detail::PendingAuditBatch>(*user_context);
    auto& right = user_context->make<detail::PendingAuditBatch>(*user_context);
    auto& first = make_operation<Result<std::uint64_t, RuntimeError>, InterleavedAudit>(
        r, *user_context, left, std::uint64_t(7));
    auto& second = make_operation<Result<std::uint64_t, RuntimeError>, InterleavedAudit>(
        r, *user_context, right, std::uint64_t(9));
    check(first.poll() == nullptr && !detail::has_audit_scope(*user_context),
          "First suspension restores ambient activation");
    check(second.poll() == nullptr && !detail::has_audit_scope(*user_context),
          "Interleaved suspension restores ambient activation");
    check(second.wait().value() == 2 && !detail::has_audit_scope(*user_context),
          "Second operation finishes independently before the first");
    check(first.wait().value() == 2 && !detail::has_audit_scope(*user_context),
          "First operation resumes its own queue");
    for (const auto& fact : left.events) {
        check(fact.values.at(String("id", r)).try_u64() == 7, "Left facts stay in the left queue");
    }
    for (const auto& fact : right.events) {
        check(fact.values.at(String("id", r)).try_u64() == 9,
              "Right facts stay in the right queue");
    }
    user_context.reset();
    check(global.live_user_bytes() == 0, "Interleaved operation and queue storage is reclaimed");
}

static void large_work_payload_stays_outside_scope_carrier() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    {
        auto small_work = [] {
            return std::uint64_t(3);
        };
        std::array<std::uint8_t, 64 * 1024> padding{};
        padding.back() = 7;
        auto large_work = [padding] {
            return std::uint64_t(padding.back());
        };
        ContextOnceFunction<std::uint64_t()> small(
            std::allocator_arg, Allocator(r), std::move(small_work));
        ContextOnceFunction<std::uint64_t()> large(
            std::allocator_arg, Allocator(r), std::move(large_work));
        static_assert(sizeof(detail::ScopeExecution<saver_fixture::Ambient, std::uint64_t>) <
                      sizeof(padding));
        check(sizeof(small) == sizeof(large) && sizeof(large) < sizeof(large_work),
              "Scope callback carrier excludes the captured work payload");
        check(small() == 3 && large() == 7, "Both Context-owned callback payloads execute");
    }
    user_context.reset();
    check(global.live_user_bytes() == 0, "Large captured work storage is reclaimed at close");
}

int main() {
    isolated_scopes_and_enrichment();
    pending_commit_isolation();
    recursive_error_retention();
    interleaved_operations_reusing_one_context();
    large_work_payload_stays_outside_scope_carrier();
    std::cout << "Commit audit scope contracts passed\n";
}
