#define main mutation_descriptor_historical_main
#include "sql_execution_probe.cpp"
#undef main

class ChangingSchema final : public SchemaProvider {
public:
    using allocator_type = Allocator;
    EntityDescriptor first, next;
    mutable Vector<String> lookups;
    bool switched = false;

    ChangingSchema(std::allocator_arg_t, Allocator a, const EntityDescriptor& entity, bool masked)
        : first(std::allocator_arg, a, entity), next(std::allocator_arg, a, entity),
          lookups(a.resource()) {
        first.audit_mask_fields.clear();
        next.audit_mask_fields.clear();
        (masked ? first : next).audit_mask_fields.emplace_back("name");
    }

    const EntityDescriptor* get_entity(std::string_view name) const override {
        lookups.emplace_back(name);
        const auto& entity = switched ? next : first;
        return name == entity.name ? &entity : nullptr;
    }
};

static Json observe_cache(UserContext& user_context, const Json& input) {
    auto r = user_context.memory_resource();
    auto descriptor = entity(r, field(input, "entity"));
    auto& dialect = user_context.make_resource<FixtureDialect>();
    dialect.numbered = true;
    auto& schema =
        user_context.make<ChangingSchema>(descriptor, std::get<bool>(field(input, "masked").data));
    auto& transport = user_context.make<Transport>(input);
    auto& cache = user_context.make<SqlExecutorState>();
    auto& executor = user_context.make<SqlDataServiceExecutor<String, Transport>>(
        dialect, transport, schema, cache);
    if (std::get<bool>(field(input, "prewarm").data)) {
        SelectQuery query(std::allocator_arg, Allocator(r), "Account");
        query.filter(Expr::eq(r, "name", "BEFORE")).limit(10);
        (void)executor.query_log_intent(r, query);
    }
    schema.switched = true;
    auto request = mutation(r, field(input, "request"), "Account");
    MutationExecutor<SqlExecutorError<String>>* active = &executor;
    if (std::get<bool>(field(input, "transaction").data)) {
        active = &executor.begin(r).wait().value().get();
    }
    const auto& result = active->mutate(r, std::move(request)).wait();
    auto out = detail::model_json(r);
    if (!result) {
        detail::model_put(out, "error", result.error().to_string(r));
    } else {
        const auto& value = result.value();
        detail::model_put(out, "affected", value.affected_rows);
        detail::model_put(out, "metadata", metadata_json(r, value.metadata));
        detail::model_put(out, "snapshot", Json(r));
        detail::model_put(out, "generated", value.generated_values.fields.size());
    }
    Json::Array lookups(r);
    for (const auto& name : schema.lookups) {
        lookups.emplace_back(name);
    }
    detail::model_put(out, "lookups", std::move(lookups));
    detail::model_put(out, "calls", transport.calls);
    return out;
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    GlobalContext global;
    for (std::string line; std::getline(std::cin, line);) {
        auto user_context = global.make_user_context();
        auto r = user_context->memory_resource();
        auto input = Json::parse(r, line);
        if (!input) {
            throw std::runtime_error("Invalid descriptor-cache fixture");
        }
        std::cout << observe_cache(*user_context, *input).dump(r) << '\n';
    }
    std::pmr::set_default_resource(previous);
}
