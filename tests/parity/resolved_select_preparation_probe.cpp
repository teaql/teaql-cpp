#include <teaql/runtime/data_service.hpp>
#include <teaql/runtime/module.hpp>
#include <teaql/core/detail/model_json.hpp>
#include "fixture_value.hpp"
#include <iostream>

using namespace teaql;

static bool flag(const Json& input, std::string_view key) {
    return std::get<bool>(field(input, key).data);
}

static Json snapshot(const SelectQuery& query) {
    auto r = query.memory_resource();
    auto output = detail::model_json(r);
    detail::model_put(output, "entity", query.entity);
    Json::Array projection(r), relations(r), trace(r);
    for (const auto& name : query.projection) {
        projection.emplace_back(name);
    }
    for (const auto& load : query.relations) {
        relations.emplace_back(load.name);
    }
    for (const auto& node : query.trace_chain) {
        trace.emplace_back(node.comment);
    }
    detail::model_put(output, "projection", projection);
    detail::model_put(output, "relations", relations);
    detail::model_put(output, "trace", trace);
    detail::model_put(
        output, "comment", query.comment_text ? Json(r, *query.comment_text) : Json(r));
    detail::model_put(
        output, "purpose", query.purpose_text ? Json(r, *query.purpose_text) : Json(r));
    return output;
}

static TraceNode frame(Memory r, std::string_view name) {
    return TraceNode(std::allocator_arg, Allocator(r), TraceKind::Request, "Parent", name);
}

struct State {
    using allocator_type = Allocator;
    Json input, output;
    Json::Array events;

    State(std::allocator_arg_t, Allocator a, const Json& input)
        : input(std::allocator_arg, a, input), output(a.resource()), events(a.resource()) {}
};

static RuntimeHookResult
apply(State& state, SelectQuery& query, std::string_view mode, std::string_view stage) {
    auto r = query.memory_resource();
    auto event = detail::model_json(r);
    detail::model_put(event, "stage", stage);
    detail::model_put(event, "query", snapshot(query));
    state.events.emplace_back(std::move(event));
    if (mode == "deny") {
        return RuntimeHookResult::err(r,
                                      stage == "policy"
                                          ? RuntimeError::policy(r, "select denied")
                                          : RuntimeError::behavior(r, "select denied"));
    }
    if (mode == "rewrite") {
        query.entity = stage == "policy" ? "Policy" : "Rewritten";
        query.projection.clear();
        query.projection.emplace_back(stage);
        query.relation("children");
        query.trace_chain.emplace_back(frame(r, stage));
    }
    return RuntimeHookResult::ok(r, {});
}

struct Hook final : EntityDataServiceBehavior {
    using allocator_type = Allocator;
    State& state;
    String stage;

    Hook(std::allocator_arg_t, Allocator a, State& state, std::string_view stage)
        : state(state), stage(stage, a.resource()) {}

    RuntimeHookResult before_select(Memory, const UserContext&, SelectQuery& query) const override {
        return apply(state, query, text(field(state.input, "behavior")), stage);
    }
};

struct Policy final : RequestPolicy {
    using allocator_type = Allocator;
    State& state;

    Policy(std::allocator_arg_t, Allocator, State& state) : state(state) {}

    RuntimeHookResult
    enforce_select(Memory, const UserContext&, SelectQuery& query) const override {
        return apply(state, query, text(field(state.input, "policy")), "policy");
    }
};

struct Executor final : QueryExecutor<String>, MutationExecutor<String> {
    DataServiceCapabilities capabilities() const override {
        return {};
    }

    DataServiceOperationResult<QueryResult, String>& query(Memory, QueryRequest) override {
        throw std::logic_error("preparation cannot query");
    }

    DataServiceOperationResult<MutationResult, String>& mutate(Memory, MutationRequest) override {
        throw std::logic_error("preparation cannot mutate");
    }
};

