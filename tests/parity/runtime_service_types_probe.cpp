#include <teaql/core/detail/model_json.hpp>
#include <teaql/runtime/detail/data_service/entity_service.hpp>
#include "../support/relation_hydration_fixture.hpp"
#include <iostream>

using namespace teaql;
using E = hydration_fixture::UnusedExecutor;
using Access = detail::EntityDataServiceAccess;

static const Json& field(const Json& input, std::string_view name) {
    return std::get<Json::Object>(input.data).find(name)->second;
}

static bool flag(const Json& input, std::string_view name) {
    return std::get<bool>(field(input, name).data);
}

static Json secrets(Memory r, const SqlIntentRedactions& value) {
    Vector<String> safe(r), plaintext(r);
    value.extend_secrets(false, safe);
    value.extend_secrets(true, plaintext);
    Json::Array safe_json(r), plaintext_json(r);
    for (const auto& text : safe) {
        safe_json.emplace_back(text);
    }
    for (const auto& text : plaintext) {
        plaintext_json.emplace_back(text);
    }
    auto result = detail::model_json(r);
    detail::model_put(result, "safe", safe_json);
    detail::model_put(result, "plaintext", plaintext_json);
    return result;
}

static Json snapshot(Memory r, const EntityDataService<E>& service) {
    auto privacy = Access::intent(service);
    return privacy ? secrets(r, *privacy) : Json(r);
}

static void metadata(UserContext& user_context) {
    auto r = user_context.memory_resource();
    auto& store = user_context.make<InMemoryMetadataStore>();
    EntityDescriptor descriptor(std::allocator_arg, Allocator(r), "Item");
    descriptor.property("id", DataType::U64).id();
    descriptor.property("version", DataType::I64).version();
    descriptor.property("name", DataType::Text);
    descriptor.property("token", DataType::Text).column("access_token");
    descriptor.property("status", DataType::Text);
    descriptor.mask_fields({"name"});
    store.register_entity(std::move(descriptor));
    user_context.set_metadata(user_context, store);
}

static SqlIntentRedactions redaction(Memory r, std::string_view text) {
    SqlLogContext context(r);
    context.generated_sql = true;
    context.parameter_policies.push_back(SqlParameterLogPolicy::Unknown);
    Vector<Value> values(r);
    values.emplace_back(Value(r, text));
    return SqlIntentRedactions::from_bindings(r, context, values, "SELECT ?");
}

static ExecutionMetadata leaf(Memory r,
                              std::array<std::string_view, 2> values,
                              std::array<SqlParameterLogPolicy, 2> policies) {
    auto value = ExecutionMetadata::unrecorded_query(r, 1);
    value.sql_log.generated_sql = true;
    value.sql_log.parameter_policies.assign(policies.begin(), policies.end());
    value.parameterized_query.emplace("SELECT ?, ?", r);
    for (auto text : values) {
        value.params.emplace_back(Value(r, text));
    }
    return value;
}

static bool pristine(const ExecutionMetadata& value) {
    return value.sql_log.intent_redactions.is_empty() &&
           std::all_of(value.statements.begin(), value.statements.end(), pristine);
}

