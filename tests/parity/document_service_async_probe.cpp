#include <iostream>
#include <memory_resource>
#include <optional>
#include <stdexcept>
#include <string>
#include <variant>
#include <teaql/runtime.hpp>

using namespace teaql;

static const Json& field(const Json& input, std::string_view key) {
    return std::get<Json::Object>(input.data).find(key)->second;
}

static std::string_view text(const Json& input, std::string_view key) {
    return std::get<String>(field(input, key).data);
}

static bool flag(const Json& input, std::string_view key) {
    return std::get<bool>(field(input, key).data);
}

static void put(Json::Object& object, std::string_view key, Json value) {
    object.emplace(String(key, object.get_allocator().resource()), std::move(value));
}

struct State {
    Memory resource;
    bool ready = false;
    std::size_t calls = 0, polls = 0, cancelled = 0, finished = 0;
    WakeSignal* wake = nullptr;
    Json::Array events;

    explicit State(Memory r) : resource(r), events(r) {}
};

struct Marker {
    using allocator_type = Allocator;
    String value;

    Marker(std::allocator_arg_t, Allocator a, std::string_view label)
        : value(label, a.resource()) {}
};

template <class T>
struct Deferred {
    using allocator_type = Allocator;
    State& state;
    DocumentResult<T> result;

    Deferred(std::allocator_arg_t, Allocator a, State& state, DocumentResult<T> value)
        : state(state), result(std::allocator_arg, a, value) {}

    std::optional<DocumentResult<T>> poll(WakeSignal& wake) {
        ++state.polls;
        if (!state.ready) {
            state.wake = &wake;
            return std::nullopt;
        }
        return std::optional<DocumentResult<T>>(std::in_place, std::move(result));
    }

    void cancel_and_wait() noexcept {
        ++state.cancelled;
        state.wake = nullptr;
    }

    void finish_and_wait() noexcept {
        ++state.finished;
        state.wake = nullptr;
    }
};

static std::optional<DocumentRoundTripError> service_error(std::string_view kind) {
    if (kind == "revision") {
        return DocumentRoundTripError::revision_conflict();
    }
    if (kind == "validation") {
        return DocumentRoundTripError::validation_failed();
    }
    if (kind == "projection") {
        return DocumentRoundTripError::projection_violation();
    }
    if (kind == "not_found") {
        return DocumentRoundTripError::not_found();
    }
    if (kind == "invalid") {
        return DocumentRoundTripError::invalid();
    }
    if (kind == "expired") {
        return DocumentRoundTripError::expired();
    }
    if (kind == "scope") {
        return DocumentRoundTripError::scope_mismatch();
    }
    if (kind == "authorization") {
        return DocumentRoundTripError::authorization_required();
    }
    if (kind == "configuration") {
        return DocumentRoundTripError::configuration();
    }
    return std::nullopt;
}

struct Service final : ContextBoundDocumentService {
    using allocator_type = Allocator;
    State& state;
    String label, error;

    Service(std::allocator_arg_t,
            Allocator a,
            State& state,
            std::string_view label,
            std::string_view error)
        : state(state), label(label, a.resource()), error(error, a.resource()) {}

    void record(const UserContext& user_context,
                const TrustedReferencePrincipal& principal,
                std::string_view method,
                Json request) const {
        auto r = state.resource;
        Json::Object event(r);
        put(event, "service", Json(r, label));
        put(event, "method", Json(r, method));
        put(event, "subject", Json(r, principal.subject));
        put(event, "realm", Json(r, principal.authentication_realm));
        put(event, "root_type", Json(r, principal.domain_root_type));
        put(event, "root_id", Json(r, principal.domain_root_id));
        auto* marker = user_context.get_resource<Marker>();
        put(event, "marker", marker ? Json(r, marker->value) : Json(r));
        put(event, "request", std::move(request));
        state.events.emplace_back(event);
        ++state.calls;
    }

