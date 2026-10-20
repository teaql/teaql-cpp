#define main constant_variant_baseline_main
#include "generated_constant_variants_probe.cpp"
#undef main
#include <teaql/core/mutation_debug.hpp>
#include <teaql/core/query_debug.hpp>
#include <teaql/runtime/checker_model_debug.hpp>

struct ExtensionSettings {
    using allocator_type = Allocator;
    String tag;
    std::int64_t delta;
    std::uint64_t& destroyed;

    ExtensionSettings(std::allocator_arg_t,
                      Allocator a,
                      std::string_view tag,
                      std::int64_t delta,
                      std::uint64_t& destroyed)
        : tag(tag, a.resource()), delta(delta), destroyed(destroyed) {}

    ~ExtensionSettings() {
        ++destroyed;
    }
};

struct ExtensionChecker final : cpp_constants_numeric::PriorityCheckerLogic {
    Json::Array& hooks;
    String mode;
    using allocator_type = Allocator;

    ExtensionChecker(std::allocator_arg_t, Allocator a, Json::Array& hooks, std::string_view mode)
        : PriorityCheckerLogic(std::allocator_arg, a), hooks(hooks), mode(mode, a.resource()) {}

    void checkAndFixPriority(const UserContext& user_context,
                             cpp_constants_numeric::Priority& entity,
                             CheckObjectStatus status,
                             const ObjectLocation& location,
                             CheckResults& results) const override {
        auto& settings = user_context.require_resource<ExtensionSettings>().value().get();
        auto r = hooks.get_allocator().resource();
        Json::Object entry(r);
        entry.emplace("hook", Json(r, "checker"));
        entry.emplace("status", Json(r, debug_string(r, status)));
        entry.emplace("rank", Json(r, entity.rank()));
        entry.emplace("tag", Json(r, settings.tag));
        if (mode == "fix") {
            entity.updateRank(entity.rank() + settings.delta);
        } else if (mode == "deny") {
            ObjectLocation path(
                std::allocator_arg, Allocator(results.get_allocator().resource()), location);
            results.push_back(CheckResult::required(results.get_allocator().resource(),
                                                    std::move(path).member("rank")));
        }
        entry.emplace("after_rank", Json(r, entity.rank()));
        hooks.emplace_back(entry);
    }
};

struct ExtensionBehavior final : EntityDataServiceBehavior {
    Json::Array& hooks;
    String mode;
    using allocator_type = Allocator;

    ExtensionBehavior(std::allocator_arg_t, Allocator a, Json::Array& hooks, std::string_view mode)
        : hooks(hooks), mode(mode, a.resource()) {}

    template <class Command>
    RuntimeHookResult record(Memory r,
                             const UserContext& user_context,
                             std::string_view kind,
                             const Command& command) const {
        auto& settings = user_context.require_resource<ExtensionSettings>().value().get();
        auto destination = hooks.get_allocator().resource();
        Json::Object entry(destination);
        entry.emplace("hook", Json(destination, kind));
        entry.emplace("command", Json(destination, debug_string(destination, command)));
        entry.emplace("tag", Json(destination, settings.tag));
        hooks.emplace_back(entry);
        return mode == "deny" && kind != "select"
                   ? RuntimeHookResult::err(r, RuntimeError::event(r, "fixture behavior denial"))
                   : RuntimeHookResult::ok(r, {});
    }

    RuntimeHookResult
    before_select(Memory r, const UserContext& context, SelectQuery& query) const override {
        return record(r, context, "select", query);
    }

    RuntimeHookResult
    before_insert(Memory r, const UserContext& context, InsertCommand& command) const override {
        return record(r, context, "insert", command);
    }

    RuntimeHookResult
    before_update(Memory r, const UserContext& context, UpdateCommand& command) const override {
        return record(r, context, "update", command);
    }
};

