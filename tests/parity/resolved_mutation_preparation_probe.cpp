#include <teaql/runtime/data_service.hpp>
#include <teaql/runtime/module.hpp>
#include <teaql/core/detail/debug.hpp>
#include <teaql/core/detail/model_json.hpp>
#include "fixture_value.hpp"
#include <iostream>

using namespace teaql;

static bool flag(const Json& input, std::string_view key) {
    return std::get<bool>(field(input, key).data);
}

static Json values(Memory r, const Record& values) {
    auto out = detail::model_json(r);
    for (const auto& [name, value] : values) {
        detail::model_put(out, name, detail::debug_value(r, value));
    }
    return out;
}

static Json trace(Memory r, std::span<const TraceNode> nodes) {
    Json::Array out(r);
    for (const auto& node : nodes) {
        auto frame = detail::model_json(r);
        detail::model_put(frame, "kind", unsigned(node.kind));
        detail::model_put(frame, "entity", node.entity_type);
        detail::model_put(frame, "comment", node.comment);
        out.emplace_back(std::move(frame));
    }
    return Json(r, out);
}

static TraceNode frame(Memory r, std::string_view name) {
    return TraceNode(std::allocator_arg, Allocator(r), TraceKind::Request, "Parent", name);
}

template <class C>
static Json snapshot(const C& command) {
    auto r = command.memory_resource();
    auto out = detail::model_json(r);
    detail::model_put(out, "entity", command.entity);
    detail::model_put(out, "values", values(r, command.values.fields));
    detail::model_put(out, "trace", trace(r, command.trace_chain));
    if constexpr (std::same_as<C, UpdateCommand>) {
        detail::model_put(out, "id", detail::debug_value(r, command.id));
        detail::model_put(out,
                          "version",
                          command.expected_version_value ? Json(r, *command.expected_version_value)
                                                         : Json(r));
        detail::model_put(
            out, "old", command.old_values ? values(r, command.old_values->fields) : Json(r));
    }
    return out;
}

static Value fixture_value(Memory r, std::string_view kind) {
    if (kind == "null") {
        return Value(r);
    }
    if (kind == "typed_null") {
        return Value(r, TypedNull{DataType::Text});
    }
    if (kind == "i64_zero") {
        return Value(r, std::int64_t(0));
    }
    if (kind == "u64_zero") {
        return Value(r, std::uint64_t(0));
    }
    if (kind == "positive") {
        return Value(r, std::uint64_t(7));
    }
    if (kind == "negative") {
        return Value(r, std::int64_t(-1));
    }
    if (kind == "text") {
        return Value(r, "provided");
    }
    if (kind == "empty_text") {
        return Value(r, "");
    }
    if (kind == "f64_zero") {
        return Value(r, 0.0);
    }
    if (kind == "bool") {
        return Value(r, false);
    }
    throw std::logic_error("unexpected fixture value");
}

static std::pair<std::string_view, std::string_view> fields(std::string_view shape,
                                                            std::string_view entity) {
    if (entity == "Target") {
        return {"target_id", "target_version"};
    }
    if (entity == "Policy") {
        return {"policy_id", "policy_version"};
    }
    if (shape == "custom") {
        return {"pk", "revision"};
    }
    return {"id", "version"};
}

struct State {
    using allocator_type = Allocator;
    Json input, output;
    Json::Array events;
    std::uint64_t next = 100;

    State(std::allocator_arg_t, Allocator a, const Json& input)
        : input(std::allocator_arg, a, input), output(a.resource()), events(a.resource()) {}
};

