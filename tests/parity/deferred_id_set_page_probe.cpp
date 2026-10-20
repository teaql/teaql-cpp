#include "../generated_dynamic_json/include/cpp_dynamic_json.hpp"
#include "fixture_value.hpp"
#include "../support/dynamic_json_runtime_fixture.hpp"
#include <iostream>

using namespace teaql;

struct Control {
    using allocator_type = Allocator;
    Memory resource;
    String mode, gate;
    bool ready = false, used = false, pending = false;
    std::uint64_t gets = 0, stops = 0;
    Json::Array events;
    WakeSignal* wake = nullptr;
    std::optional<RetainedIdSet> retained;

    Control(std::allocator_arg_t, Allocator a, std::string_view mode, std::string_view gate)
        : resource(a.resource()), mode(mode, resource), gate(gate, resource), events(resource) {}
};

class Store final : public IdSetStore {
public:
    using allocator_type = Allocator;
    Memory resource;
    Control& control;

    Store(std::allocator_arg_t, Allocator a, Control& control)
        : resource(a.resource()), control(control) {}

    Memory memory_resource() const noexcept override {
        return resource;
    }

    template <bool Read>
    struct Work {
        using allocator_type = Allocator;
        using Output = std::conditional_t<Read,
                                          PaginationResult<std::optional<RetainedIdSet>>,
                                          PaginationWriteResult>;
        Memory resource;
        ResourceBorrow<Store> store;
        Control& control;
        String key, gate;
        std::optional<RetainedIdSet> value;
        bool waiting = false;

        Work(std::allocator_arg_t,
             Allocator a,
             Store& store,
             std::string_view key,
             const RetainedIdSet* value)
            : resource(a.resource()),
              store(std::allocator_arg, a, store, detail::resource_arena(store.resource)),
              control(store.control), key(key, resource), gate(resource) {
            Json::Object event(control.resource);
            if constexpr (Read) {
                auto ordinal = ++control.gets;
                gate = "get";
                gate += std::to_string(ordinal);
                event.emplace("get", Json(control.resource, key));
                event.emplace("ordinal", Json(control.resource, ordinal));
            } else {
                gate = "put";
                this->value.emplace(std::allocator_arg, a, *value);
                event.emplace("put", Json(control.resource, key));
                Json::Array ids(control.resource);
                for (auto id : value->ids) {
                    ids.emplace_back(id);
                }
                event.emplace("ids", Json(control.resource, ids));
            }
            control.events.emplace_back(event);
        }

        std::optional<Output> poll(WakeSignal& wake) {
            (void)store.get();
            if (control.gate == gate && (!control.used || waiting)) {
                control.used = true;
                if (!control.ready) {
                    waiting = true;
                    control.pending = true;
                    control.wake = &wake;
                    return {};
                }
                waiting = false;
                control.pending = false;
                control.wake = nullptr;
            }
            if constexpr (Read) {
                if (control.mode == "error_get") {
                    return Output::err(resource, String("fixture ID store read failure", resource));
                }
                if (control.retained) {
                    return Output::ok(resource, detail::clone_in(resource, control.retained));
                }
                if (control.mode == "hit" || control.mode == "empty" ||
                    control.mode == "alternate") {
                    RetainedIdSet hit(std::allocator_arg, Allocator(resource));
                    hit.query_key = key;
                    if (control.mode == "hit") {
                        hit.ids = {10, 11, 12};
                    } else if (control.mode == "alternate") {
                        hit.ids = {12};
                    }
                    hit.expires_at = *SystemTime::now().checked_add_seconds(60);
                    return Output::ok(resource, std::move(hit));
                }
                return Output::ok(resource, std::optional<RetainedIdSet>{});
            } else {
                if (control.mode == "error_put") {
                    return Output::err(resource,
                                       String("fixture ID store write failure", resource));
                }
                control.retained.emplace(std::allocator_arg, Allocator(control.resource), *value);
                return Output::ok(resource, {});
            }
        }