static Json evaluate(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto family = std::string_view(std::get<String>(field(input, "family").data));
    if (family != "metadata" || flag(input, "populated")) {
        metadata(user_context);
    }
    auto& executor = user_context.make_resource<E>();
    auto& service =
        EntityDataService<E>::for_executor(user_context, "Item", user_context, executor);
    auto output = detail::model_json(r);
    if (family == "intent") {
        if (flag(input, "trace")) {
            TraceChain trace(r);
            trace.emplace_back(
                TraceKind::AuditReason, "Injected", "trace cannot grant request intent");
            service.with_trace_context(trace);
        }
        if (flag(input, "override")) {
            auto intent = QueryIntent::create(r, "owned what", "owned why");
            Access::bind_query_intent(service, intent.value());
        }
        SelectQuery query(std::allocator_arg, Allocator(r), "Item");
        if (auto* text = std::get_if<String>(&field(input, "comment").data)) {
            query.comment_text.emplace(*text, r);
        }
        if (auto* text = std::get_if<String>(&field(input, "purpose").data)) {
            query.purpose_text.emplace(*text, r);
        }
        auto intent = Access::request_intent(service, query);
        auto result = detail::model_json(r);
        if (intent) {
            detail::model_put(result, "comment", intent.value().comment());
            detail::model_put(result, "purpose", intent.value().purpose());
        } else {
            detail::model_put(result, "error", intent.error().to_string(r));
        }
        detail::model_put(output, "result", result);
        detail::model_put(output, "owned", Access::has_request_intent(service));
        detail::model_put(output, "privacy", snapshot(r, service));
        detail::model_put(
            output, "mutation", Access::context_service(service).mutation_intent() != nullptr);
    } else if (family == "metadata") {
        user_context.set_sql_log_options({flag(input, "select"), flag(input, "mutation")});
        const auto& view = Access::context_service(service).metadata;
        detail::model_put(output, "count", std::uint64_t(view.all_entities(r).size()));
        detail::model_put(output, "item", view.entity("Item") != nullptr);
        detail::model_put(output, "missing", view.entity("Missing") != nullptr);
        detail::model_put(output, "debug", view.capture_query_debug());
        detail::model_put(output, "capture", view.capture_execution_metadata());
        detail::model_put(output, "query_observer", Access::observer(service).has_value());
        detail::model_put(
            output, "mutation_observer", view.mutation_diagnostic_observer(r).has_value());
    } else if (family == "plan") {
        auto& plan = user_context.make<GraphMutationPlan>();
        MutationValues values(r);
        values.fields.emplace(String("name", r), Value(r, "CURRENT-MASKED"));
        values.fields.emplace(String("status", r), Value(r, "CURRENT-PLAIN"));
        if (flag(input, "ignored")) {
            for (auto name : {"id", "version", "unmapped"}) {
                values.fields.emplace(String(name, r), Value(r, "IGNORED-VALUE"));
            }
        }
        std::optional<EntitySnapshot> old;
        if (flag(input, "old")) {
            old.emplace(r);
            old->fields.emplace(String("name", r), Value(r, "OLD-MASKED"));
            old->fields.emplace(String("token", r), Value(r, "OLD-CREDENTIAL"));
        }
        Vector<String> fields(r);
        fields.emplace_back("name");
        plan.push("Item", GraphMutationKind::Update, values, fields, nullptr, old);
        plan.rebuild_batches();
        if (flag(input, "missing")) {
            plan.batches.emplace_back(
                "Missing", GraphMutationKind::Create, std::span<const String>{});
        }
        auto scoped = Access::scoped(service, "Item");
        Access::bind_mutation_intent(scoped, "governed root reason").value();
        auto bound = Access::bind_plan_privacy(scoped, plan);
        if (!bound) {
            detail::model_put(output, "error", bound.error().to_string(r));
        } else {
            detail::model_put(output, "query", snapshot(r, scoped));
            detail::model_put(output,
                              "mutation",
                              secrets(r, *Access::context_service(scoped).mutation_privacy()));
            detail::model_put(output, "parent", snapshot(r, service));
        }
    } else if (family == "ledger") {
        auto& state = user_context.make<EntityRuntimeState>();
        Record fields(r);
        fields.emplace(String("id", r), Value(r, std::uint64_t{7}));
        fields.emplace(String("version", r), Value(r, std::int64_t{1}));
        fields.emplace(String("name", r), Value(r, "LEDGER-OLD-MASKED"));
        fields.emplace(String("token", r), Value(r, "LEDGER-OLD-CREDENTIAL"));
        CompactRow row(std::allocator_arg, Allocator(r), fields);
        state.set_original_compact_row("Item", row);
        const std::array definitions{DynamicFieldDefinitions::Field{"private", DataType::Text}};
        auto parsed = DynamicFieldDefinitions::from_fields(r, "Item", "v1", definitions);
        auto& schema = user_context.make<DynamicFieldDefinitions>(std::move(parsed).value());
        Value::Object values(r);
        values.emplace(String("private", r), Value(r, "DYNAMIC-ORIGINAL"));
        auto loaded = DynamicFieldValues::from_values(r, schema, values);
        state.install_loaded_dynamic_fields(loaded.value());
        EntityKey key(std::allocator_arg, Allocator(r), "Item", Value(r, std::uint64_t{7}));
        auto action = std::string_view(std::get<String>(field(input, "action").data));
        if (action == "set") {
            state.update_dynamic_field(key, "private", Value(r, "DYNAMIC-CURRENT")).value();
        } else if (action == "null") {
            state.update_dynamic_field(key, "private", Value(r)).value();
        } else {
            state.delete_dynamic_field(key, "private").value();
        }
        if (flag(input, "native")) {
            state.set(key, "name", Value(r, "LEDGER-CURRENT-MASKED"));
        }
        if (flag(input, "deleted")) {
            state.mark_as_delete(key);
        }
        auto scoped = Access::scoped(service, "Item");
        Access::bind_mutation_intent(scoped, "ledger root reason").value();
        Access::bind_ledger_privacy(scoped, state).value();
        detail::model_put(output, "query", snapshot(r, scoped));
        detail::model_put(
            output, "mutation", secrets(r, *Access::context_service(scoped).mutation_privacy()));
    } else if (family == "inherit") {
        auto child =
            flag(input, "scope") ? Access::query_scoped(service) : Access::scoped(service, "Item");
        if (flag(input, "scope")) {
            Access::extend_intent(child, redaction(r, "ANCESTOR-PRIVATE"));
        }
        auto value = ExecutionMetadata::unrecorded_query(r, 2);
        auto first = leaf(r,
                          {"LEFT-SENSITIVE", "LEFT-PUBLIC"},
                          {SqlParameterLogPolicy::Masked, SqlParameterLogPolicy::Plain});
        auto second = leaf(r,
                           {"LEFT-SENSITIVE", "RIGHT-SENSITIVE"},
                           {SqlParameterLogPolicy::Plain, SqlParameterLogPolicy::Unknown});
        if (flag(input, "nested")) {
            auto branch = ExecutionMetadata::unrecorded_query(r, 2);
            branch.statements.emplace_back(std::move(first));
            branch.statements.emplace_back(std::move(second));
            value.statements.emplace_back(std::move(branch));
        } else {
            value.statements.emplace_back(std::move(first));
            value.statements.emplace_back(std::move(second));
        }
        Access::record(child, value);
        detail::model_put(output, "child", snapshot(r, child));
        detail::model_put(output, "parent", snapshot(r, service));
        detail::model_put(output, "input_pristine", pristine(value));
        detail::model_put(output, "logs", std::uint64_t(user_context.sql_logs().size()));
    } else {
        throw std::logic_error("unknown observer family");
    }
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
        auto result = evaluate(*user_context, *input);
        std::cout << result.dump(r) << '\n';
    }
    std::pmr::set_default_resource(previous);
}
