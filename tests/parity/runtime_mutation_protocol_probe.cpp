#include "../support/runtime_mutation_fixture.hpp"
#include <teaql/core/detail/model_json.hpp>
#include <teaql/core/detail/debug.hpp>
#include "fixture_value.hpp"
#include <iostream>

using namespace teaql;
using mutation_fixture::State;

struct Executor final : QueryExecutor<String>, MutationExecutor<String> {
    mutation_fixture::Executor implementation;
    bool event;

    Executor(State& state, bool event) : implementation(state), event(event) {}

    DataServiceCapabilities capabilities() const override {
        return {};
    }

    DataServiceOperationResult<QueryResult, String>& query(Memory r,
                                                           QueryRequest request) override {
        return implementation.query(r, std::move(request));
    }

    DataServiceOperationResult<MutationResult, String>& mutate(Memory r,
                                                               MutationRequest request) override {
        return mutate_observed(r, std::move(request), {});
    }

    DataServiceOperationResult<MutationResult, String>& mutate_observed(
        Memory r, MutationRequest request, std::optional<ExecutionObserver> observer) override {
        return implementation.mutate_observed(
            r, std::move(request), event ? std::move(observer) : std::nullopt);
    }
};

static bool flag(const Json& input, std::string_view name) {
    return std::get<bool>(field(input, name).data);
}

static Json record_out(Memory r, const Record& record) {
    auto output = detail::model_json(r);
    for (const auto& [key, value] : record) {
        detail::model_put(output, key, detail::debug_value(r, value));
    }
    return output;
}

struct Spy final : MetadataStore {
    using allocator_type = Allocator;
    State& state;
    bool observer;
    mutable Vector<std::uint64_t> logs;

    Spy(std::allocator_arg_t, Allocator a, State& state, bool observer)
        : state(state), observer(observer), logs(a.resource()) {}

    Memory memory_resource() const noexcept override {
        return state.resource;
    }

    const EntityDescriptor* entity(std::string_view) const override {
        return nullptr;
    }

    Vector<const EntityDescriptor*> all_entities(Memory r) const override {
        return Vector<const EntityDescriptor*>(r);
    }

    bool capture_query_debug() const override {
        throw std::logic_error("mutation read query debug flag");
    }

    bool capture_execution_metadata() const override {
        throw std::logic_error("mutation read query capture flag");
    }

    void record_metadata_log(const ExecutionMetadata& metadata) const override {
        state.note("record_metadata");
        logs.push_back(metadata.affected_rows.value_or(0));
    }

    struct Observer {
        using allocator_type = Allocator;
        ResourceBorrow<const Spy> spy;

        Observer(std::allocator_arg_t, Allocator a, const Spy& spy)
            : spy(std::allocator_arg, a, spy, detail::resource_arena(spy.memory_resource())) {}

        Observer(std::allocator_arg_t, Allocator a, const Observer& other)
            : spy(std::allocator_arg, a, other.spy) {}

        void operator()(ExecutionMetadata metadata) const {
            spy.get().state.note("callback");
            spy.get().record_metadata_log(metadata);
        }
    };

    std::optional<ExecutionObserver> mutation_diagnostic_observer(Memory r) const override {
        state.note("observer_factory");
        if (!observer) {
            return {};
        }
        return ExecutionObserver(
            std::allocator_arg, Allocator(r), Observer(std::allocator_arg, Allocator(r), *this));
    }
};