        void finish_and_wait() noexcept {
            control.wake = nullptr;
        }

        void cancel_and_wait() noexcept {
            if (waiting) {
                ++control.stops;
            }
            waiting = false;
            control.pending = false;
            control.wake = nullptr;
        }
    };

    AsyncOperation<PaginationResult<std::optional<RetainedIdSet>>>&
    get(Memory r, std::string_view key) override {
        return make_operation<PaginationResult<std::optional<RetainedIdSet>>, Work<true>>(
            r, *this, key, nullptr);
    }

    AsyncOperation<PaginationWriteResult>& put(Memory r, const RetainedIdSet& value) override {
        return make_operation<PaginationWriteResult, Work<false>>(
            r, *this, value.query_key, &value);
    }

    AsyncOperation<PaginationWriteResult>& invalidate(Memory r, std::string_view key) override {
        Json::Object event(control.resource);
        event.emplace("invalidate", Json(control.resource, key));
        control.events.emplace_back(event);
        control.retained.reset();

        struct Done {
            using allocator_type = Allocator;
            Memory resource;

            Done(std::allocator_arg_t, Allocator a) : resource(a.resource()) {}

            std::optional<PaginationWriteResult> poll(WakeSignal&) {
                return PaginationWriteResult::ok(resource, {});
            }

            void finish_and_wait() noexcept {}

            void cancel_and_wait() noexcept {}
        };

        return make_operation<PaginationWriteResult, Done>(r);
    }
};

static Json observe(GlobalContext& global, UserContext& user_context, const Json& input) {
    teaql::test::initialize_dynamic_json_database(user_context);
    user_context.set_user_identifier("fixture");
    user_context.set_trace_id("page");
    auto r = user_context.memory_resource();
    auto& control = global.make<Control>(text(field(input, "mode")), text(field(input, "gate")));
    auto& store = user_context.make<Store>(control);
    user_context.set_id_set_store(user_context, store);
    auto Q = user_context.queries<cpp_dynamic_json::query_factory>();
    auto request = Q.orders()
                       .orderByIdAsc()
                       .optimizePaginationWithIdSetConfig("deferred_page", 60, 30)
                       .comment("what: inspect deferred typed pages")
                       .purpose("why: qualify ID store lifetime");
    auto offset = std::get<std::uint64_t>(field(input, "offset").data);
    auto limit = std::get<std::uint64_t>(field(input, "limit").data);
    auto& operation = request.executeForPage(user_context, offset, limit);
    auto* initial = operation.poll();
    for (unsigned i = 0; !initial && !control.pending && i < 8; ++i) {
        initial = operation.poll();
        if (initial || control.pending) {
            break;
        }
    }
    bool pending = control.pending;
    Json::Object out(r);
    out.emplace("pending", Json(r, pending));
    out.emplace("before", Json(r, control.events));
    if (pending && text(field(input, "action")) == "cancel") {
        operation.cancel();
        out.emplace("cancelled", Json(r, true));
    } else {
        control.ready = true;
        auto ticket = operation.wake_signal().ticket();
        if (control.wake) {
            control.wake->notify();
        }
        bool woke = operation.wake_signal().ticket() != ticket;
        const auto& result = initial ? *initial : operation.wait();
        if (result) {
            Json::Array rows(r);
            for (const auto& value : result.value().data) {
                Json::Array row(r);
                row.emplace_back(value.id());
                row.emplace_back(value.version());
                rows.emplace_back(row);
            }
            out.emplace("rows", Json(r, rows));
            out.emplace("total",
                        result.value().total_count ? Json(r, *result.value().total_count)
                                                   : Json(r));
            out.emplace("error", Json(r));
        } else {
            out.emplace("error", Json(r, result.error().to_string(r)));
        }
        out.emplace("woke", Json(r, woke));
        out.emplace("cancelled", Json(r, false));
    }
    const auto& retry = request.executeForPage(user_context, offset, limit).wait();
    Json::Object after(r);
    if (retry) {
        Json::Array rows(r);
        for (const auto& value : retry.value().data) {
            Json::Array row(r);
            row.emplace_back(value.id());
            row.emplace_back(value.version());
            rows.emplace_back(row);
        }
        after.emplace("rows", Json(r, rows));
        after.emplace("total",
                      retry.value().total_count ? Json(r, *retry.value().total_count) : Json(r));
    } else {
        after.emplace("error", Json(r, retry.error().to_string(r)));
    }
    out.emplace("retry", Json(r, after));
    out.emplace("events", Json(r, control.events));
    out.emplace("stops", Json(r, control.stops));
    return Json(r, out);
}

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

