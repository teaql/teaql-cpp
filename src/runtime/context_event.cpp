#include <teaql/runtime/detail/governance_scope.hpp>
#include <teaql/runtime/event.hpp>
#include <teaql/runtime/log_manager.hpp>
#include <teaql/runtime/telemetry.hpp>
#include <chrono>

namespace teaql {
Result<std::monostate, RuntimeError> UserContext::send_event(const RawAuditEvent& input) const {
    ensure_open();
    auto r = memory_resource();
    Arena::Quiescence active;
    active.attach(detail::resource_arena(r), nullptr, [](void*) noexcept {
        std::terminate();
    });
    using R = Result<std::monostate, RuntimeError>;
    auto event = detail::clone_in(r, input);
    event.mutation_governance = detail::current_mutation_governance(*this);
    if (detail::is_generated_schema_bootstrap(*this) &&
        (event.kind == RawAuditEventKind::Created || event.kind == RawAuditEventKind::Updated)) {
        const Value* version = nullptr;
        if (event.new_values) {
            auto found = event.new_values->find(std::string_view("version"));
            if (found != event.new_values->end()) {
                version = &found->second;
            }
        }
        if (!version) {
            auto found = event.values.find(std::string_view("version"));
            if (found != event.values.end()) {
                version = &found->second;
            }
        }
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::system_clock::now().time_since_epoch())
                          .count();
        event.bootstrap_audit.emplace(std::allocator_arg,
                                      Allocator(r),
                                      "teaql-generated-bootstrap",
                                      "runtime-bootstrap",
                                      event.trace_chain.empty()
                                          ? std::string_view("generated runtime bootstrap")
                                          : std::string_view(event.trace_chain.back().comment),
                                      version ? version->try_i64() : std::nullopt,
                                      millis < 0 ? 0 : static_cast<std::uint64_t>(millis));
    }
    String name(event.entity, r);
    name += ".event";
    RuntimeOperation operation(std::allocator_arg, Allocator(r), "audit", name);
    operation.attribute("teaql.entity.type", std::string_view(event.entity));
    auto& scope = start_runtime_operation(operation);

    struct Close {
        FailOpenRuntimeTelemetryScope& scope;

        ~Close() {
            scope.close();
        }
    } close{scope};

    auto send = [&]() -> R {
        if (event_sink_) {
            if (event_sink_borrow_) {
                (void)event_sink_borrow_->get();
            }
            auto result = event_sink_->on_event(r, *this, event);
            if (!result) {
                return R::err(r, std::move(result).error());
            }
        }
        if (custom_event_sink_) {
            if (custom_event_sink_borrow_) {
                (void)custom_event_sink_borrow_->get();
            }
            auto* descriptor = entity(event.entity);
            auto safe = event.build_safe_event(
                r,
                descriptor ? std::span<const String>(descriptor->audit_mask_fields)
                           : std::span<const String>{},
                descriptor ? descriptor->audit_value_max_len : std::nullopt);
            auto result = custom_event_sink_->on_safe_event(r, *this, safe);
            if (!result) {
                return R::err(r, std::move(result).error());
            }
        }
        global_.log_manager().write_audit_log(r, event);
        return R::ok(r, {});
    };
    auto result = send();
    if (result) {
        scope.success(RuntimeAttributes(r));
    } else {
        scope.failure("audit_error");
    }
    return result;
}
}
