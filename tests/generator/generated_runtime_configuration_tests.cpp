#include "../../examples/generated/include/cpp_examples.hpp"
#include <iostream>

using namespace teaql;

static void check(bool value, const char* message) {
    if (!value) {
        throw std::logic_error(message);
    }
}

static void copied_configuration_and_expired_executor() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto global_config = user_context->make_global<cpp_examples::ServiceRuntimeConfig>(":memory:");
    bool rejected = false;
    try {
        (void)cpp_examples::serviceRuntime(*user_context, user_context->get(global_config));
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    check(rejected, "Foreign configuration requires an explicit destination copy");
    auto config = user_context->make_ref<cpp_examples::ServiceRuntimeConfig>(
        user_context->get(global_config));
    check(user_context->get(config).memory_resource() == user_context->memory_resource(),
          "Derived generated config copies strings into its destination");
    Ref<cpp_examples::DataServiceExecutor> executor;
    {
        auto result = cpp_examples::serviceRuntime(*user_context, user_context->get(config));
        check(bool(result), "Explicit destination configuration assembles successfully");
        executor = result.value();
        auto registered = user_context->require_resource<cpp_examples::DataServiceExecutor>();
        check(registered && &registered.value().get() == &user_context->get(executor),
              "Assembly registers the distinct application executor resource");
        auto Q = user_context->queries<cpp_examples::query_factory>();
        const auto& before_schema = Q.productsMinimal()
                                        .comment("Check explicit schema lifecycle")
                                        .purpose("Verify assembly does not initialize schema")
                                        .executeForCount(*user_context)
                                        .wait();
        check(!before_schema, "Generated configuration leaves schema initialization explicit");
        cpp_examples::require_value(user_context->ensure_schema().wait());
        const auto& count = Q.productsMinimal()
                                .comment("Count initialized products")
                                .purpose("Verify configured provider execution")
                                .executeForCount(*user_context)
                                .wait();
        check(count && count.value() == 0, "Explicit schema initialization enables real queries");
    }
    user_context.reset();
    check(global.live_user_bytes() == 0, "Configuration and executor retain no User storage");
    auto next_user_context = global.make_user_context();
    rejected = false;
    try {
        (void)next_user_context->get(executor);
    } catch (const ReferenceError&) {
        rejected = true;
    }
    check(rejected, "A configured executor reference expires with its UserContext");
    rejected = false;
    try {
        (void)next_user_context->get(config);
    } catch (const ReferenceError&) {
        rejected = true;
    }
    check(rejected, "Generated config references expire without implicit retained ownership");
    auto rebound = next_user_context->rebind_global(global_config);
    check(next_user_context->get(rebound).databaseUrl() == ":memory:",
          "Only the explicit Global copy remains available to a new Context");
}

static void explicitly_rebound_global_connection() {
    GlobalContext global;
    auto user_context = global.make_user_context();
    auto connection = user_context->make_global_resource<SqliteConnection>(":memory:");
    cpp_examples::require_value(
        cpp_examples::serviceRuntimeFromConnection(*user_context, connection));
    cpp_examples::require_value(user_context->ensure_schema().wait());
    {
        auto Q = user_context->queries<cpp_examples::query_factory>();
        auto& product = Q.products()
                            .comment("Construct a retained database row")
                            .purpose("Verify the application-owned connection lifecycle")
                            .newEntity(*user_context);
        product.updateName("retained-product");
        cpp_examples::require_value(
            cpp_examples::AuditedSaveExt<cpp_examples::Product>::save(
                std::move(product).audit_as("Create the cross-context SQLite fixture"),
                *user_context)
                .wait());
    }
    user_context.reset();
    check(global.live_user_bytes() == 0, "Global connection does not retain its former User state");
    auto next_user_context = global.make_user_context();
    bool rejected = false;
    try {
        cpp_examples::require_value(
            cpp_examples::serviceRuntimeFromConnection(*next_user_context, connection));
    } catch (const ReferenceError&) {
        rejected = true;
    }
    check(rejected, "Global ownership does not preserve an expired User access identity");
    auto rebound = next_user_context->rebind_global(connection);
    cpp_examples::require_value(
        cpp_examples::serviceRuntimeFromConnection(*next_user_context, rebound));
    {
        auto Q = next_user_context->queries<cpp_examples::query_factory>();
        const auto& rows = cpp_examples::require_value(
            Q.products()
                .comment("Read the row through new Context access")
                .purpose("Verify explicit connection rebinding preserves the database")
                .executeForList(*next_user_context)
                .wait());
        check(rows.len() == 1 && rows[0].name() == "retained-product",
              "A rebound connection executes against the same application-owned database");
    }
    next_user_context.reset();
    check(global.live_user_bytes() == 0, "The second assembly also releases all User bytes");
}

int main() {
    auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
    try {
        copied_configuration_and_expired_executor();
        explicitly_rebound_global_connection();
    } catch (const std::exception& error) {
        std::pmr::set_default_resource(previous);
        std::cerr << error.what() << '\n';
        return 1;
    }
    std::pmr::set_default_resource(previous);
    std::cout << "Generated configuration, explicit schema and connection lifetimes passed\n";
    return 0;
}
