#include <teaql/runtime/detail/data_service/entity_query.hpp>
#include <teaql/core/detail/model_json.hpp>
#include <iostream>
#include <string>
#include <vector>

using namespace teaql;
using Access = detail::EntityDataServiceAccess;

struct State {
    std::vector<std::string> events, active;
    std::uint64_t classified = 0;

    std::string path() const {
        std::string result;
        for (const auto& value : active) {
            if (!result.empty()) {
                result += '/';
            }
            result += value;
        }
        return result;
    }
};

struct Scope final : RuntimeTelemetryScope {
    using allocator_type = Allocator;
    State& state;
    String family;

    Scope(std::allocator_arg_t, Allocator a, State& state, std::string_view family)
        : state(state), family(family, a.resource()) {}

    void with_context(FunctionRef<void()> callback) const override {
        state.active.emplace_back(family);

        struct Leave {
            State& state;

            ~Leave() {
                state.active.pop_back();
            }
        } leave{state};

        callback();
    }

    void success(const RuntimeAttributes& attrs) override {
        std::string outcome;
        if (auto i = attrs.find("teaql.cache.result"); i != attrs.end()) {
            outcome = std::get<String>(i->second.data);
        }
        state.events.emplace_back("success:" + std::string(family) + ":" + outcome);
    }

    void failure(std::string_view error) override {
        state.events.emplace_back("failure:" + std::string(family) + ":" + std::string(error));
    }
};

struct Telemetry final : RuntimeTelemetry {
    using allocator_type = Allocator;
    Memory r;
    State& state;

    Telemetry(std::allocator_arg_t, Allocator a, State& state) : r(a.resource()), state(state) {}

    Memory memory_resource() const noexcept override {
        return r;
    }

    ContextBox<RuntimeTelemetryScope>& start(Memory target,
                                             const RuntimeOperation& op) const override {
        state.events.emplace_back("start:" + std::string(op.family) + ":" + state.path());
        return detail::resource_arena(target).make<ContextBox<RuntimeTelemetryScope>>(
            std::in_place_type<Scope>, state, std::string_view(op.family));
    }
};

static Vector<CompactRow> rows(Memory r, std::string_view label) {
    Vector<CompactRow> result(r);
    result.emplace_back();
    result.back().insert("source", Value(r, label));
    return result;
}

struct Cache final : AggregationCacheBackend {
    using allocator_type = Allocator;
    Memory r;
    State& state;
    bool hit;

    Cache(std::allocator_arg_t, Allocator a, State& state, bool hit)
        : r(a.resource()), state(state), hit(hit) {}

    Memory memory_resource() const noexcept override {
        return r;
    }

    std::string_view name_space() const noexcept override {
        return "custom";
    }

    std::optional<Vector<CompactRow>>
    get(Memory target, std::string_view, std::uint64_t age) const override {
        state.events.emplace_back("get:" + std::to_string(age) + ":" + state.path());
        if (hit) {
            return Vector<CompactRow>(target);
        }
        return {};
    }

    void put(std::string_view, std::span<const CompactRow> value) override {
        state.events.emplace_back("put:" + std::to_string(value.size()) + ":" + state.path());
    }

    void invalidate_namespace(std::string_view) override {}
};

struct Executor final : QueryExecutor<String>, MutationExecutor<String> {
    using allocator_type = Allocator;
    Memory r;
    State& state;
    bool fail;

    Executor(std::allocator_arg_t, Allocator a, State& state, bool fail)
        : r(a.resource()), state(state), fail(fail) {}

    Memory memory_resource() const {
        return r;
    }

    DataServiceCapabilities capabilities() const override {
        return {};
    }

    SqlIntentRedactions query_log_intent(Memory target, const SelectQuery&) const override {
        ++state.classified;
        SqlIntentRedactions value(target);
        value.capture_target_id(Value(target, "classified"));
        return value;
    }

    struct Ready {
        using allocator_type = Allocator;
        Memory r;
        bool fail;

        Ready(std::allocator_arg_t, Allocator a, bool fail) : r(a.resource()), fail(fail) {}