static Json extension_observe(GlobalContext& global, const Json& input) {
    namespace domain = cpp_constants_numeric;
    auto r = global.memory_resource();
    auto& connection = global.make_resource<SqliteConnection>();
    auto& transport = global.make<SqliteMutationExecutor>(connection);
    auto& hooks = global.make<Json::Array>();
    auto& destroyed = global.make<std::uint64_t>(0);
    auto create = [&](bool extensions) {
        auto owner = global.make_user_context();
        auto& user_context = *owner;
        auto mode = extensions ? text(field(input, "module")) : "combined";
        auto& module = mode == "plain"      ? domain::module(user_context)
                       : mode == "checker"  ? domain::moduleWithCheckers(user_context)
                       : mode == "behavior" ? domain::moduleWithBehaviors(user_context)
                                            : domain::moduleWithBehaviorsAndCheckers(user_context);
        if (extensions) {
            auto checker_mode = text(field(input, "checker"));
            if (checker_mode != "none") {
                auto& logic = user_context.make<ExtensionChecker>(hooks, checker_mode);
                auto& checker = user_context.make<domain::PriorityChecker>(logic);
                auto& adapter =
                    user_context
                        .make<TypedEntityChecker<domain::Priority, domain::PriorityChecker>>(
                            user_context, checker);
                module.checker(user_context, adapter);
            }
            auto behavior_mode = text(field(input, "behavior"));
            if (behavior_mode != "none") {
                auto& behavior = user_context.make<ExtensionBehavior>(hooks, behavior_mode);
                module.entity_with_behavior<domain::Priority>(user_context, behavior);
            }
            auto delta =
                static_cast<std::int64_t>(std::get<std::uint64_t>(field(input, "delta").data));
            auto& settings =
                user_context.make<ExtensionSettings>(text(field(input, "tag")), delta, destroyed);
            user_context.insert_resource(user_context, settings);
        }
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
        user_context.set_trace_id("bootstrap-extension");
        user_context.set_user_identifier("fixture");
        user_context.enable_all_sql_log();
        return owner;
    };
    auto state = text(field(input, "state"));
    if (state != "fresh") {
        auto warmup = create(false);
        domain::require_value(warmup->ensure_schema().wait());
        warmup.reset();
        if (state == "drift") {
            fixture_sql(connection.native_handle(),
                        "UPDATE priority_data SET rank=55,version=3 WHERE id=1;");
        } else if (state == "missing") {
            fixture_sql(connection.native_handle(), "DELETE FROM priority_data WHERE id=22;");
        } else if (state == "deleted") {
            fixture_sql(connection.native_handle(),
                        "UPDATE priority_data SET version=-1 WHERE id=1;");
        } else if (state == "extra") {
            fixture_sql(
                connection.native_handle(),
                "INSERT INTO priority_data(id,name,code,platform,rank,weight,ratio,cost,day,note,optional_rank,version) VALUES(99,'Custom','CUSTOM',1,42,1.0,1.0,'0.00','1970-01-01','Extra',0,7);");
        } else if (state != "existing") {
            throw std::runtime_error("Unknown extension fixture state");
        }
    }
    Json::Array rounds(r);
    for (std::uint64_t index = 0; index < std::get<std::uint64_t>(field(input, "rounds").data);
         ++index) {
        auto user_context = create(true);
        const auto& result = user_context->ensure_schema().wait();
        Json::Object round(r);
        round.emplace("error", result ? Json(r) : Json(r, result.error().to_string(r)));
        round.emplace("database", database_view(r, connection.native_handle(), true));
        round.emplace("root",
                      user_context->require_active_root("Platform") ? Json(r, std::uint64_t(1))
                                                                    : Json(r));
        if (std::get<bool>(field(input, "continuation").data)) {
            auto Q = user_context->queries<domain::query_factory>();
            auto request = Q.priorities()
                               .selectSelfFields()
                               .orderByIdAsc()
                               .comment("what: read extension-controlled priorities")
                               .purpose("why: verify behavior after schema scope");
            const auto& queried = request.executeForList(*user_context).wait();
            round.emplace("query_error", queried ? Json(r) : Json(r, queried.error().to_string(r)));
            Json save_error(r);
            bool saved = false;
            if (queried && !queried.value().data.empty()) {
                auto& entity = user_context->make<domain::Priority>(queried.value().data.front());
                entity.updateRank(100);
                const auto& result =
                    domain::AuditedSaveExt<domain::Priority>::save(
                        std::move(entity).audit_as("Verify custom extension on ordinary update"),
                        *user_context)
                        .wait();
                if (!result) {
                    save_error = Json(r, result.error().to_string(r));
                }
                saved = static_cast<bool>(result);
            }
            round.emplace("saved", Json(r, saved));
            round.emplace("save_error", std::move(save_error));
            round.emplace("after_continuation", database_view(r, connection.native_handle(), true));
            auto& service = user_context
                                ->entity_data_service<domain::ServiceRuntimeExecutor>(
                                    domain::Priority::ENTITY_NAME)
                                .value()
                                .get();
            auto insert = service.insert_command();
            insert.value("id", std::uint64_t(99))
                .value("name", "Extension")
                .value("code", "EXTENSION")
                .value("platform_id", std::uint64_t(1))
                .value("rank", std::int64_t(100))
                .value("weight", 1.0)
                .value("ratio", 1.0f)
                .value("cost", Decimal::from_i64_with_scale(0, 2))
                .value("day", Date{})
                .value("note", Value(user_context->memory_resource()))
                .value("optional_rank", Value(user_context->memory_resource()));
            auto prepared_insert = service.prepare_insert_command(insert);
            round.emplace("prepared_insert",
                          prepared_insert ? Json(r, debug_string(r, prepared_insert.value()))
                                          : Json(r));
            round.emplace("prepare_insert_error",
                          prepared_insert ? Json(r)
                                          : Json(r, prepared_insert.error().to_string(r)));
            auto update = service.update_command(std::uint64_t(1));
            update.value("rank", std::int64_t(200));
            auto prepared_update = service.prepare_update_command(update);
            round.emplace("prepared_update",
                          prepared_update ? Json(r, debug_string(r, prepared_update.value()))
                                          : Json(r));
            round.emplace("prepare_update_error",
                          prepared_update ? Json(r)
                                          : Json(r, prepared_update.error().to_string(r)));
        }
        Json::Array logs(r);
        for (const auto& entry : user_context->sql_logs()) {
            Json::Object item(r);
            item.emplace("sql", Json(r, entry.sql));
            item.emplace("comment", entry.comment ? Json(r, *entry.comment) : Json(r));
            item.emplace("purpose", entry.purpose ? Json(r, *entry.purpose) : Json(r));
            item.emplace("audit_as", entry.audit_reason ? Json(r, *entry.audit_reason) : Json(r));
            Json::Array params(r);
            for (const auto& value : entry.params) {
                params.emplace_back(debug_string(r, value));
            }
            item.emplace("params", Json(r, params));
            logs.emplace_back(item);
        }
        round.emplace("logs", Json(r, logs));
        user_context.reset();
        round.emplace("destroyed", Json(r, destroyed));
        rounds.emplace_back(round);
    }
    Json::Object output(r);
    output.emplace("rounds", Json(r, rounds));
    output.emplace("hooks", Json(r, hooks));
    return Json(r, output);
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        for (std::string line; std::getline(std::cin, line);) {
            GlobalContext global;
            auto input = Json::parse(global.memory_resource(), line);
            if (!input) {
                throw std::runtime_error("Invalid bootstrap extension input");
            }
            std::cout << extension_observe(global, *input).dump(global.memory_resource()) << '\n';
        }
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
}