static void ownership(std::string_view gate, unsigned action) {
    GlobalContext global;
    auto user_context = global.make_user_context();
    teaql::test::initialize_dynamic_json_database(*user_context);
    auto& old_control = global.make<Control>("miss", gate);
    auto& next_control = global.make<Control>(action == 0 ? "alternate" : "miss", "none");
    auto old_store = user_context->make_ref<Store>(old_control);
    auto next_store = user_context->make_ref<Store>(next_control);
    user_context->set_id_set_store(*user_context, user_context->get(old_store));
    auto Q = user_context->queries<cpp_dynamic_json::query_factory>();
    auto request = Q.orders()
                       .orderByIdAsc()
                       .optimizePaginationWithIdSetConfig("ownership_page", 60, 30)
                       .comment("what: inspect pending store ownership")
                       .purpose("why: release locks and Context borrows");
    auto& operation = request.executeForPage(*user_context, 0, 1);
    check(!operation.poll() && old_control.pending, "typed page reaches the selected store gate");
    if (action == 3) {
        user_context.reset();
        check(old_control.stops == 1 && !old_control.pending && !old_control.wake,
              "actual Context deletion cancels the pending store and its lock exactly once");
        return;
    }
    user_context->set_id_set_store(*user_context, user_context->get(next_store));
    if (action == 0) {
        old_control.ready = true;
        old_control.wake->notify();
        const auto& result = operation.wait();
        check(result && result.value().total_count == 3 && next_control.gets == 0,
              "registration replacement cannot redirect an already pending ID-set page");
    } else if (action == 1) {
        user_context->destroy(old_store);
        bool rejected = false;
        try {
            operation.poll();
        } catch (const ReferenceError&) {
            rejected = true;
        }
        check(rejected && old_control.stops == 1 && !old_control.wake,
              "destroyed store rejects access and cancels its checked pending operation");
    } else {
        operation.cancel();
        user_context->destroy(old_store);
        check(old_control.stops == 1 && !old_control.wake,
              "explicit cancellation releases store work before destroying its owner");
    }
    const auto& reused = request.executeForPage(*user_context, 0, 1).wait();
    check(reused && reused.value().total_count == (action == 0 ? 1 : 3) && next_control.gets > 0,
          "new request selects the replacement store and reuses the released build lock");
    user_context.reset();
    check(old_control.stops == (action == 0 ? 0 : 1) && !old_control.wake && !next_control.wake,
          "Context close does not repeat completed or already cancelled store work");
}

int main(int argc, char** argv) {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        if (argc == 2 && std::string_view(argv[1]) == "--self-test") {
            for (auto gate : {"get1", "get2", "put"}) {
                for (unsigned action = 0; action < 4; ++action) {
                    ownership(gate, action);
                }
            }
            std::pmr::set_default_resource(previous);
            std::cout << "12 deferred page replacement/destruction scenarios passed\n";
            return 0;
        }
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            auto input = Json::parse(user_context->memory_resource(), line);
            if (!input) {
                throw std::runtime_error("invalid deferred page fixture");
            }
            std::cout
                << observe(global, *user_context, *input).dump(user_context->memory_resource())
                << '\n';
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
