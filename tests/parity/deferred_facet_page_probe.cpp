#include "../generated_dynamic_json/include/cpp_dynamic_json.hpp"
#include "fixture_value.hpp"
#include "../support/dynamic_json_runtime_fixture.hpp"
#include <teaql/core/query_debug.hpp>
#include <teaql/data_service/sql_log_debug.hpp>
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
        String host_buffer;
        bool waiting = false;
        AsyncOperation<Output>* child = nullptr;
        WakeSignal::Forward forwarding;

        Work(std::allocator_arg_t, Allocator a, Executor& executor, QueryRequest request)
            : resource(a.resource()), inner(std::allocator_arg, a, executor.inner),
              control(executor.control), request(std::allocator_arg, a, std::move(request)),
              phase(this->request.query.entity == "Person"   ? "facet"
                    : this->request.query.aggregates.empty() ? "rows"
                                                             : "count",
                    resource),
              host_buffer(32768, 'h', resource) {
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
            if (host_buffer.size() != 32768 || host_buffer.back() != 'h') {
                std::terminate();
            }
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

template <Entity T>
static Json snapshot(Memory r, const T& entity) {
    Json::Object output(r), loaded(r);
    auto descriptor = T::entity_descriptor(r);
    for (const auto& field : descriptor.properties) {
        loaded.emplace(String(field.name, r), Json(r, entity.is_field_loaded(field.name)));
    }
    for (const auto& relation : descriptor.relations) {
        loaded.emplace(String(relation.name, r), Json(r, entity.is_field_loaded(relation.name)));
    }
    T copy(std::allocator_arg, Allocator(r), entity);
    output.emplace("loaded", Json(r, loaded));
    output.emplace("json", std::move(copy).into_json(r));
    return Json(r, output);
}

template <class R>
static Json result_json(Memory r, const R& result) {
    Json::Object out(r);
    if (!result) {
        out.emplace("error", Json(r, result.error().to_string(r)));
    } else {
        Json::Array rows(r);
        Json::Object facets(r);
        const auto& value = result.value();
        if constexpr (requires { value.facets; }) {
            for (const auto& row : value.data) {
                rows.emplace_back(snapshot(r, row));
            }
            for (const auto& [name, list] : value.facets) {
                Json::Array values(r);
                for (const auto& row : list.data) {
                    Json::Object fields(r);
                    for (const auto& [key, item] : row.iter()) {
                        fields.emplace(String(key, r), Json(r, debug_string(r, item)));
                    }
                    values.emplace_back(fields);
                }
                facets.emplace(String(name, r), Json(r, values));
            }
            out.emplace("total", value.total_count ? Json(r, *value.total_count) : Json(r));
        } else {
            if (value) {
                rows.emplace_back(snapshot(r, *value));
            }
            out.emplace("total", Json(r));
        }
        out.emplace("rows", Json(r, rows));
        out.emplace("facets", Json(r, facets));
    }
    return Json(r, out);
}

static Json logs_json(UserContext& user_context) {
    auto r = user_context.memory_resource();
    Json::Array logs(r);
    for (const auto& entry : user_context.sql_logs()) {
        Json::Object log(r);
        Json::Array params(r), trace(r);
        for (const auto& value : entry.params) {
            params.emplace_back(debug_string(r, value));
        }
        for (const auto& node : entry.trace_path) {
            trace.emplace_back(debug_string(r, node));
        }
        log.emplace("sql", Json(r, entry.sql));
        log.emplace("params", Json(r, params));
        log.emplace("comment", entry.comment ? Json(r, *entry.comment) : Json(r));
        log.emplace("purpose", entry.purpose ? Json(r, *entry.purpose) : Json(r));
        log.emplace("debug_sql", Json(r, entry.debug_sql));
        log.emplace("pretty_sql", Json(r, entry.pretty_sql));
        log.emplace("context", Json(r, debug_string(r, entry.log_context)));
        log.emplace("trace", Json(r, trace));
        log.emplace("result_count", entry.result_count ? Json(r, *entry.result_count) : Json(r));
        log.emplace("result_type", entry.result_type ? Json(r, *entry.result_type) : Json(r));
        log.emplace("affected_rows", entry.affected_rows ? Json(r, *entry.affected_rows) : Json(r));
        log.emplace("summary", Json(r, entry.result_summary));
        logs.emplace_back(log);
    }
    return Json(r, logs);
}

static Control&
initialize(GlobalContext& global, UserContext& user_context, std::string_view gate) {
    teaql::test::initialize_dynamic_json_database(user_context);
    auto& control = global.make<Control>(gate);
    auto& executor =
        user_context.make<Executor>(user_context.require_resource<Inner>().value().get(), control);
    user_context.insert_resource(user_context, executor);
    auto& ids = user_context.make<InMemoryIdSetStore>();
    user_context.set_id_set_store(user_context, ids);
    auto& cursors = user_context.make<InMemoryContinuousPageCursorStore>();
    user_context.set_continuous_page_cursor_store(user_context, cursors);
    return control;
}

static auto composed_request(UserContext& user_context, const Json& input) {
    EntityQuery<cpp_dynamic_json::Person, Executor> facet(user_context, true);
    facet.orderAsc("id").comment("Inspect deferred customers");
    EntityQuery<cpp_dynamic_json::Order, Executor> query(user_context);
    query.orderAsc("id")
        .offsetOnly(std::get<std::uint64_t>(field(input, "offset").data))
        .limit(std::get<std::uint64_t>(field(input, "limit").data))
        .facetBy("customer", "_customers", facet, std::get<bool>(field(input, "include_all").data));
    auto optimization = text(field(input, "optimization"));
    if (optimization == "id_set") {
        query.optimizePaginationWithIdSetConfig("deferred_facets", 60, 100);
    } else if (optimization == "continuous") {
        query.optimizeForContinuousPageFetchWith("deferred_facets", 60);
    }
    return query.comment("Inspect deferred facet pages").purpose("Qualify composed query cleanup");
}

static Json observe(GlobalContext& global, UserContext& user_context, const Json& input) {
    auto& control = initialize(global, user_context, text(field(input, "gate")));
    auto r = user_context.memory_resource();
    auto query = composed_request(user_context, input);
    auto offset = std::get<std::uint64_t>(field(input, "offset").data);
    auto limit = std::get<std::uint64_t>(field(input, "limit").data);
    auto& connection =
        user_context.require_resource<SqliteMutationExecutor>().value().get().connection();
    auto sql = [&](const String& statement) {
        if (sqlite3_exec(
                connection.native_handle(), statement.c_str(), nullptr, nullptr, nullptr) !=
            SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(connection.native_handle()));
        }
    };
    auto run = [&](auto start) {
        auto& operation = start();
        auto* initial = operation.poll();
        bool pending = control.pending;
        Json::Object out(r);
        Json::Array before(r);
        for (const auto& event : control.events) {
            before.emplace_back(event);
        }
        out.emplace("pending", Json(r, pending));
        out.emplace("before", Json(r, before));
        auto action = text(field(input, "action"));
        String table(text(field(input, "gate")) == "facet" ? "person_data" : "order_data", r);
        if (pending && action == "cancel") {
            operation.cancel();
            out.emplace("cancelled", Json(r, true));
        } else {
            if (pending && action == "fail") {
                String statement("ALTER TABLE ", r);
                statement += table;
                statement += " RENAME TO unavailable_table";
                sql(statement);
            }
            control.ready = true;
            auto ticket = operation.wake_signal().ticket();
            if (control.wake) {
                control.wake->notify();
            }
            bool woke = operation.wake_signal().ticket() != ticket;
            out.emplace("result", result_json(r, initial ? *initial : operation.wait()));
            out.emplace("woke", Json(r, woke));
            out.emplace("cancelled", Json(r, false));
        }
        if (pending && action == "fail") {
            String statement("ALTER TABLE unavailable_table RENAME TO ", r);
            statement += table;
            sql(statement);
        }
        out.emplace("retry", result_json(r, start().wait()));
        Json::Array events(r);
        for (const auto& event : control.events) {
            events.emplace_back(event);
        }
        out.emplace("events", Json(r, events));
        out.emplace("stops", Json(r, control.stops));
        out.emplace("logs", logs_json(user_context));
        return Json(r, out);
    };
    auto mode = text(field(input, "mode"));
    if (mode == "list") {
        return run([&]() -> auto& {
            return query.executeForList(user_context);
        });
    }
    if (mode == "one") {
        return run([&]() -> auto& {
            return query.executeForOne(user_context);
        });
    }
    return run([&]() -> auto& {
        return query.executeForPage(user_context, offset, limit);
    });
}