    AsyncOperation<DocumentResult<ContextualDocument>>&
    open_document(const UserContext& user_context,
                  const TrustedReferencePrincipal& principal,
                  DocumentOpenRequest request) const override {
        auto r = user_context.memory_resource();
        auto g = state.resource;
        Json::Object event(g);
        put(event, "business", Json(g, request.business_id));
        put(event, "purpose", Json(g, request.purpose));
        put(event, "type", Json(g, request.aggregate_type));
        put(event, "seconds", Json(g, request.lifetime.seconds));
        put(event, "nanos", Json(g, request.lifetime.nanoseconds));
        record(user_context, principal, "open", Json(g, event));
        auto result = [&] {
            if (auto failure = service_error(error)) {
                return DocumentResult<ContextualDocument>::err(r, *failure);
            }
            Json::Object body(r);
            put(body, "service", Json(r, label));
            return DocumentResult<ContextualDocument>::ok(
                r,
                ContextualDocument(
                    std::allocator_arg, Allocator(r), request.business_id, label, Json(r, body)));
        }();
        return make_operation<DocumentResult<ContextualDocument>, Deferred<ContextualDocument>>(
            r, state, std::move(result));
    }

    AsyncOperation<DocumentResult<AcceptedContextualDocument>>&
    accept_document(const UserContext& user_context,
                    const TrustedReferencePrincipal& principal,
                    DocumentAcceptRequest request) const override {
        auto r = user_context.memory_resource();
        auto g = state.resource;
        Json::Object event(g);
        put(event, "business", Json(g, request.submitted.business_id));
        put(event, "purpose", Json(g, request.purpose));
        put(event, "type", Json(g, request.expected_aggregate_type));
        put(event, "command", Json(g, request.submitted.command_id));
        put(event, "token", Json(g, request.submitted.document_token));
        put(event, "body", Json(g, request.submitted.body));
        record(user_context, principal, "accept", Json(g, event));
        auto result = [&] {
            if (auto failure = service_error(error)) {
                return DocumentResult<AcceptedContextualDocument>::err(r, *failure);
            }
            ContextualDocument document(std::allocator_arg,
                                        Allocator(r),
                                        request.submitted.business_id,
                                        label,
                                        request.submitted.body);
            Vector<ReferenceIdentity> changed(r);
            changed.emplace_back("Order", 10, 5);
            changed.emplace_back("Line", 21, 2);
            return DocumentResult<AcceptedContextualDocument>::ok(
                r,
                AcceptedContextualDocument(
                    std::allocator_arg, Allocator(r), document, 4, 5, changed));
        }();
        return make_operation<DocumentResult<AcceptedContextualDocument>,
                              Deferred<AcceptedContextualDocument>>(r, state, std::move(result));
    }
};

static Json value_json(Memory r, const ContextualDocument& value) {
    Json::Object result(r);
    put(result, "business", Json(r, value.business_id));
    put(result, "token", Json(r, value.document_token));
    put(result, "body", Json(r, value.body));
    return Json(r, result);
}

static Json value_json(Memory r, const AcceptedContextualDocument& value) {
    auto result = value_json(r, value.document);
    auto& object = std::get<Json::Object>(result.data);
    put(object, "previous", Json(r, value.previous_revision));
    put(object, "next", Json(r, value.new_revision));
    Json::Array changed(r);
    for (const auto& identity : value.changed_entities) {
        Json::Object item(r);
        put(item, "type", Json(r, identity.entity_type));
        put(item, "id", Json(r, identity.id));
        put(item, "version", Json(r, identity.version));
        changed.emplace_back(item);
    }
    put(object, "changed", Json(r, changed));
    return result;
}