template <class C>
static RuntimeHookResult
apply(State& state, C& command, std::string_view stage, std::string_view mode) {
    auto r = command.memory_resource();
    auto event = detail::model_json(r);
    detail::model_put(event, "stage", stage);
    detail::model_put(event, "command", snapshot(command));
    state.events.emplace_back(std::move(event));
    if (mode == "deny") {
        return RuntimeHookResult::err(r,
                                      stage == "policy"
                                          ? RuntimeError::policy(r, "preparation denied")
                                          : RuntimeError::behavior(r, "preparation denied"));
    }
    if (mode == "retarget") {
        command.entity = stage == "policy" ? "Policy" : "Target";
        command.value("name", stage);
        command.trace_chain.emplace_back(frame(r, stage));
        if constexpr (std::same_as<C, UpdateCommand>) {
            command.id = Value(r, std::uint64_t(stage == "policy" ? 10 : 9));
            command.expected_version_value = stage == "policy" ? 88 : 77;
        }
    }
    return RuntimeHookResult::ok(r, {});
}

struct Hook final : EntityDataServiceBehavior {
    using allocator_type = Allocator;
    State& state;
    String stage;

    Hook(std::allocator_arg_t, Allocator a, State& state, std::string_view stage)
        : state(state), stage(stage, a.resource()) {}

    RuntimeHookResult
    before_insert(Memory, const UserContext&, InsertCommand& command) const override {
        return apply(state, command, stage, text(field(state.input, "behavior")));
    }

    RuntimeHookResult
    before_update(Memory, const UserContext&, UpdateCommand& command) const override {
        return apply(state, command, stage, text(field(state.input, "behavior")));
    }
};

struct Policy final : RequestPolicy {
    using allocator_type = Allocator;
    State& state;

    Policy(std::allocator_arg_t, Allocator, State& state) : state(state) {}

    RuntimeHookResult
    enforce_insert(Memory, const UserContext&, InsertCommand& command) const override {
        return apply(state, command, "policy", text(field(state.input, "policy")));
    }

    RuntimeHookResult
    enforce_update(Memory, const UserContext&, UpdateCommand& command) const override {
        return apply(state, command, "policy", text(field(state.input, "policy")));
    }
};

struct Id final : InternalIdGenerator {
    using allocator_type = Allocator;
    State& state;

    Id(std::allocator_arg_t, Allocator, State& state) : state(state) {}

    Result<std::uint64_t, RuntimeError> generate_id(Memory r,
                                                    std::string_view entity) const override {
        auto event = detail::model_json(r);
        detail::model_put(event, "stage", "id");
        detail::model_put(event, "entity", entity);
        state.events.emplace_back(std::move(event));
        using R = Result<std::uint64_t, RuntimeError>;
        if (flag(state.input, "id_failure")) {
            return R::err(r, RuntimeError::id_generation(r, "allocation failed"));
        }
        return R::ok(r, state.next++);
    }
};

struct Check final : Checker {
    using allocator_type = Allocator;
    State& state;
    String name;

    Check(std::allocator_arg_t, Allocator a, State& state, std::string_view name)
        : state(state), name(name, a.resource()) {}

    std::string_view entity() const override {
        return name;
    }

    void check_and_fix(const UserContext&,
                       EntityValues& checked,
                       const ObjectLocation& location,
                       CheckResults& results) const override {
        auto r = checked.memory_resource();
        auto event = detail::model_json(r);
        detail::model_put(event, "stage", "checker");
        detail::model_put(event, "entity", name);
        detail::model_put(event, "values", values(r, checked.fields));
        detail::model_put(event, "status", check_object_status_name(check_object_status(checked)));
        detail::model_put(event, "location", location.model_path(r));
        state.events.emplace_back(std::move(event));
        auto mode = text(field(state.input, "checker"));
        if (mode == "fix") {
            checked.fields.insert_or_assign(String("name", r), Value(r, "fixed"));
        }
        if (mode == "deny") {
            auto blocked = detail::clone_in(r, location);
            blocked.member("blocked");
            results.emplace_back(CheckResult::required(r, std::move(blocked)));
        }
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
        throw std::logic_error("preparation cannot persist");
    }
};

