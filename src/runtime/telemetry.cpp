#include <teaql/runtime/telemetry.hpp>
#include <teaql/runtime/context.hpp>

namespace teaql {
namespace {
bool forbidden(std::string_view key) {
    for (auto v : {"teaql.entity.id",
                   "teaql.user.id",
                   "teaql.tenant.id",
                   "teaql.query.parameters",
                   "teaql.field.values",
                   "teaql.audit.reason",
                   "db.query.parameter_values",
                   "http.request.body",
                   "url.full"}) {
        if (key == v) {
            return true;
        }
    }
    return false;
}

bool contains_ascii_lower(std::string_view value, std::string_view term) {
    if (term.size() > value.size()) {
        return false;
    }
    for (std::size_t p = 0; p <= value.size() - term.size(); ++p) {
        bool matches = true;
        for (std::size_t i = 0; i < term.size(); ++i) {
            auto c = value[p + i];
            if (c >= 'A' && c <= 'Z') {
                c += 32;
            }
            if (c != term[i]) {
                matches = false;
                break;
            }
        }
        if (matches) {
            return true;
        }
    }
    return false;
}

class NoopScope final : public RuntimeTelemetryScope {
    void success(const RuntimeAttributes&) override {}

    void failure(std::string_view) override {}
};

class NoopPropagation final : public RuntimeTelemetryPropagationContext {};

template <class T>
void require_delegate(Memory r, ContextBox<T>* box) {
    if (box) {
        if (box->memory_resource() != r) {
            throw std::invalid_argument("Telemetry delegate must belong to its output Context");
        }
        box->claim();
    }
}

template <class Wrapper, class Delegate, class Create>
Wrapper& create_fail_open(Memory r, const RuntimeTelemetry& telemetry, Create&& create) {
    auto& arena = detail::resource_arena(r);
    if (!detail::resource_arena(telemetry.memory_resource()).can_lend_to(arena)) {
        throw std::invalid_argument("Telemetry belongs to a shorter or unrelated Context");
    }
    ContextBox<Delegate>* delegate = nullptr;
    try {
        auto& box = create();
        if (box.memory_resource() == r && box.available()) {
            delegate = &box;
        } else if (box.available()) {
            box.reset();
        }
    } catch (...) {
    }
    try {
        return arena.make<Wrapper>(delegate);
    } catch (...) {
        if (delegate) {
            delegate->reset();
        }
        throw;
    }
}
}

RuntimeOperation::RuntimeOperation(std::allocator_arg_t,
                                   Allocator a,
                                   std::string_view family,
                                   std::string_view name)
    : family(family, a.resource()), name(name, a.resource()),
      attributes(std::less<>{}, a.resource()) {
    attribute("teaql.operation.family", family);
    attribute("teaql.operation.name", name);
}

RuntimeOperation& RuntimeOperation::attribute(std::string_view key,
                                              const RuntimeAttributeValue& value) {
    if (!forbidden(key)) {
        attributes.insert_or_assign(String(key, memory_resource()),
                                    detail::clone_in(memory_resource(), value));
    }
    return *this;
}

std::string_view runtime_error_category(std::string_view error) noexcept {
    struct Group {
        std::string_view category;
        std::initializer_list<std::string_view> terms;
    };

    for (const auto& g :
         {Group{"timeout", {"timeout", "deadline"}},
          Group{"authorization",
                {"authentication", "authorization", "unauthorized", "forbidden", "permission"}},
          Group{"validation", {"validation", "invalidargument", "valueerror", "parse", "format"}},
          Group{"conflict", {"conflict", "optimistic", "version", "duplicate", "alreadyexists"}},
          Group{"transport", {"transport", "network", "connection", "socket", "http", "ioerror"}},
          Group{"provider", {"provider", "sql", "database", "jdbc"}}}) {
        for (auto term : g.terms) {
            if (contains_ascii_lower(error, term)) {
                return g.category;
            }
        }
    }
    return "internal";
}

ContextBox<RuntimeTelemetryPropagationContext>&
RuntimeTelemetry::extract_context(Memory r, const RuntimeContextCarrier&) const {
    return detail::resource_arena(r).make<ContextBox<RuntimeTelemetryPropagationContext>>(
        std::in_place_type<NoopPropagation>);
}

ContextBox<RuntimeTelemetryScope>& NoopRuntimeTelemetry::start(Memory r,
                                                               const RuntimeOperation&) const {
    return detail::resource_arena(r).make<ContextBox<RuntimeTelemetryScope>>(
        std::in_place_type<NoopScope>);
}

FailOpenRuntimeTelemetryScope::FailOpenRuntimeTelemetryScope(
    std::allocator_arg_t, Allocator a, ContextBox<RuntimeTelemetryScope>* delegate)
    : resource_(a.resource()), delegate_(delegate) {
    require_delegate(resource_, delegate_);
}

FailOpenRuntimeTelemetryPropagationContext::FailOpenRuntimeTelemetryPropagationContext(
    std::allocator_arg_t, Allocator a, ContextBox<RuntimeTelemetryPropagationContext>* delegate)
    : resource_(a.resource()), delegate_(delegate) {
    require_delegate(resource_, delegate_);
}

void FailOpenRuntimeTelemetryScope::success(const RuntimeAttributes& attributes) {
    if (polling_) {
        throw std::logic_error("Cannot finish telemetry scope while activating it");
    }
    if (poisoned_ || !delegate_) {
        return;
    }
    auto* owner = std::exchange(delegate_, nullptr);
    try {
        if (auto* scope = owner->get()) {
            scope->success(attributes);
        }
    } catch (...) {
    }
    owner->reset();
}

void FailOpenRuntimeTelemetryScope::failure(std::string_view error) {
    if (polling_) {
        throw std::logic_error("Cannot finish telemetry scope while activating it");
    }
    if (poisoned_ || !delegate_) {
        return;
    }
    auto* owner = std::exchange(delegate_, nullptr);
    try {
        if (auto* scope = owner->get()) {
            scope->failure(error);
        }
    } catch (...) {
    }
    owner->reset();
}

FailOpenRuntimeTelemetryScope&
start_runtime_operation(Memory r, const RuntimeTelemetry& t, const RuntimeOperation& operation) {
    return create_fail_open<FailOpenRuntimeTelemetryScope, RuntimeTelemetryScope>(
        r, t, [&]() -> auto& {
            return t.start(r, operation);
        });
}

FailOpenRuntimeTelemetryPropagationContext&
extract_runtime_context(Memory r, const RuntimeTelemetry& t, const RuntimeContextCarrier& carrier) {
    return create_fail_open<FailOpenRuntimeTelemetryPropagationContext,
                            RuntimeTelemetryPropagationContext>(r, t, [&]() -> auto& {
        return t.extract_context(r, carrier);
    });
}

const RuntimeTelemetry& UserContext::runtime_telemetry() const {
    ensure_open();
    if (!runtime_telemetry_) {
        auto& telemetry = make<NoopRuntimeTelemetry>();
        auto registered = detail::ContextResources::entry(
            detail::resource_arena(memory_resource()), memory_resource(), telemetry);
        runtime_telemetry_.emplace(std::allocator_arg,
                                   Allocator(memory_resource()),
                                   static_cast<const RuntimeTelemetry&>(telemetry),
                                   *registered.owner,
                                   registered.slot);
    }
    return runtime_telemetry_->get();
}

bool UserContext::runtime_telemetry_is_noop() const {
    ensure_open();
    Arena::Quiescence active;
    if (!memory_.closing()) {
        active.attach(const_cast<detail::ContextArena&>(memory_), nullptr, [](void*) noexcept {
            std::terminate();
        });
    }
    return runtime_telemetry().is_noop();
}

FailOpenRuntimeTelemetryScope&
UserContext::start_runtime_operation(const RuntimeOperation& operation) const {
    Arena::Quiescence active;
    active.attach(detail::resource_arena(memory_resource()), nullptr, [](void*) noexcept {
        std::terminate();
    });
    return teaql::start_runtime_operation(memory_resource(), runtime_telemetry(), operation);
}
}
