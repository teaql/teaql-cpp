#pragma once
#if defined(TEAQL_USE_MODULES)
import teaql_provider_sqlite;
#else
#include <teaql/provider_sqlite/schema.hpp>
#include <teaql/runtime/module.hpp>
#include <teaql/sql/executor.hpp>
#endif

namespace teaql::test {
// Controlled manual-metadata fixtures deliberately have no generated bootstrap,
// like their unchanged Rust probe contexts. Actual generated setup is tested
// separately through its public module/bootstrap path.
template <class Executor>
Ref<Executor> configure_schema_only(UserContext& user_context, RuntimeModule& module) {
    module.apply_to(user_context);
    user_context.set_metadata(user_context, module.metadata);
    auto& connection = user_context.make_resource<SqliteConnection>();
    auto& transport = user_context.make<SqliteMutationExecutor>(connection);
    use_sqlite_provider(user_context, user_context, transport);
    auto dialect = user_context.require_resource<SqliteDialect>();
    if (!dialect) {
        throw std::runtime_error(
            std::string(dialect.error().to_string(user_context.memory_resource())));
    }
    auto& state = user_context.make<SqlExecutorState>();
    auto executor =
        user_context.make_ref<Executor>(dialect.value().get(), transport, module.metadata, state);
    user_context.register_executor(executor);
    auto& result = user_context.ensure_schema().wait();
    if (!result) {
        throw std::runtime_error(
            std::string(result.error().to_string(user_context.memory_resource())));
    }
    return executor;
}
}
