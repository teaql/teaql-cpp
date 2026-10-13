#include <teaql/sql/executor.hpp>
#include "fixture_sql.hpp"
#include <iostream>

static bool flag(const Json& j, std::string_view key, bool fallback = false) {
    auto p = optional(j, key);
    return p ? std::get<bool>(p->data) : fallback;
}

static Json values_json(Memory r, std::span<const Value> values) {
    Json::Array out(r);
    for (const auto& v : values) {
        out.emplace_back(v.to_json_value(r));
    }
    return Json(r, std::move(out));
}

static Json metadata_json(Memory r, const ExecutionMetadata& m) {
    auto out = detail::model_json(r);
    detail::model_put(out, "backend", m.backend);
    detail::model_put(out, "operation", unsigned(m.operation));
    detail::model_put(out,
                      "outcome",
                      m.sql_log.execution_outcome ? Json(r, unsigned(*m.sql_log.execution_outcome))
                                                  : Json(r));
    detail::model_put(out, "count", m.result_count ? Json(r, *m.result_count) : Json(r));
    detail::model_put(out, "comment", m.comment ? Json(r, *m.comment) : Json(r));
    detail::model_put(
        out, "sql", m.parameterized_query ? Json(r, *m.parameterized_query) : Json(r));
    detail::model_put(out, "debug", m.debug_query ? Json(r, *m.debug_query) : Json(r));
    detail::model_put(out, "params", values_json(r, m.params));
    Json::Array policies(r);
    for (auto p : m.sql_log.parameter_policies) {
        policies.emplace_back(unsigned(p));
    }
    detail::model_put(out, "policies", policies);
    detail::model_put(out, "generated", m.sql_log.generated_sql);
    detail::model_put(out,
                      "epoch",
                      m.started_at == ExecutionMetadata::SystemTime{} &&
                          m.ended_at == ExecutionMetadata::SystemTime{});
    detail::model_put(out, "trace", m.trace_chain.size());
    return out;
}

struct Dialect : FixtureDialect {
    mutable unsigned compiles = 0;
    bool probes;

    explicit Dialect(bool probes) : probes(probes) {}

    bool prefers_small_parent_relation_probes() const override {
        return probes;
    }

    CompiledQuery
    compile_select(Memory r, const EntityDescriptor& e, const SelectQuery& q) const override {
        ++compiles;
        return FixtureDialect::compile_select(r, e, q);
    }
};

struct Schema : SchemaProvider {
    const EntityDescriptor& descriptor;
    mutable unsigned lookups = 0;

    explicit Schema(const EntityDescriptor& e) : descriptor(e) {}

    const EntityDescriptor* get_entity(std::string_view name) const override {
        ++lookups;
        return name == descriptor.name ? &descriptor : nullptr;
    }
};

struct Transport : SqlTransport<String> {
    using allocator_type = Allocator;
    Memory resource;
    String mode;
    Json::Array calls;

    Transport(std::allocator_arg_t, Allocator a)
        : resource(a.resource()), mode(a.resource()), calls(a.resource()) {}

    struct Fetch {
        using allocator_type = Allocator;
        Memory resource;
        String mode;

        Fetch(std::allocator_arg_t, Allocator a, std::string_view mode)
            : resource(a.resource()), mode(mode, a.resource()) {}

        std::optional<Result<Rows, String>> poll(WakeSignal&) {
            if (mode == "pending") {
                return {};
            }
            if (mode == "panic") {
                throw std::runtime_error("provider panic");
            }
            if (mode == "error") {
                return Result<Rows, String>::err(resource, String("fetch failed", resource));
            }
            return Result<Rows, String>::ok(resource, Rows(resource));
        }

        void finish_and_wait() noexcept {}

        void cancel_and_wait() noexcept {}
    };

    void record(const CompiledQuery& q,
                std::optional<std::size_t> index,
                std::span<const Value> values) {
        auto out = detail::model_json(resource);
        detail::model_put(out, "sql", q.sql);
        detail::model_put(out, "params", values_json(resource, q.params));
        detail::model_put(out, "comment", q.comment ? Json(resource, *q.comment) : Json(resource));
        detail::model_put(out, "index", index ? Json(resource, *index) : Json(resource));
        detail::model_put(out, "values", values_json(resource, values));
        calls.emplace_back(std::move(out));
    }