template <class C>
static void seed(C& command, const Json& input) {
    auto r = command.memory_resource();
    auto [id, version] = fields(text(field(input, "shape")), command.entity);
    command.value("__teaql_object_status", "update");
    if (flag(input, "name")) {
        command.value("name", "original");
    }
    auto id_kind = text(field(input, "id_kind"));
    auto version_kind = text(field(input, "version_kind"));
    if (id_kind != "missing") {
        command.value(id, fixture_value(r, id_kind));
    }
    if (version_kind != "missing") {
        command.value(version, fixture_value(r, version_kind));
    }
    command.trace_chain.emplace_back(frame(r, "command"));
}

static Json run(const UserContext& user_context, State& state) {
    auto r = user_context.memory_resource();
    const auto& input = state.input;
    auto& executor = user_context.make_resource<Executor>();
    auto& repository =
        EntityDataService<Executor>::for_executor(user_context, "Parent", user_context, executor);
    TraceChain lineage(r);
    lineage.emplace_back(frame(r, "service"));
    repository.with_trace_context(lineage);
    auto output = detail::model_json(r);
    auto collect = [&](const auto& command, auto prepared) {
        auto result = detail::model_json(r);
        if (prepared) {
            detail::model_put(result, "ok", snapshot(prepared.value()));
        } else {
            detail::model_put(result, "error", prepared.error().to_string(r));
        }
        detail::model_put(output, "result", result);
        detail::model_put(output, "source", snapshot(command));
    };
    auto entity = text(field(input, "entity"));
    if (text(field(input, "entry")) == "insert") {
        InsertCommand command(std::allocator_arg, Allocator(r), entity);
        seed(command, input);
        auto before = snapshot(command).dump(r);
        collect(command, repository.prepare_insert_command(command));
        if (before != snapshot(command).dump(r)) {
            throw std::logic_error("source insert mutated");
        }
    } else {
        UpdateCommand command(std::allocator_arg, Allocator(r), entity, std::uint64_t(7));
        seed(command, input);
        const auto& expected_version = field(input, "expected_version");
        if (!std::holds_alternative<std::monostate>(expected_version.data)) {
            command.expected_version(
                detail::model_integer<std::int64_t>(expected_version, "expected version"));
        }
        if (flag(input, "old")) {
            Record old(r);
            old.emplace(String("name", r), Value(r, "old"));
            command.old_values.emplace(std::allocator_arg, Allocator(r), old);
        }
        auto before = snapshot(command).dump(r);
        collect(command, repository.prepare_update_command(command));
        if (before != snapshot(command).dump(r)) {
            throw std::logic_error("source update mutated");
        }
    }
    detail::model_put(output, "events", state.events);
    detail::model_put(output, "next_id", state.next);
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
    auto shape = text(field(input, "shape"));
    if (shape != "missing") {
        for (std::string_view name : {"Parent", "Child", "Target", "Policy"}) {
            auto [id, version] = fields(shape, name);
            EntityDescriptor descriptor(std::allocator_arg, Allocator(r), name);
            if (shape != "no_id") {
                descriptor.property(id, DataType::U64).id();
            }
            if (shape != "no_version") {
                descriptor.property(version, DataType::I64).version();
            }
            descriptor.property("name", DataType::Text).not_null();
            descriptor.property("create_time", DataType::Timestamp);
            descriptor.property("update_time", DataType::Timestamp);
            module.descriptor(std::move(descriptor));
        }
    }
    if (text(field(input, "behavior")) != "none") {
        for (std::string_view name : {"Parent", "Child"}) {
            String stage("behavior:", r);
            stage += name;
            auto& hook = user_context.make<Hook>(state, stage);
            module.behavior(name, user_context, hook);
        }
    }
    if (text(field(input, "checker")) != "none") {
        for (std::string_view name : {"Parent", "Child", "Target", "Policy"}) {
            auto& checker = user_context.make<Check>(state, name);
            module.checker(user_context, checker);
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
    auto& generator = user_context.make<Id>(state);
    user_context.set_internal_id_generator(user_context, generator);
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