        std::optional<Result<QueryResult, String>> poll(WakeSignal&) {
            if (fail) {
                return Result<QueryResult, String>::err(r, String("provider failed", r));
            }
            QueryResult result(
                std::allocator_arg, Allocator(r), ExecutionMetadata::unrecorded_query(r, 1));
            result.rows = rows(r, "provider");
            return Result<QueryResult, String>::ok(r, std::move(result));
        }

        void finish_and_wait() noexcept {}

        void cancel_and_wait() noexcept {}
    };

    DataServiceOperationResult<QueryResult, String>& query(Memory target, QueryRequest) override {
        state.events.emplace_back("query:" + state.path());
        return make_operation<Result<QueryResult, String>, Ready>(target, fail);
    }

    DataServiceOperationResult<MutationResult, String>& mutate(Memory, MutationRequest) override {
        throw std::logic_error("unexpected mutation");
    }
};

int main() {
    std::uint64_t case_index = 0;
    for (std::string_view entry : {"borrowed", "owned", "compact"}) {
        for (std::string_view backend : {"none", "concrete", "custom", "both"}) {
            for (bool enabled : {false, true}) {
                for (std::string_view outcome : {"hit", "miss", "failure"}) {
                    for (bool capture : {false, true}) {
                        for (bool scoped : {false, true}) {
                            State state;
                            GlobalContext global;
                            auto user_context = global.make_user_context();
                            auto r = user_context->memory_resource();
                            auto& telemetry = user_context->make<Telemetry>(state);
                            user_context->with_runtime_telemetry(*user_context, telemetry);
                            user_context->set_sql_log_options(SqlLogOptions{capture, false});
                            SelectQuery query(std::allocator_arg, Allocator(r), "Item");
                            query.comment("inspect cache execution")
                                .purpose("verify backend and telemetry contracts");
                            if (enabled) {
                                query.enable_aggregation_cache();
                                query.aggregation_cache->cache_expired_millis = 0;
                            }
                            if (backend == "concrete" || backend == "both") {
                                auto& cache =
                                    user_context->make<InMemoryAggregationCache>("concrete");
                                if (outcome == "hit" || backend == "both") {
                                    auto key = detail::aggregation_cache_key(
                                        r,
                                        "concrete",
                                        detail::aggregation_cache_namespace(r, "Item"),
                                        query);
                                    cache.put(key, rows(r, "concrete"));
                                }
                                user_context->insert_resource(*user_context, cache);
                            }
                            if (backend == "custom" || backend == "both") {
                                auto& cache = user_context->make<Cache>(state, outcome == "hit");
                                user_context->insert_resource(
                                    *user_context, static_cast<AggregationCacheBackend&>(cache));
                            }
                            auto& executor =
                                user_context->make<Executor>(state, outcome == "failure");
                            auto& service = EntityDataService<Executor>::for_executor(
                                *user_context, "Item", *user_context, executor);
                            if (scoped) {
                                auto value = Access::query_scoped(service, query);
                                service = std::move(value).value();
                            }
                            auto& operation =
                                make_operation<detail::RelationRowsResult<Executor>,
                                               detail::PreparedEntityQueryOperation<Executor>>(
                                    r, service, query, entry == "owned", entry == "compact");
                            auto& result = operation.wait();
                            auto output = detail::model_json(r);
                            detail::model_put(output, "case", case_index++);
                            detail::model_put(output, "entry", entry);
                            detail::model_put(output, "backend", backend);
                            detail::model_put(output, "enabled", enabled);
                            detail::model_put(output, "outcome", outcome);
                            detail::model_put(output, "capture", capture);
                            detail::model_put(output, "scoped", scoped);
                            detail::model_put(output, "classified", state.classified);
                            if (result) {
                                Json::Array values(r);
                                for (const auto& row : result.value()) {
                                    values.emplace_back(std::get<String>(row.get("source")->data));
                                }
                                detail::model_put(output, "values", Json(r, values));
                                detail::model_put(output, "error", Json(r));
                            } else {
                                detail::model_put(output, "values", Json(r));
                                detail::model_put(output, "error", result.error().to_string(r));
                            }
                            Json::Array events(r);
                            for (const auto& event : state.events) {
                                events.emplace_back(std::string_view(event));
                            }
                            detail::model_put(output, "events", Json(r, events));
                            std::cout << output.dump(r) << '\n';
                        }
                    }
                }
            }
        }
    }
}
