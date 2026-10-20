#define main constant_variant_baseline_main
#include "generated_constant_variants_probe.cpp"
#undef main

struct BootstrapRacePolicy final : MutationPolicy {
    SqliteConnection& rival;
    Json::Array& reviews;
    std::uint64_t target;
    mutable std::uint64_t remaining;
    bool repair;
    using allocator_type = Allocator;

    BootstrapRacePolicy(std::allocator_arg_t,
                        Allocator,
                        SqliteConnection& rival,
                        Json::Array& reviews,
                        std::uint64_t target,
                        std::uint64_t conflicts,
                        bool repair)
        : rival(rival), reviews(reviews), target(target), remaining(conflicts), repair(repair) {}

    MutationPolicyIdentity identity(Memory r) const override {
        return MutationPolicyIdentity(
            std::allocator_arg, Allocator(r), "bootstrap-race", "1", "fixture");
    }

    MutationDecision review(Memory r, const UserContext&, const MutationPlan& plan) const override {
        auto output = reviews.get_allocator().resource();
        Json::Object entry(output);
        entry.emplace("plan", Json(output, debug_string(output, plan)));
        entry.emplace("before", database_view(output, rival.native_handle(), true));
        bool injected = false;
        for (const auto& operation : plan.operations) {
            if (remaining && operation.kind == MutationOperationKind::Update &&
                operation.entity.entity == "Priority" && operation.entity.id.try_u64() == target) {
                std::string sql = "UPDATE priority_data SET version=version+1";
                if (repair) {
                    sql += target == 1 ? ",rank=-7" : ",rank=9";
                }
                sql += " WHERE id=" + std::to_string(target);
                fixture_sql(rival.native_handle(), sql);
                --remaining;
                injected = true;
                break;
            }
        }
        entry.emplace("injected", Json(output, injected));
        entry.emplace("remaining", Json(output, remaining));
        entry.emplace("after", database_view(output, rival.native_handle(), true));
        reviews.emplace_back(entry);
        return MutationDecision::allow(r);
    }
};

static Json conflict_observe(GlobalContext& global, const Json& input) {
    namespace domain = cpp_constants_numeric;
    auto r = global.memory_resource();
    constexpr auto uri = "file:teaql_bootstrap_conflict?mode=memory&cache=shared";
    auto& connection = global.make_resource<SqliteConnection>(uri);
    auto& rival = global.make_resource<SqliteConnection>(uri);
    auto& transport = global.make<SqliteMutationExecutor>(connection);
    auto create = [&](std::string_view mode) {
        auto owner = global.make_user_context();
        auto& user_context = *owner;
        auto& module = mode == "plain"      ? domain::module(user_context)
                       : mode == "checker"  ? domain::moduleWithCheckers(user_context)
                       : mode == "behavior" ? domain::moduleWithBehaviors(user_context)
                                            : domain::moduleWithBehaviorsAndCheckers(user_context);
        module.apply_to(user_context);
        use_sqlite_provider(user_context, global, transport);
        auto& ids = user_context.make<SqliteIdSpaceGenerator>(transport);
        user_context.set_internal_id_generator(user_context, ids);
        auto& cache = user_context.make<SqlExecutorState>();
        auto executor = user_context.make_ref<domain::ServiceRuntimeExecutor>(
            user_context.require_resource<SqliteDialect>().value().get(),
            transport,
            module.metadata,
            cache);
        user_context.register_executor(executor);
        user_context.set_trace_id("bootstrap-conflict");
        user_context.set_user_identifier("fixture");
        user_context.disable_sql_log();
        return owner;
    };
    auto warmup = create("combined");
    const auto& initial = warmup->ensure_schema().wait();
    if (!initial) {
        throw std::runtime_error(initial.error().to_string(r).c_str());
    }
    warmup.reset();
    auto target = std::get<std::uint64_t>(field(input, "target").data);
    auto version = std::get<std::uint64_t>(field(input, "version").data);
    fixture_sql(rival.native_handle(),
                "UPDATE priority_data SET rank=55,version=" + std::to_string(version) +
                    " WHERE id=" + std::to_string(target));
    auto user_context = create(text(field(input, "module")));
    if (std::get<bool>(field(input, "logging").data)) {
        user_context->enable_all_sql_log();
    }
    auto& reviews = global.make<Json::Array>();
    auto& policy = user_context->make<BootstrapRacePolicy>(
        rival,
        reviews,
        target,
        std::get<std::uint64_t>(field(input, "conflicts").data),
        std::get<bool>(field(input, "repair").data));
    auto source = user_context->memory_resource();
    auto registry = user_context->make_ref<FunctionMutationPolicyRegistry>(
        [&policy, source](Memory destination,
                          std::string_view) -> std::optional<MutationPolicyBorrow> {
            return MutationPolicyBorrow(
                std::allocator_arg, Allocator(destination), policy, detail::resource_arena(source));
        });
    user_context->set_mutation_policy_registry(registry);
    Json::Array rounds(r);
    for (std::uint64_t index = 0; index < std::get<std::uint64_t>(field(input, "rounds").data);
         ++index) {
        Json::Object round(r);
        round.emplace("before", database_view(r, connection.native_handle(), true));
        auto review_start = reviews.size();
        const auto& result = user_context->ensure_schema().wait();
        round.emplace("error", result ? Json(r) : Json(r, result.error().to_string(r)));
        round.emplace("after", database_view(r, connection.native_handle(), true));
        round.emplace("reviews",
                      Json(r, static_cast<std::uint64_t>(reviews.size() - review_start)));
        round.emplace("remaining", Json(r, policy.remaining));
        round.emplace("autocommit",
                      Json(r, sqlite3_get_autocommit(connection.native_handle()) != 0));
        rounds.emplace_back(round);
    }
    Json::Array logs(r);
    for (const auto& entry : user_context->sql_logs()) {
        Json::Object item(r);
        item.emplace("sql", Json(r, entry.sql));
        item.emplace("comment", entry.comment ? Json(r, *entry.comment) : Json(r));
        item.emplace("purpose", entry.purpose ? Json(r, *entry.purpose) : Json(r));
        item.emplace("audit_as", entry.audit_reason ? Json(r, *entry.audit_reason) : Json(r));
        Json::Array parameters(r);
        for (const auto& value : entry.params) {
            parameters.emplace_back(debug_string(r, value));
        }
        item.emplace("params", Json(r, parameters));
        logs.emplace_back(item);
    }
    user_context.reset();
    Json::Object output(r);
    output.emplace("rounds", Json(r, rounds));
    output.emplace("plans", Json(r, reviews));
    output.emplace("logs", Json(r, logs));
    output.emplace("after_close", database_view(r, rival.native_handle(), true));
    return Json(r, output);
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto input = Json::parse(global.memory_resource(), line);
            if (!input) {
                throw std::runtime_error("Invalid bootstrap conflict input");
            }
            std::cout << conflict_observe(global, *input).dump(global.memory_resource()) << '\n';
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
