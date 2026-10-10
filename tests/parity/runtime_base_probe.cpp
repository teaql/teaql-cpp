#include <teaql/runtime/detail/data_service/data_service.hpp>
#include <teaql/core/detail/model_json.hpp>
#include <teaql/core/detail/debug.hpp>
#include "fixture_value.hpp"
#include "../support/entity_decoder_fixture.hpp"
#include <iostream>
using namespace teaql;

static bool flag(const Json& j, std::string_view key) {
    return std::get<bool>(field(j, key).data);
}

struct Spy : MetadataStore {
    using allocator_type = Allocator;
    Memory r;
    bool capture, debug;
    mutable Vector<std::size_t> logs;

    Spy(std::allocator_arg_t, Allocator a, bool c, bool d)
        : r(a.resource()), capture(c), debug(d), logs(r) {}

    Memory memory_resource() const noexcept override {
        return r;
    }

    const EntityDescriptor* entity(std::string_view) const override {
        return nullptr;
    }

    Vector<const EntityDescriptor*> all_entities(Memory target) const override {
        return Vector<const EntityDescriptor*>(target);
    }

    bool capture_execution_metadata() const override {
        return capture;
    }

    bool capture_query_debug() const override {
        return debug;
    }

    void record_metadata_log(const ExecutionMetadata& m) const override {
        logs.push_back(m.result_count.value_or(0));
    }
};

struct Mock : QueryExecutor<String>, MutationExecutor<String> {
    using allocator_type = Allocator;
    Memory r;
    bool failure, bad_model, event;
    Json seen;

    Mock(std::allocator_arg_t, Allocator a, const Json& input)
        : r(a.resource()), failure(flag(input, "failure")), bad_model(flag(input, "bad_model")),
          event(flag(input, "event")), seen(r) {}

    Memory memory_resource() const {
        return r;
    }

    DataServiceCapabilities capabilities() const override {
        return {};
    }

    struct Return {
        using allocator_type = Allocator;
        Result<QueryResult, String> value;

        Return(std::allocator_arg_t, Allocator a, Result<QueryResult, String> value)
            : value(std::allocator_arg, a, std::move(value)) {}

        std::optional<Result<QueryResult, String>> poll(WakeSignal&) {
            return std::move(value);
        }

        void finish_and_wait() noexcept {}

        void cancel_and_wait() noexcept {}
    };

    DataServiceOperationResult<QueryResult, String>& query(Memory r, QueryRequest q) override {
        return query_observed(r, std::move(q), {});
    }

    DataServiceOperationResult<QueryResult, String>& query_observed(
        Memory target, QueryRequest q, std::optional<ExecutionObserver> observer) override {
        seen = detail::model_json(r);
        detail::model_put(seen, "entity", q.query.entity);
        detail::model_put(seen, "trace", q.trace_chain.size());
        detail::model_put(seen, "comment", q.comment ? Json(r, *q.comment) : Json(r));
        detail::model_put(seen, "capture", q.capture_execution_metadata);
        detail::model_put(seen, "debug", q.capture_debug_query);
        detail::model_put(seen, "observer", observer.has_value());
        if (event && observer) {
            (*observer)(ExecutionMetadata::unrecorded_query(target, 99));
        }
        using R = Result<QueryResult, String>;
        if (failure) {
            return make_operation<R, Return>(target,
                                             R::err(target, String("fetch failed", target)));
        }
        QueryResult result(
            std::allocator_arg, Allocator(target), ExecutionMetadata::unrecorded_query(target, 1));
        Record values(target);
        values.emplace(String("id", target), Value(target, std::uint64_t(1)));
        values.emplace(String("fail", target), Value(target, bad_model));
        result.rows.emplace_back(values);
        return make_operation<R, Return>(target, R::ok(target, std::move(result)));
    }

    DataServiceOperationResult<MutationResult, String>& mutate(Memory, MutationRequest) override {
        throw std::logic_error("unused");
    }
};

static Json evaluate(UserContext& c, const Json& j) {
    auto r = c.memory_resource();
    auto& spy = c.make<Spy>(flag(j, "capture"), flag(j, "debug"));
    auto& mock = c.make<Mock>(j);
    ResourceBorrow<Mock> executor(
        std::allocator_arg, Allocator(r), mock, detail::resource_arena(r));
    auto& base = c.make<detail::RuntimeDataService<Spy, Mock>>(spy, executor);
    auto& q = c.make<SelectQuery>("Item");
    q.comment_text.emplace("comment", r);
    if (flag(j, "trace")) {
        q.trace_chain.emplace_back(TraceKind::Request, "Item", "trace");
    }
    Json output = detail::model_json(r);
    auto collect = [&](const auto& result, bool loaded) {
        if (!result) {
            detail::model_put(output, "error", result.error().to_string(r));
            return;
        }
        Json::Array rows(r);
        for (const auto& item : result.value()) {
            if constexpr (std::same_as<std::decay_t<decltype(item)>, CompactRow>) {
                rows.emplace_back(detail::debug_value(r, *item.get("id")));
            } else {
                rows.emplace_back(detail::debug_value(r, item.id_value()));
            }
        }
        detail::model_put(output, "rows", rows);
        detail::model_put(output, "loaded", loaded);
    };
    auto mode = text(field(j, "mode"));
    if (mode == "all") {
        collect(base.fetch_all(q).wait(), false);
    } else if (mode == "list") {
        collect(base.fetch_smart_list(q).wait(), true);
    } else if (mode == "entities") {
        collect(base.fetch_entities<decoder_fixture::Item<>>(q).wait(), true);
    } else {
        collect(base.fetch_enhanced_entities<decoder_fixture::Item<>>(q).wait(), true);
    }
    Json::Array logs(r);
    for (auto n : spy.logs) {
        logs.emplace_back(n);
    }
    detail::model_put(output, "logs", logs);
    detail::model_put(output, "request", mock.seen);
    return output;
}

int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        GlobalContext g;
        auto c = g.make_user_context();
        auto r = c->memory_resource();
        auto j = Json::parse(r, line);
        std::cout << evaluate(*c, *j).dump(r) << '\n';
    }
}
