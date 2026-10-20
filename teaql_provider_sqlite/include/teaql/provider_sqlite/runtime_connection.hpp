#pragma once
#include <teaql/provider_sqlite/runtime_config.hpp>
#include <teaql/provider_sqlite/provider.hpp>
#include <teaql/runtime/context.hpp>

namespace teaql {
// Prepare a generated SQLite URL, then allocate the actual connection resource
// in its execution UserContext. Successful results are checked borrowed IDs.
Result<Ref<SqliteConnection>, GeneratedRuntimeConfigurationError>
open_generated_sqlite_runtime_connection(UserContext&, std::string_view database_url);
}