static Json request_out(Memory r, const MutationRequest& request) {
    auto output = detail::model_json(r);
    std::visit(
        [&](const auto& command) {
            using C = std::decay_t<decltype(command)>;
            if constexpr (!std::same_as<C, MutationRequest::Batch>) {
                constexpr auto kind = std::same_as<C, InsertCommand>   ? "insert"
                                      : std::same_as<C, UpdateCommand> ? "update"
                                      : std::same_as<C, DeleteCommand> ? "delete"
                                                                       : "recover";
                detail::model_put(output, "kind", kind);
                detail::model_put(output, "entity", command.entity);
                if constexpr (std::same_as<C, InsertCommand> || std::same_as<C, UpdateCommand>) {
                    detail::model_put(output, "values", record_out(r, command.values.fields));
                } else {
                    detail::model_put(output, "values", Json(r));
                }
                if constexpr (std::same_as<C, InsertCommand>) {
                    detail::model_put(output, "id", Json(r));
                    detail::model_put(output, "version", Json(r));
                } else {
                    detail::model_put(output, "id", detail::debug_value(r, command.id));
                    if constexpr (std::same_as<C, RecoverCommand>) {
                        detail::model_put(output, "version", command.expected_version);
                    } else {
                        detail::model_put(output,
                                          "version",
                                          command.expected_version_value
                                              ? Json(r, *command.expected_version_value)
                                              : Json(r));
                    }
                }
                if constexpr (std::same_as<C, UpdateCommand>) {
                    detail::model_put(output,
                                      "old",
                                      command.old_values ? record_out(r, command.old_values->fields)
                                                         : Json(r));
                } else {
                    detail::model_put(output, "old", Json(r));
                }
            } else {
                throw std::logic_error("runtime must expand batches");
            }
        },
        request.data);
    Json::Array trace(r);
    for (const auto& node : request.trace_chain()) {
        auto frame = detail::model_json(r);
        detail::model_put(frame, "kind", node.kind == TraceKind::Entity ? "Entity" : "AuditReason");
        detail::model_put(frame, "entity", node.entity_type);
        detail::model_put(frame, "id", node.entity_id ? Json(r, *node.entity_id) : Json(r));
        detail::model_put(frame, "comment", node.comment);
        trace.emplace_back(std::move(frame));
    }
    detail::model_put(output, "trace", trace);
    detail::model_put(output,
                      "privacy",
                      request.diagnostic_redactions()
                          ? Json(r, request.diagnostic_redactions()->is_empty())
                          : Json(r));
    detail::model_put(output, "comment", request.comment());
    return output;
}