static Json run(const UserContext& user_context, State& state) {
    auto r = user_context.memory_resource();
    const auto& input = state.input;
    auto& executor = user_context.make_resource<Executor>();
    auto& repository =
        EntityDataService<Executor>::for_executor(user_context, "Parent", user_context, executor);
    TraceChain trace(r);
    if (flag(input, "trace")) {
        trace.emplace_back(frame(r, "service"));
        repository.with_trace_context(trace);
    }
    if (text(field(input, "intent")) == "bound") {
        detail::EntityDataServiceAccess::bind_query_intent(
            repository, QueryIntent::create(r, "bound comment", "bound purpose").value());
    }
    SelectQuery query(std::allocator_arg, Allocator(r), text(field(input, "entity")));
    query.project("name").purpose("  verify preparation order  ");
    auto intent = text(field(input, "intent"));
    if (intent == "valid") {
        query.comment_text.emplace("  inspect parents  ", r);
    }
    if (intent == "blank") {
        query.comment_text.emplace("  ", r);
    }
    if (flag(input, "trace")) {
        query.trace_chain.emplace_back(frame(r, "query"));
    }
    auto relations = text(field(input, "relations"));
    if (relations == "one" || relations == "duplicate") {
        query.relations.emplace_back("children");
    }
    if (relations == "duplicate") {
        query.relations.emplace_back("children");
    }
    if (relations == "missing") {
        query.relations.emplace_back("unknown");
    }
    auto before = snapshot(query).dump(r);
    auto prepared = detail::EntityDataServiceAccess::prepare_select(repository, query);
    auto result = detail::model_json(r);
    if (prepared) {
        detail::model_put(result, "ok", snapshot(prepared.value()));
    } else {
        detail::model_put(result, "error", prepared.error().to_string(r));
    }
    if (before != snapshot(query).dump(r)) {
        throw std::logic_error("source query mutated");
    }
    auto output = detail::model_json(r);
    detail::model_put(output, "result", result);
    detail::model_put(output, "source", snapshot(query));
    detail::model_put(output, "events", state.events);
    return output;
}

template <class T>
struct Ready {
    using allocator_type = Allocator;
    T result;

    Ready(std::allocator_arg_t, Allocator a, T result)
        : result(detail::move_in(a.resource(), std::move(result))) {}

    std::optional<T> poll(WakeSignal&) {
        return std::move(result);
    }

    void cancel_and_wait() noexcept {}

    void finish_and_wait() noexcept {}
};

struct Schema final : runtime::SchemaProvider {
    AsyncOperation<RuntimeSchemaResult>& ensure_schema(const UserContext& user_context,
                                                       const SchemaInvocation&) const override {
        auto r = user_context.memory_resource();
        return make_operation<RuntimeSchemaResult, Ready<RuntimeSchemaResult>>(
            r, RuntimeSchemaResult::ok(r, {}));
    }
};

static GeneratedSchemaBootstrapFuture& bootstrap(const UserContext& user_context) {
    auto& state = const_cast<State&>(*user_context.get_resource<State>());
    state.output = run(user_context, state);
    auto r = user_context.memory_resource();
    return make_operation<RuntimeSchemaResult, Ready<RuntimeSchemaResult>>(
        r, RuntimeSchemaResult::ok(r, {}));
}

static Json evaluate(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    user_context.disable_sql_log();
    auto& state = user_context.make<State>(input);
    user_context.insert_resource(user_context, state);
    auto& module = user_context.make<RuntimeModule>();
    if (flag(input, "metadata")) {
        for (std::string_view name : {"Parent", "Other", "Rewritten", "Policy"}) {
            EntityDescriptor descriptor(std::allocator_arg, Allocator(r), name);
            String key(name, r);
            for (char& ch : key) {
                ch = char(std::tolower(static_cast<unsigned char>(ch)));
            }
            key += "_key";
            descriptor.relation("children", "Child").local_key = key;
            module.descriptor(std::move(descriptor));
        }
    }
    if (text(field(input, "behavior")) != "none") {
        for (std::string_view name : {"Parent", "Other"}) {
            String stage("behavior:", r);
            stage += name;
            auto& hook = user_context.make<Hook>(state, stage);
            module.behavior(name, user_context, hook);
        }
    }
    if (flag(input, "bootstrap")) {
        module.generated_schema_bootstrap(bootstrap);
    }
    module.apply_to(user_context);
    if (text(field(input, "policy")) != "none") {
        auto& policy = user_context.make<Policy>(state);
        user_context.set_request_policy(user_context, policy);
    }
    if (flag(input, "bootstrap")) {
        auto& schema = user_context.make_resource<Schema>();
        user_context.set_schema_provider(user_context, schema);
        if (!user_context.ensure_schema().wait()) {
            throw std::logic_error("schema failed");
        }
        return detail::clone_in(r, state.output);
    }
    return run(user_context, state);
}

int main() {
    std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    std::string line;
    while (std::getline(std::cin, line)) {
        auto user_context = global.make_user_context();
        auto input = Json::parse(user_context->memory_resource(), line);
        if (!input) {
            throw std::runtime_error("invalid JSON");
        }
        std::cout << evaluate(*user_context, *input).dump(user_context->memory_resource()) << '\n';
    }
}