template <class T>
static Json drive(AsyncOperation<DocumentResult<T>>& operation, State& state, const Json& input) {
    auto r = state.resource;
    auto finish = text(input, "finish");
    const DocumentResult<T>* completed = nullptr;
    if (finish != "unpolled") {
        auto count = std::get<std::uint64_t>(field(input, "pending").data);
        for (std::uint64_t i = 0; i < count; ++i) {
            if ((completed = operation.poll())) {
                break;
            }
        }
    }
    bool woke = false;
    if (!completed && finish == "ready") {
        auto ticket = operation.wake_signal().ticket();
        state.ready = true;
        if (state.wake) {
            state.wake->notify();
        }
        woke = operation.wake_signal().ticket() != ticket;
        completed = operation.poll();
        if (!completed) {
            throw std::runtime_error("Ready document service remains pending");
        }
    }
    Json::Object result(r);
    put(result, "woke", Json(r, woke));
    if (completed) {
        put(result, "status", Json(r, *completed ? "ready" : "error"));
        if (*completed) {
            put(result, "value", value_json(r, completed->value()));
        } else {
            put(result, "error", Json(r, completed->error().code()));
        }
    } else {
        put(result, "status", Json(r, "cancelled"));
        if (finish != "close") {
            operation.cancel();
        }
    }
    return Json(r, result);
}

static Json phase(UserContext& user_context, State& state, const Json& input) {
    state.ready = false;
    auto r = user_context.memory_resource();
    if (text(input, "method") == "open") {
        DocumentOpenRequest request(std::allocator_arg,
                                    Allocator(r),
                                    "Order",
                                    "ORD-source",
                                    "edit-source",
                                    ReferenceLifetime(60));
        return drive(user_context.open_document(std::move(request)), state, input);
    }
    SubmittedContextualDocument submitted(std::allocator_arg,
                                          Allocator(r),
                                          "ORD-source",
                                          "prior-token",
                                          "command-source",
                                          field(input, "body"));
    DocumentAcceptRequest request(
        std::allocator_arg, Allocator(r), "Order", "edit-source", submitted);
    return drive(user_context.accept_document(std::move(request)), state, input);
}

static void install_principal(UserContext& user_context, std::string_view subject) {
    user_context.set_trusted_reference_principal(
        TrustedReferencePrincipal(std::allocator_arg,
                                  Allocator(user_context.memory_resource()),
                                  "oidc",
                                  subject,
                                  "Merchant",
                                  7));
}

static Json observe(GlobalContext& global, const Json& input) {
    auto r = global.memory_resource();
    State state(r);
    auto user_context_owner = global.make_user_context();
    auto& user_context = *user_context_owner;
    if (flag(input, "principal")) {
        install_principal(user_context, "alice");
    }
    if (flag(input, "marker")) {
        user_context.insert_resource(user_context, user_context.make<Marker>("marker-first"));
    }
    if (flag(input, "service")) {
        auto& service =
            user_context.make<Service>(state, text(input, "label"), text(input, "error"));
        user_context.set_context_bound_document_service(user_context, service);
    }
    Json::Array phases(r);
    phases.emplace_back(phase(user_context, state, input));
    if (flag(input, "replace")) {
        auto& service =
            user_context.make<Service>(state, "replacement-service", text(input, "error"));
        user_context.set_context_bound_document_service(user_context, service);
        install_principal(user_context, "bob");
        user_context.insert_resource(user_context, user_context.make<Marker>("marker-replacement"));
        phases.emplace_back(phase(user_context, state, input));
    }
    user_context_owner.reset();
    if (global.live_user_bytes() != 0 || state.wake) {
        throw std::runtime_error("Document service survives actual UserContext deletion");
    }
    Json::Object output(r);
    put(output, "phases", Json(r, phases));
    put(output, "calls", Json(r, state.calls));
    put(output, "polls", Json(r, state.polls));
    put(output, "cancelled", Json(r, state.cancelled));
    put(output, "finished", Json(r, state.finished));
    put(output, "has_waker", Json(r, state.wake != nullptr));
    put(output, "events", Json(r, state.events));
    return Json(r, output);
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        std::string line;
        while (std::getline(std::cin, line)) {
            GlobalContext global;
            auto input = Json::parse(global.memory_resource(), line);
            if (!input) {
                throw std::runtime_error("Invalid document-service observation input");
            }
            auto output = observe(global, *input);
            std::cout << output.dump(global.memory_resource()) << '\n';
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
