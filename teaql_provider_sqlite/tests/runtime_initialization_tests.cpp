#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory_resource>
#include <stdexcept>
#include <string_view>
#include <utility>
#if defined(TEAQL_USE_MODULES)
import teaql_provider_sqlite;
#else
#include <teaql/provider_sqlite/runtime_assembly.hpp>
#include <teaql/runtime/generated_business_id_environment.hpp>
#endif

using namespace teaql;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::runtime_error(message);
    }
}

using BaseExecutor = SqlDataServiceExecutor<MutationExecutorError, SqliteMutationExecutor>;

class ObservedExecutor final : public BaseExecutor {
public:
    using allocator_type = Allocator;
    static inline unsigned constructions = 0;

    ObservedExecutor(std::allocator_arg_t,
                     Allocator allocator,
                     const SqlDialect& dialect,
                     SqliteMutationExecutor& transport,
                     const SchemaProvider& schema,
                     SqlExecutorState& state)
        : BaseExecutor(std::allocator_arg, allocator, dialect, transport, schema, state) {
        ++constructions;
    }
};

static RuntimeModule& make_module(UserContext& user_context) {
    auto& module = user_context.make<RuntimeModule>();
    EntityDescriptor descriptor(
        std::allocator_arg, Allocator(user_context.memory_resource()), "Record");
    module.descriptor(std::move(descriptor));
    return module;
}

static std::uint64_t table_count(UserContext& user_context) {
    auto resource = user_context.memory_resource();
    auto provider = user_context.require_resource<SqliteMutationExecutor>();
    check(bool(provider), "Provider infrastructure is installed before initialization");
    CompiledQuery query(std::allocator_arg, Allocator(resource));
    query.sql = "SELECT count(*) AS total FROM sqlite_master WHERE type='table'";
    auto rows = provider.value().get().fetch_all_compact(resource, query);
    check(bool(rows), "The caller's connection remains usable after initialization");
    return static_cast<std::uint64_t>(*rows.value()[0].get("total")->try_i64());
}

static void failed_and_successful_startup() {
    GlobalContext global;
    Ref<SqliteConnection> connection;
    for (unsigned round = 0; round != 3; ++round) {
        auto user_context_owner = global.make_user_context();
        auto& user_context = *user_context_owner;
        if (round == 0) {
            connection = user_context.make_global_resource<SqliteConnection>(":memory:");
        } else {
            connection = user_context.rebind_global(connection);
        }
        if (round == 0) {
            check(::unsetenv("TEAQL_BUSINESS_ID_KEY_V1_HEX") == 0, "Supply a missing key");
        } else {
            check(::setenv("TEAQL_BUSINESS_ID_KEY_V1_HEX",
                           "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f",
                           1) == 0,
                  "Supply the source key");
        }
        check(::setenv("TEAQL_BUSINESS_ID_KEY_VERSION", round == 1 ? "0" : "1", 1) == 0,
              "Supply invalid and valid source versions");
        {
            auto& module = make_module(user_context);
            auto constructed = ObservedExecutor::constructions;
            bool called = false;
            auto assembled = assemble_generated_sqlite_runtime<ObservedExecutor, true>(
                user_context, module, connection, module.metadata, [&](UserContext& context) {
                    called = true;
                    check(context.entity("Record") != nullptr,
                          "Module metadata precedes the initializer");
                    check(bool(context.require_resource<SqliteDialect>()) &&
                              bool(context.require_resource<SqliteMutationExecutor>()),
                          "Actual provider infrastructure precedes the initializer");
                    check(bool(context.business_ids()),
                          "Actual Business-ID infrastructure precedes key installation");
                    check(!context.require_resource<ObservedExecutor>() &&
                              ObservedExecutor::constructions == constructed,
                          "No executor is constructed or registered before key installation");
                    auto* previous = &context.business_ids().value().get();
                    auto installed = install_generated_business_id_environment_key(context);
                    if (installed) {
                        check(&context.business_ids().value().get() != previous,
                              "A successful initializer installs the configured service");
                    }
                    return installed;
                });
            check(called, "The synchronous initializer is actually invoked");
            check(table_count(user_context) == (round == 0 ? 0 : 1),
                  "Assembly does not initialize modeled or Business-ID schema");
            if (round != 2) {
                check(!assembled && !user_context.require_resource<ObservedExecutor>() &&
                          ObservedExecutor::constructions == constructed,
                      "Failed keys prevent executor construction and registration");
                check(assembled.error().memory_resource() == user_context.memory_resource(),
                      "Initialization errors retain their Context resource");
                check(assembled.error().kind() ==
                          (round == 0 ? GeneratedRuntimeConfigurationError::Kind::MissingEnv
                                      : GeneratedRuntimeConfigurationError::Kind::Configuration),
                      "The initializer's typed failure is preserved");
            } else {
                check(bool(assembled) && ObservedExecutor::constructions == constructed + 1,
                      "Successful key setup constructs exactly one executor");
                auto installed = user_context.require_resource<ObservedExecutor>();
                check(installed && &installed.value().get() == &user_context.get(assembled.value()),
                      "Successful startup registers the returned checked executor");
            }
            auto id = user_context.generate_id("Record");
            check(
                id && id.value() && *id.value() == round + 1,
                "Internal ID infrastructure survives the initializer outcome on its shared connection");
        }
        user_context_owner.reset();
        check(global.live_user_bytes() == 0,
              "Actual Context deletion cleans failed and successful startup allocations");
    }
}

static void manual_and_foreign_domains() {
    GlobalContext global;
    auto user_context_owner = global.make_user_context();
    auto& user_context = *user_context_owner;
    {
        check(::unsetenv("TEAQL_BUSINESS_ID_KEY_V1_HEX") == 0, "Remove the environment key");
        auto& module = make_module(user_context);
        auto connection = user_context.make_resource_ref<SqliteConnection>(":memory:");
        auto executor = assemble_generated_sqlite_runtime<ObservedExecutor, true>(
            user_context, module, connection, module.metadata);
        check(&user_context.require_resource<ObservedExecutor>().value().get() ==
                  &user_context.get(executor),
              "The existing manual-key assembly path does not read environment keys");
        check(bool(user_context.business_ids()),
              "Manual assembly retains Business-ID infrastructure");
    }
    user_context_owner.reset();
    check(global.live_user_bytes() == 0, "Manual startup releases all User allocations");

    auto first_user_context = global.make_user_context();
    auto next_user_context = global.make_user_context();
    {
        auto& module = make_module(*first_user_context);
        auto connection = next_user_context->make_resource_ref<SqliteConnection>(":memory:");
        bool called = false;
        auto initialize = [&](UserContext& context) {
            called = true;
            return install_generated_business_id_environment_key(context);
        };
        bool rejected = false;
        try {
            (void)assemble_generated_sqlite_runtime<ObservedExecutor, true>(
                *next_user_context, module, connection, module.metadata, initialize);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        check(rejected && !called && !next_user_context->require_resource<ObservedExecutor>() &&
                  !next_user_context->require_resource<SqliteMutationExecutor>(),
              "Foreign module validation precedes infrastructure and initializer side effects");
    }
    first_user_context.reset();
    next_user_context.reset();
    check(global.live_user_bytes() == 0, "Rejected startup retains no User storage");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        failed_and_successful_startup();
        manual_and_foreign_domains();
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
    std::cout << "Generated runtime initialization ordering and ownership passed\n";
}