static Json evaluate(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto& state = user_context.make<State>();
    state.failure = detail::model_integer<int>(field(input, "failure"), "failure");
    for (const auto& count : std::get<Json::Array>(field(input, "counts").data)) {
        state.affected.push_back(detail::model_integer<std::uint64_t>(count, "count"));
    }
    auto& executor = user_context.make_resource<Executor>(state, flag(input, "event"));
    auto& spy = user_context.make<Spy>(state, flag(input, "observer"));
    ResourceBorrow<Executor> borrow(
        std::allocator_arg, Allocator(r), executor, detail::resource_arena(r));
    auto& service = user_context.make<detail::RuntimeDataService<Spy, Executor>>(spy, borrow);
    auto id = value(r, field(input, "id"));
    SqlIntentRedactions privacy(r);
    privacy.capture_target_id(id);
    if (flag(input, "intent")) {
        auto intent = MutationIntent::create(r, "execute mutation protocol");
        service.with_mutation_intent(intent.value(), flag(input, "privacy") ? &privacy : nullptr);
    }
    Vector<std::optional<std::int64_t>> versions(r);
    for (const auto& version : std::get<Json::Array>(field(input, "versions").data)) {
        versions.emplace_back(std::holds_alternative<std::monostate>(version.data)
                                  ? std::nullopt
                                  : std::optional<std::int64_t>(
                                        detail::model_integer<std::int64_t>(version, "version")));
    }
    TraceChain frames(r);
    if (flag(input, "trace")) {
        frames.emplace_back(TraceKind::AuditReason, "Order", "approve", 99);
        frames.emplace_back(TraceKind::Entity, "Payment", "edit", 123);
    }
    auto n = detail::model_integer<std::size_t>(field(input, "length"), "length");
    auto m = detail::model_integer<std::size_t>(field(input, "ids"), "ids");
    auto entry = text(field(input, "entry"));
    auto payload = [&] {
        MutationValues values(r);
        values.fields.emplace(String("id", r), Value(r, id));
        values.fields.emplace(String("name", r), Value(r, "new"));
        values.fields.emplace(String("extra", r), Value(r, std::uint64_t(9)));
        return values;
    };
    auto old = [&] {
        EntitySnapshot snapshot(r);
        snapshot.fields.emplace(String("name", r), Value(r, "old"));
        return snapshot;
    };
    auto outcome = detail::model_json(r);
    auto collect = [&](auto& work) {
        const auto& result = work.wait();
        if (result) {
            detail::model_put(outcome, "rows", result.value());
        } else {
            detail::model_put(outcome, "error", result.error().to_string(r));
            detail::model_put(outcome,
                              "kind",
                              result.error().kind() == DataServiceError<String>::Kind::Runtime
                                  ? "Runtime"
                                  : "Executor");
        }
    };
    try {
        if (entry == "batch_insert") {
            auto& command = user_context.make<BatchInsertCommand>("Payment");
            for (std::size_t i = 0; i < n; ++i) {
                command.batch_values.emplace_back(payload());
            }
            if (flag(input, "trace")) {
                command.trace_chains.emplace_back(frames);
            }
            collect(service.batch_insert(command));
        } else if (entry == "batch_update") {
            auto& command = user_context.make<BatchUpdateCommand>("Payment");
            command.update_fields.emplace_back("name");
            command.update_fields.emplace_back("missing");
            for (std::size_t i = 0; i < n; ++i) {
                command.batch_values.emplace_back(payload());
            }
            for (std::size_t i = 0; i < m; ++i) {
                command.batch_ids.emplace_back(id);
            }
            command.batch_expected_versions = detail::clone_in(r, versions);
            if (flag(input, "old")) {
                command.batch_old_values.emplace_back(old());
            }
            if (flag(input, "trace")) {
                command.trace_chains.emplace_back(frames);
            }
            collect(service.batch_update(command));
        } else {
            auto invoke = [&]<class C>(C& command) {
                command.trace_chain = detail::clone_in(r, frames);
                if constexpr (std::same_as<C, InsertCommand> || std::same_as<C, UpdateCommand>) {
                    command.values = payload();
                }
                if constexpr (std::same_as<C, UpdateCommand> || std::same_as<C, DeleteCommand>) {
                    command.expected_version_value =
                        versions.empty() ? std::nullopt : versions.front();
                }
                if constexpr (std::same_as<C, UpdateCommand>) {
                    if (flag(input, "old")) {
                        command.old_values.emplace(old());
                    }
                }
                if (entry.ends_with("s")) {
                    Vector<C> commands(r);
                    for (std::size_t i = 0; i < n; ++i) {
                        commands.emplace_back(command);
                    }
                    if constexpr (std::same_as<C, InsertCommand>) {
                        collect(service.insert_many(commands));
                    } else if constexpr (std::same_as<C, UpdateCommand>) {
                        collect(service.update_many(commands));
                    } else if constexpr (std::same_as<C, DeleteCommand>) {
                        collect(service.delete_many(commands));
                    } else {
                        collect(service.recover_many(commands));
                    }
                } else {
                    if constexpr (std::same_as<C, InsertCommand>) {
                        collect(service.insert(command));
                    } else if constexpr (std::same_as<C, UpdateCommand>) {
                        collect(service.update(command));
                    } else if constexpr (std::same_as<C, DeleteCommand>) {
                        collect(service.delete_command(command));
                    } else {
                        collect(service.recover(command));
                    }
                }
                if (command.trace_chain != frames) {
                    throw std::logic_error("caller lineage changed");
                }
            };
            if (entry == "insert" || entry == "inserts") {
                auto& command = user_context.make<InsertCommand>("Payment");
                invoke(command);
            } else if (entry == "update" || entry == "updates") {
                auto& command = user_context.make<UpdateCommand>("Payment", id);
                invoke(command);
            } else if (entry == "delete" || entry == "deletes") {
                auto& command = user_context.make<DeleteCommand>("Payment", id);
                invoke(command);
            } else {
                auto& command = user_context.make<RecoverCommand>("Payment", id, 1);
                invoke(command);
            }
        }
    } catch (const std::out_of_range&) {
        detail::model_put(outcome, "panic", true);
    } catch (const std::overflow_error&) {
        detail::model_put(outcome, "panic", true);
    }
    auto output = detail::model_json(r);
    detail::model_put(output, "outcome", outcome);
    Json::Array requests(r), notes(r), logs(r);
    for (const auto& request : state.requests) {
        requests.emplace_back(request_out(r, request));
    }
    for (const auto& note : state.notes) {
        notes.emplace_back(note);
    }
    for (auto count : spy.logs) {
        logs.emplace_back(count);
    }
    detail::model_put(output, "requests", requests);
    detail::model_put(output, "notes", notes);
    detail::model_put(output, "logs", logs);
    return output;
}

int main() {
    auto previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    std::string line;
    while (std::getline(std::cin, line)) {
        GlobalContext global;
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        auto input = Json::parse(r, line);
        std::cout << evaluate(*user_context, *input).dump(r) << '\n';
    }
    std::pmr::set_default_resource(previous);
}