    DataServiceOperationResult<Rows, String>&
    fetch_all_compact_sql(Memory r, const CompiledQuery& q) override {
        record(q, {}, {});
        return make_operation<Result<Rows, String>, Fetch>(r, mode);
    }

    DataServiceOperationResult<Rows, String>& fetch_repeated_compact_sql(
        Memory r, const CompiledQuery& q, std::size_t i, std::span<const Value> v) override {
        record(q, i, v);
        return make_operation<Result<Rows, String>, Fetch>(r, mode);
    }

    DataServiceOperationResult<std::uint64_t, String>& execute_sql(Memory,
                                                                   const CompiledQuery&) override {
        throw std::logic_error("unused fixture path");
    }
};

static Json evaluate(UserContext& user, const Json& j) {
    auto r = user.memory_resource();
    auto e = entity(r, field(j, "entity"));
    auto& dialect = user.make_resource<Dialect>(flag(j, "probes", true));
    auto& schema = user.make_resource<Schema>(e);
    auto& transport = user.make<Transport>();
    auto& state = user.make<SqlExecutorState>();
    auto& executor = user.make<SqlDataServiceExecutor<String>>(dialect, transport, schema, state);
    auto& clone = user.make<SqlDataServiceExecutor<String>>(executor);
    auto& events = user.make<Json::Array>();
    auto observer =
        ExecutionObserver(std::allocator_arg, Allocator(r), [r, &events](ExecutionMetadata m) {
            events.emplace_back(metadata_json(r, m));
        });
    Json::Array results(r);
    for (const auto& item : items(j, "requests")) {
        auto q = query(r, field(item, "query"));
        q.entity =
            optional(item, "entity") ? text(field(item, "entity")) : std::string_view(e.name);
        if (auto p = optional(item, "threshold")) {
            q.top_n_probe_parent_threshold_value = integer(*p);
        }
        if (auto p = optional(item, "hard_limit")) {
            q.hard_limit_value = integer(*p);
        }
        transport.mode = optional(item, "mode") ? text(field(item, "mode")) : "success";
        auto& active = results.size() % 2 ? clone : executor;
        if (flag(item, "intent")) {
            (void)active.query_log_intent(r, q);
        }
        auto comment = optional(item, "comment");
        auto intent = QueryIntent::from_optional(
            r,
            comment          ? std::optional<std::string_view>(text(*comment))
            : q.comment_text ? std::optional<std::string_view>(*q.comment_text)
                             : std::nullopt,
            "inspect SQL query fixture");
        QueryRequest request(std::allocator_arg,
                             Allocator(r),
                             q,
                             intent.value(),
                             flag(item, "debug"),
                             flag(item, "metadata", true));
        if (flag(item, "trace")) {
            request.trace_chain.emplace_back(TraceKind::Request, "Fixture", "request trace");
        }
        auto out = detail::model_json(r);
        auto& op = active.query_observed(r, std::move(request), observer);
        try {
            auto* result = op.poll();
            if (!result) {
                op.cancel();
                detail::model_put(out, "pending", true);
            } else if (*result) {
                detail::model_put(out, "metadata", metadata_json(r, result->value().metadata));
            } else {
                detail::model_put(out, "error", result->error().to_string(r));
            }
        } catch (const std::runtime_error&) {
            detail::model_put(out, "panic", true);
        }
        detail::model_put(out, "compiles", dialect.compiles);
        detail::model_put(out, "lookups", schema.lookups);
        results.emplace_back(std::move(out));
    }
    auto out = detail::model_json(r);
    detail::model_put(out, "results", results);
    detail::model_put(out, "calls", transport.calls);
    detail::model_put(out, "events", events);
    return out;
}

int main() {
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user = global.make_user_context();
        auto r = user->memory_resource();
        auto j = Json::parse(r, line);
        if (!j) {
            throw std::runtime_error("invalid fixture");
        }
        std::cout << evaluate(*user, *j).dump(r) << '\n';
    }
}
