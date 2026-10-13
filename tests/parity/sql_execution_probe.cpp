#include <teaql/sql/executor.hpp>
#include "fixture_sql.hpp"
#include <iostream>

static MutationRequest mutation(Memory r, const Json& j, std::string_view fallback) {
    auto mode = text(field(j, "mode"));
    auto reason = text(field(j, "comment"));
    auto name = optional(j, "entity") ? text(field(j, "entity")) : fallback;
    auto request = [&](auto&& command) {
        MutationRequest result(
            std::allocator_arg, Allocator(r), std::forward<decltype(command)>(command), reason);
        if (auto* values = optional(j, "trusted_redactions")) {
            auto& redactions = detail::async_arena(r).make<SqlIntentRedactions>();
            for (const auto& input : std::get<Json::Array>(values->data)) {
                redactions.capture_target_id(value(r, input));
            }
            result.with_diagnostic_redactions(&redactions);
        }
        return result;
    };
    if (mode == "batch") {
        MutationRequest::Batch batch(r);
        for (const auto& child : items(j, "children")) {
            batch.emplace_back(mutation(r, child, fallback));
        }
        return request(std::move(batch));
    }
    auto decorate = [&](auto& c) {
        if (auto p = optional(j, "trace")) {
            c.audit_as(text(*p));
        }
        for (const auto& node : items(j, "trace_nodes")) {
            auto* id = optional(node, "id");
            c.trace_chain.emplace_back(static_cast<TraceKind>(integer(field(node, "kind"))),
                                       text(field(node, "entity")),
                                       text(field(node, "comment")),
                                       id && !std::holds_alternative<std::monostate>(id->data)
                                           ? std::optional<std::uint64_t>(integer(*id))
                                           : std::nullopt);
        }
        if constexpr (std::same_as<std::remove_cvref_t<decltype(c)>, UpdateCommand>) {
            if (auto* old = optional(j, "old_values")) {
                c.old_values.emplace(r);
                for (const auto& [key, input] : std::get<Json::Object>(old->data)) {
                    c.old_values->fields.emplace(String(key, r), value(r, input));
                }
            }
        }
    };
    auto fields = [&](MutationValues& v) {
        for (const auto& [k, x] : std::get<Json::Object>(field(j, "values").data)) {
            v.fields.emplace(String(k, r), value(r, x));
        }
    };
    auto version = [&]() -> std::optional<std::int64_t> {
        auto p = optional(j, "version");
        return p && std::holds_alternative<String>(p->data)
                   ? std::optional(number<std::int64_t>(*p))
                   : std::nullopt;
    };
    if (mode == "insert") {
        InsertCommand c(std::allocator_arg, Allocator(r), name);
        fields(c.values);
        decorate(c);
        return request(std::move(c));
    }
    if (mode == "update") {
        UpdateCommand c(std::allocator_arg, Allocator(r), name, value(r, field(j, "id")));
        fields(c.values);
        c.expected_version_value = version();
        decorate(c);
        return request(std::move(c));
    }
    if (mode == "delete") {
        DeleteCommand c(std::allocator_arg, Allocator(r), name, value(r, field(j, "id")));
        c.soft_delete = std::get<bool>(field(j, "soft").data);
        c.expected_version_value = version();
        decorate(c);
        return request(std::move(c));
    }
    RecoverCommand c(std::allocator_arg, Allocator(r), name, value(r, field(j, "id")), *version());
    decorate(c);
    return request(std::move(c));
}

