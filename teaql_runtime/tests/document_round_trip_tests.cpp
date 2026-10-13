#include <iostream>
#include <memory_resource>
#include <optional>
#include <variant>
#include <array>
#ifdef TEAQL_USE_MODULES
import teaql_runtime;
#else
#include <teaql/runtime.hpp>
#endif

using namespace teaql;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

struct State {
    bool ready = false;
    int polls = 0, cancelled = 0;
    WakeSignal* wake = nullptr;
};

template <class T>
struct Deferred {
    using allocator_type = Allocator;
    State& state;
    DocumentResult<T> result;

    Deferred(std::allocator_arg_t, Allocator a, State& s, DocumentResult<T> value)
        : state(s), result(std::allocator_arg, a, value) {}

    std::optional<DocumentResult<T>> poll(WakeSignal& wake) {
        ++state.polls;
        state.wake = &wake;
        if (!state.ready) {
            return std::nullopt;
        }
        return std::optional<DocumentResult<T>>(std::in_place, std::move(result));
    }

    void cancel_and_wait() noexcept {
        ++state.cancelled;
        state.wake = nullptr;
    }

    void finish_and_wait() noexcept {
        state.wake = nullptr;
    }
};

struct Service final : ContextBoundDocumentService {
    using allocator_type = Allocator;
    State& state;
    Memory foreign;
    mutable int calls = 0;

    Service(std::allocator_arg_t, Allocator, State& state, Memory foreign = nullptr)
        : state(state), foreign(foreign) {}

    AsyncOperation<DocumentResult<ContextualDocument>>&
    open_document(const UserContext& user_context,
                  const TrustedReferencePrincipal& principal,
                  DocumentOpenRequest request) const override {
        ++calls;
        check(principal.subject == "alice" && request.business_id == "ORD-1",
              "service request/principal changed");
        auto r = foreign ? foreign : user_context.memory_resource();
        Json::Object body(r);
        body.emplace(String("text", r), Json(r, String(1024, 'x', r)));
        auto result = DocumentResult<ContextualDocument>::ok(
            r,
            ContextualDocument(
                std::allocator_arg, Allocator(r), request.business_id, "token", Json(r, body)));
        return make_operation<DocumentResult<ContextualDocument>, Deferred<ContextualDocument>>(
            r, state, std::move(result));
    }

    AsyncOperation<DocumentResult<AcceptedContextualDocument>>&
    accept_document(const UserContext& user_context,
                    const TrustedReferencePrincipal& principal,
                    DocumentAcceptRequest request) const override {
        ++calls;
        check(principal.subject == "alice" && request.submitted.command_id == "command-1",
              "submitted request changed");
        auto r = foreign ? foreign : user_context.memory_resource();
        ContextualDocument document(std::allocator_arg,
                                    Allocator(r),
                                    request.submitted.business_id,
                                    "next-token",
                                    request.submitted.body);
        Vector<ReferenceIdentity> changed(r);
        changed.emplace_back("Order", 10, 2);
        auto result = DocumentResult<AcceptedContextualDocument>::ok(
            r,
            AcceptedContextualDocument(std::allocator_arg, Allocator(r), document, 1, 2, changed));
        return make_operation<DocumentResult<AcceptedContextualDocument>,
                              Deferred<AcceptedContextualDocument>>(r, state, std::move(result));
    }
};

static DocumentOpenRequest open_request(Memory r) {
    return DocumentOpenRequest(
        std::allocator_arg, Allocator(r), "Order", "ORD-1", "edit", ReferenceLifetime(600));
}

static void principal(UserContext& user_context) {
    auto r = user_context.memory_resource();
    user_context.set_trusted_reference_principal(TrustedReferencePrincipal(
        std::allocator_arg, Allocator(r), "oidc", "alice", "Merchant", 7));
}

static void dispatch_lifetimes() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    const auto& absent = user_context->open_document(open_request(r)).wait();
    check(!absent && absent.error() == DocumentRoundTripError::configuration(),
          "missing service takes precedence");
    State state;
    auto& service = global.make<Service>(state);
    user_context->set_context_bound_document_service(global, service);
    const auto& unauthorized = user_context->open_document(open_request(r)).wait();
    check(!unauthorized &&
              unauthorized.error() == DocumentRoundTripError::authorization_required() &&
              service.calls == 0,
          "principal is required before service calls");
    principal(*user_context);
    auto& operation = user_context->open_document(open_request(r));
    check(!operation.poll() && state.polls == 1, "child service must remain pending");
    auto ticket = operation.wake_signal().ticket();
    state.ready = true;
    state.wake->notify();
    check(operation.wake_signal().ticket() != ticket, "document child wakes must reach the parent");
    const auto& completed = operation.wait();
    check(completed && completed.value().body.memory_resource() == r,
          "document result belongs to UserContext");
    auto retained = user_context->make_global<ContextualDocument>(completed.value());
    {
        SubmittedContextualDocument submitted(std::allocator_arg,
                                              Allocator(r),
                                              "ORD-1",
                                              "token",
                                              "command-1",
                                              completed.value().body);
        auto accept = DocumentAcceptRequest::create(r, "Order", "edit", submitted);
        check(static_cast<bool>(accept), "accept construction failed");
        const auto& accepted = user_context->accept_document(std::move(accept).value()).wait();
        check(accepted && accepted.value().previous_revision == 1 &&
                  accepted.value().new_revision == 2 &&
                  accepted.value().changed_entities.size() == 1,
              "accepted document loses revision/change state");
    }
    state.ready = false;
    auto& pending = user_context->open_document(open_request(r));
    check(!pending.poll(), "pending service fixture failed");
    user_context.reset();
    check(state.cancelled == 1 && !state.wake,
          "Context close must join the pending document service");
    auto next_user_context = global.make_user_context();
    const auto& copy = next_user_context->get(next_user_context->rebind_global(retained));
    const auto& object = std::get<Json::Object>(copy.body.data);
    check(copy.memory_resource() == global.memory_resource() &&
              std::get<String>(object.find("text")->second.data).size() == 1024,
          "explicit Global document copy must survive source close");
}

static void foreign_child() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto other_user_context = global.make_user_context();
    State state;
    auto& service = global.make<Service>(state, other_user_context->memory_resource());
    user_context->set_context_bound_document_service(global, service);
    principal(*user_context);
    bool rejected = false;
    try {
        (void)user_context->open_document(open_request(user_context->memory_resource())).poll();
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    check(rejected && state.cancelled == 1, "foreign child must cancel before rejection");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        dispatch_lifetimes();
        foreign_child();
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
    std::cout << "Document dispatch/Context contracts passed\n";
}
