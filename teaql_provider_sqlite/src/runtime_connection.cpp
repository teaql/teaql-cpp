#include <teaql/provider_sqlite/runtime_connection.hpp>

namespace teaql {
Result<Ref<SqliteConnection>, GeneratedRuntimeConfigurationError>
open_generated_sqlite_runtime_connection(UserContext& user_context, std::string_view database_url) {
    using Output = Result<Ref<SqliteConnection>, GeneratedRuntimeConfigurationError>;
    auto resource = user_context.memory_resource();
    auto filename = prepare_generated_sqlite_runtime_path(resource, database_url);
    if (!filename) {
        return Output::err(resource, std::move(filename).error());
    }
    try {
        auto connection = user_context.make_resource_ref<SqliteConnection>(filename.value());
        return Output::ok(resource, connection);
    } catch (const std::runtime_error& error) {
        String message(error.what(), resource);
        // The existing filename API exposes native SQLite exception text.
        // The generated source's CannotOpen adapter additionally retains path.
        if (message == "unable to open database file") {
            message += ": ";
            message += filename.value();
        }
        return Output::err(resource,
                           GeneratedRuntimeConfigurationError::connection_error(resource, message));
    }
}
}