static Json values_json(Memory r, std::span<const Value> v) {
    Json::Array out(r);
    for (const auto& x : v) {
        out.emplace_back(x.to_json_value(r));
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
    detail::model_put(out, "affected", m.affected_rows ? Json(r, *m.affected_rows) : Json(r));
    detail::model_put(out, "count", m.result_count ? Json(r, *m.result_count) : Json(r));
    detail::model_put(out, "comment", m.comment ? Json(r, *m.comment) : Json(r));
    detail::model_put(
        out, "sql", m.parameterized_query ? Json(r, *m.parameterized_query) : Json(r));
    detail::model_put(out, "debug", m.debug_query ? Json(r, *m.debug_query) : Json(r));
    detail::model_put(out, "params", values_json(r, m.params));
    detail::model_put(out, "trace", m.trace_chain.size());
    Json::Array trace_nodes(r);
    for (const auto& node : m.trace_chain) {
        auto item = detail::model_json(r);
        detail::model_put(item, "kind", unsigned(node.kind));
        detail::model_put(item, "entity", node.entity_type);
        detail::model_put(item, "id", node.entity_id ? Json(r, *node.entity_id) : Json(r));
        detail::model_put(item, "comment", node.comment);
        trace_nodes.emplace_back(std::move(item));
    }
    detail::model_put(out, "trace_nodes", trace_nodes);
    detail::model_put(out, "generated", m.sql_log.generated_sql);
    Json::Array policies(r), statements(r);
    for (auto p : m.sql_log.parameter_policies) {
        policies.emplace_back(unsigned(p));
    }
    for (const auto& s : m.statements) {
        statements.emplace_back(metadata_json(r, s));
    }
    detail::model_put(out, "policies", policies);
    detail::model_put(out, "statements", statements);
    for (bool debug : {false, true}) {
        Vector<String> secrets(r);
        m.sql_log.intent_redactions.extend_secrets(debug, secrets);
        Json::Array v(r);
        for (const auto& s : secrets) {
            v.emplace_back(s);
        }
        detail::model_put(out, debug ? "secrets_debug" : "secrets", v);
    }
    return out;
}

struct Schema : SchemaProvider {
    const EntityDescriptor& e;

    explicit Schema(const EntityDescriptor& e) : e(e) {}

    const EntityDescriptor* get_entity(std::string_view n) const override {
        return n == e.name ? &e : nullptr;
    }
};

struct Transport : SqlTransactionTransport<String> {
    using allocator_type = Allocator;
    Memory resource;
    const Json& input;
    Json::Array calls;

    Transport(std::allocator_arg_t, Allocator a, const Json& j)
        : resource(a.resource()), input(j), calls(a.resource()) {}

    template <class T>
    struct Call {
        using allocator_type = Allocator;
        Memory resource;
        String mode;
        std::uint64_t count;

        Call(std::allocator_arg_t, Allocator a, std::string_view mode, std::uint64_t count)
            : resource(a.resource()), mode(mode, a.resource()), count(count) {}

        std::optional<Result<T, String>> poll(WakeSignal&) {
            if (mode == "pending") {
                return {};
            }
            if (mode == "panic") {
                throw std::runtime_error("provider panic");
            }
            if (mode == "error") {
                return Result<T, String>::err(resource, String("provider failed", resource));
            }
            if constexpr (std::same_as<T, std::uint64_t>) {
                return Result<T, String>::ok(resource, count);
            } else {
                T rows(resource);
                for (std::uint64_t i = 0; i < count; ++i) {
                    rows.emplace_back();
                    rows.back().insert("id", Value(resource, std::uint64_t(7)));
                    rows.back().insert("saved", Value(resource, "database default"));
                }
                return Result<T, String>::ok(resource, std::move(rows));
            }
        }

        void finish_and_wait() noexcept {}

        void cancel_and_wait() noexcept {}
    };

    template <class T>
    DataServiceOperationResult<T, String>& start(Memory r, const CompiledQuery& q) {
        auto steps = items(input, "steps");
        auto step = calls.size() < steps.size() ? &steps[calls.size()] : nullptr;
        auto mode = step && optional(*step, "mode") ? text(field(*step, "mode")) : "success";
        auto count = step && optional(*step, "count") ? integer(field(*step, "count")) : 1;
        auto out = detail::model_json(resource);
        detail::model_put(out, "kind", std::same_as<T, std::uint64_t> ? "execute" : "fetch");
        detail::model_put(out, "sql", q.sql);
        detail::model_put(out, "params", values_json(resource, q.params));
        detail::model_put(out, "comment", q.comment ? Json(resource, *q.comment) : Json(resource));
        calls.emplace_back(std::move(out));
        return make_operation<Result<T, String>, Call<T>>(r, mode, count);
    }

    template <class T>
    struct Ready {
        using allocator_type = Allocator;
        T result;

        Ready(std::allocator_arg_t, Allocator a, T&& result)
            : result(detail::move_in(a.resource(), std::move(result))) {}

        std::optional<T> poll(WakeSignal&) {
            return std::move(result);
        }

        void finish_and_wait() noexcept {}

        void cancel_and_wait() noexcept {}
    };

    struct Tx : SqlTransportTransaction<String> {
        void cancel_sql_transaction() noexcept override {}

        using allocator_type = Allocator;
        Transport& inner;

        Tx(std::allocator_arg_t, Allocator a, Transport& inner)
            : SqlTransportTransaction<String>(a.resource()), inner(inner) {}

        DataServiceOperationResult<Rows, String>&
        fetch_all_compact_sql_transaction(Memory r, const CompiledQuery& q) override {
            return inner.fetch_all_compact_sql(r, q);
        }

        DataServiceOperationResult<std::uint64_t, String>&
        execute_sql_transaction(Memory r, const CompiledQuery& q) override {
            return inner.execute_sql(r, q);
        }

        DataServiceOperationResult<std::monostate, String>&
        commit_sql_transaction(Memory r) override {
            return make_operation<Result<std::monostate, String>,
                                  Ready<Result<std::monostate, String>>>(
                r, Result<std::monostate, String>::ok(r, std::monostate{}));
        }

        DataServiceOperationResult<std::monostate, String>&
        rollback_sql_transaction(Memory r) override {
            return commit_sql_transaction(r);
        }
    };

    DataServiceOperationResult<SqlTransactionHandle<String>, String>& begin_sql(Memory r) override {
        auto& tx = detail::async_arena(r).make<Tx>(*this);
        using R = Result<SqlTransactionHandle<String>, String>;
        return make_operation<R, Ready<R>>(
            r, R::ok(r, SqlTransactionHandle<String>(std::allocator_arg, Allocator(r), tx)));
    }

    DataServiceOperationResult<Vector<CompactRow>, String>&
    fetch_all_compact_sql(Memory r, const CompiledQuery& q) override {
        return start<Vector<CompactRow>>(r, q);
    }

    DataServiceOperationResult<std::uint64_t, String>&
    execute_sql(Memory r, const CompiledQuery& q) override {
        return start<std::uint64_t>(r, q);
    }
};

class KindCountingDialect : public FixtureDialect {
public:
    mutable std::size_t calls = 0;
    std::optional<std::size_t> fail_after;

    DatabaseKind kind() const override {
        ++calls;
        if (fail_after && calls > *fail_after) {
            throw std::runtime_error("fixture kind failure");
        }
        return FixtureDialect::kind();
    }
};

static Json evaluate(UserContext& user, const Json& j) {
    auto r = user.memory_resource();
    auto e = entity(r, field(j, "entity"));
    auto& d = user.make_resource<KindCountingDialect>();
    d.numbered = true;
    if (auto limit = optional(j, "kind_fail_after")) {
        d.fail_after = integer(*limit);
    }
    auto& schema = user.make_resource<Schema>(e);
    auto& transport = user.make<Transport>(j);
    auto& state = user.make<SqlExecutorState>();
    auto& executor =
        user.make<SqlDataServiceExecutor<String, Transport>>(d, transport, schema, state);
    auto& events = user.make<Json::Array>();
    std::optional<ExecutionObserver> observer;
    if (!optional(j, "observe") || std::get<bool>(field(j, "observe").data)) {
        observer.emplace(std::allocator_arg, Allocator(r), [r, &events](ExecutionMetadata m) {
            events.emplace_back(metadata_json(r, m));
        });
    }
    GuardedMutationExecutor<SqlExecutorError<String>>* active = &executor;
    if (auto p = optional(j, "transaction"); p && std::get<bool>(p->data)) {
        active = &dynamic_cast<GuardedMutationExecutor<SqlExecutorError<String>>&>(
            executor.begin(r).wait().value().get());
    }
    auto req = mutation(r, field(j, "request"), e.name);
    auto& op =
        optional(j, "guard")
            ? active->mutate_guarded_observed(
                  r,
                  GuardedMutationRequest(
                      std::allocator_arg, Allocator(r), req, expression(r, field(j, "guard"))),
                  observer)
            : active->mutate_observed(r, std::move(req), observer);
    auto out = detail::model_json(r);
    detail::model_put(out, "calls_before_poll", transport.calls.size());
    try {
        auto* result = op.poll();
        if (!result) {
            op.cancel();
            detail::model_put(out, "pending", true);
        } else if (!*result) {
            detail::model_put(out, "error", result->error().to_string(r));
        } else {
            const auto& v = result->value();
            detail::model_put(out, "affected", v.affected_rows);
            detail::model_put(out, "metadata", metadata_json(r, v.metadata));
            auto snapshot = Json(r);
            if (v.persisted_snapshot) {
                snapshot = detail::model_json(r);
                for (const auto& [k, x] : v.persisted_snapshot->fields) {
                    detail::model_put(snapshot, k, x.to_json_value(r));
                }
            }
            detail::model_put(out, "snapshot", snapshot);
            detail::model_put(out, "generated", v.generated_values.fields.size());
        }
    } catch (const std::exception&) {
        detail::model_put(out, "panic", true);
    }
    detail::model_put(out, "calls", transport.calls);
    detail::model_put(out, "events", events);
    if (auto flag = optional(j, "observe_kind"); flag && std::get<bool>(flag->data)) {
        detail::model_put(out, "kind_calls", d.calls);
    }
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
