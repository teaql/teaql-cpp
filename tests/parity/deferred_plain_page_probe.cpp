#include "../generated_dynamic_json/include/cpp_dynamic_json.hpp"
#include "fixture_value.hpp"
#include "../support/dynamic_json_runtime_fixture.hpp"
#include <iostream>

using namespace teaql;
using Inner = cpp_dynamic_json::ServiceRuntimeExecutor;
using Error = typename Inner::Error;

struct Control {
    using allocator_type = Allocator;
    Memory resource;
    String gate;
    bool ready = false, used = false, pending = false;
    std::uint64_t stops = 0;
    Vector<String> events;
    WakeSignal* wake = nullptr;

    Control(std::allocator_arg_t, Allocator a, std::string_view gate)
        : resource(a.resource()), gate(gate, resource), events(resource) {}
};

struct Executor final : QueryExecutor<Error>, MutationExecutor<Error> {
    using allocator_type = Allocator;
    ResourceBorrow<Inner> inner;
    Control& control;

    Executor(std::allocator_arg_t, Allocator a, Inner& inner, Control& control)
        : inner(std::allocator_arg, a, inner, detail::resource_arena(a.resource())),
          control(control) {}

    DataServiceCapabilities capabilities() const override {
        return inner.get().capabilities();
    }

    struct Work {
        using allocator_type = Allocator;
        using Output = Result<QueryResult, Error>;
        Memory resource;
        ResourceBorrow<Inner> inner;
        Control& control;
        QueryRequest request;
        String phase;
        bool waiting = false;
        AsyncOperation<Output>* child = nullptr;
        WakeSignal::Forward forwarding;

        Work(std::allocator_arg_t, Allocator a, Executor& executor, QueryRequest request)
            : resource(a.resource()), inner(std::allocator_arg, a, executor.inner),
              control(executor.control), request(std::allocator_arg, a, std::move(request)),
              phase(this->request.query.aggregates.empty() ? "rows" : "count", resource) {
            control.events.emplace_back(phase);
        }

        std::optional<Output> poll(WakeSignal& wake) {
            if (control.gate == phase && (!control.used || waiting)) {
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
            if (!child) {
                child = &inner.get().query(resource, std::move(request));
            }
            forwarding.attach(child->wake_signal(), wake);
            if (!child->poll()) {
                return {};
            }
            forwarding.reset();
            return child->take_result();
        }

        void finish_and_wait() noexcept {
            forwarding.reset();
            control.wake = nullptr;
        }

        void cancel_and_wait() noexcept {
            if (waiting) {
                ++control.stops;
            }
            if (child) {
                child->cancel();
            }
            waiting = false;
            control.pending = false;
            control.wake = nullptr;
            forwarding.reset();
        }
    };

    DataServiceOperationResult<QueryResult, Error>& query(Memory r, QueryRequest request) override {
        return make_operation<Result<QueryResult, Error>, Work>(r, *this, std::move(request));
    }