static void destroyed_context(std::string_view gate,
                              std::string_view mode,
                              std::string_view optimization,
                              bool explicit_cancel) {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto& control = initialize(global, *user_context, gate);
    Json::Object input(global.memory_resource());
    auto r = global.memory_resource();
    input.emplace("offset", Json(r, std::uint64_t(0)));
    input.emplace("limit", Json(r, std::uint64_t(1)));
    input.emplace("optimization", Json(r, optimization));
    input.emplace("include_all", Json(r, true));
    auto query = composed_request(*user_context, Json(r, input));
    auto close = [&](auto& operation) {
        if (operation.poll() || !control.pending || !control.wake) {
            throw std::runtime_error("Composed request did not suspend at its selected boundary");
        }
        if (explicit_cancel) {
            operation.cancel();
        }
        user_context.reset();
        if (control.stops != 1 || control.pending || control.wake ||
            global.live_user_bytes() != 0) {
            throw std::runtime_error(
                "Composed child work did not stop before actual Context reclamation");
        }
    };
    if (mode == "list") {
        close(query.executeForList(*user_context));
    } else if (mode == "one") {
        close(query.executeForOne(*user_context));
    } else {
        close(query.executeForPage(*user_context, 0, 1));
    }
}

int main(int argc, char** argv) {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        if (argc == 2 && std::string_view(argv[1]) == "--self-test") {
            for (auto mode : {"list", "one", "page"}) {
                for (auto optimization : {"plain", "id_set", "continuous"}) {
                    for (auto gate : {"rows", "facet"}) {
                        for (bool cancel : {false, true}) {
                            destroyed_context(gate, mode, optimization, cancel);
                        }
                    }
                }
            }
            destroyed_context("count", "page", "plain", false);
            destroyed_context("count", "page", "plain", true);
            std::cout << "38 composed facet/page Context destruction scenarios passed\n";
        } else {
            for (std::string line; std::getline(std::cin, line);) {
                GlobalContext global;
                auto user_context = global.make_user_context();
                {
                    auto input = Json::parse(user_context->memory_resource(), line);
                    if (!input) {
                        throw std::runtime_error("Invalid deferred facet/page input");
                    }
                    std::cout << observe(global, *user_context, *input)
                                     .dump(user_context->memory_resource())
                              << '\n';
                }
                user_context.reset();
                if (global.live_user_bytes() != 0) {
                    throw std::runtime_error("Composed observer retained User memory");
                }
            }
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
    return 0;
}