    DataServiceOperationResult<MutationResult, Error>& mutate(Memory r,
                                                              MutationRequest request) override {
        return inner.get().mutate(r, std::move(request));
    }
};

template <class R>
static Json page(Memory r, const R& result) {
    Json::Object out(r);
    if (!result) {
        out.emplace("error", Json(r, result.error().to_string(r)));
    } else {
        Json::Array rows(r);
        for (const auto& value : result.value().data) {
            Json::Array row(r);
            row.emplace_back(value.id());
            row.emplace_back(value.version());
            rows.emplace_back(row);
        }
        out.emplace("rows", Json(r, rows));
        out.emplace("total",
                    result.value().total_count ? Json(r, *result.value().total_count) : Json(r));
    }
    return Json(r, out);
}

static Json observe(GlobalContext& global, UserContext& user_context, const Json& input) {
    teaql::test::initialize_dynamic_json_database(user_context);
    auto r = user_context.memory_resource();
    auto& control = global.make<Control>(text(field(input, "gate")));
    auto& executor =
        user_context.make<Executor>(user_context.require_resource<Inner>().value().get(), control);
    user_context.insert_resource(user_context, executor);
    EntityQuery<cpp_dynamic_json::Order, Executor> entry(user_context);
    auto request = entry.orderAsc("id")
                       .comment("what: inspect ordinary deferred pages")
                       .purpose("why: qualify count and list cleanup");
    auto offset = std::get<std::uint64_t>(field(input, "offset").data);
    auto limit = std::get<std::uint64_t>(field(input, "limit").data);
    auto& operation = request.executeForPage(user_context, offset, limit);
    auto* initial = operation.poll();
    bool pending = control.pending;
    Json::Object out(r);
    out.emplace("pending", Json(r, pending));
    Json::Array before(r);
    for (const auto& event : control.events) {
        before.emplace_back(event);
    }
    out.emplace("before", Json(r, before));
    auto& connection =
        user_context.require_resource<SqliteMutationExecutor>().value().get().connection();
    auto fail = [&](const char* sql) {
        if (sqlite3_exec(connection.native_handle(), sql, nullptr, nullptr, nullptr) != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(connection.native_handle()));
        }
    };
    auto action = text(field(input, "action"));
    if (pending && action == "cancel") {
        operation.cancel();
        out.emplace("cancelled", Json(r, true));
    } else {
        if (pending && action == "fail") {
            fail("ALTER TABLE order_data RENAME TO unavailable_order");
        }
        control.ready = true;
        auto ticket = operation.wake_signal().ticket();
        if (control.wake) {
            control.wake->notify();
        }
        bool woke = operation.wake_signal().ticket() != ticket;
        const auto& result = initial ? *initial : operation.wait();
        out.emplace("result", page(r, result));
        out.emplace("woke", Json(r, woke));
        out.emplace("cancelled", Json(r, false));
    }
    if (pending && action == "fail") {
        fail("ALTER TABLE unavailable_order RENAME TO order_data");
    }
    out.emplace("retry", page(r, request.executeForPage(user_context, offset, limit).wait()));
    Json::Array events(r);
    for (const auto& event : control.events) {
        events.emplace_back(event);
    }
    out.emplace("events", Json(r, events));
    out.emplace("stops", Json(r, control.stops));
    return Json(r, out);
}

static void destroyed_context(std::string_view gate, bool explicit_cancel) {
    GlobalContext global;
    auto user_context = global.make_user_context();
    teaql::test::initialize_dynamic_json_database(*user_context);
    auto& control = global.make<Control>(gate);
    auto& executor = user_context->make<Executor>(
        user_context->require_resource<Inner>().value().get(), control);
    user_context->insert_resource(*user_context, executor);
    EntityQuery<cpp_dynamic_json::Order, Executor> entry(*user_context);
    auto request = entry.orderAsc("id")
                       .comment("what: close a deferred ordinary page")
                       .purpose("why: reclaim pending count or row work");
    auto& operation = request.executeForPage(*user_context, 1, 1);
    if (operation.poll() || !control.pending || !control.wake) {
        throw std::runtime_error("ordinary page did not suspend at its actual query boundary");
    }
    if (explicit_cancel) {
        operation.cancel();
    }
    user_context.reset();
    if (control.stops != 1 || control.pending || control.wake) {
        throw std::runtime_error(
            "count/list work was not cancelled exactly once before Context reclamation");
    }
}

int main(int argc, char** argv) {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        if (argc == 2 && std::string_view(argv[1]) == "--self-test") {
            for (auto gate : {"count", "rows"}) {
                destroyed_context(gate, false);
                destroyed_context(gate, true);
            }
            std::pmr::set_default_resource(previous);
            std::cout << "4 ordinary page count/list Context destruction scenarios passed\n";
            return 0;
        }
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto user_context = global.make_user_context();
            auto input = Json::parse(user_context->memory_resource(), line);
            if (!input) {
                throw std::runtime_error("invalid deferred plain page fixture");
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
