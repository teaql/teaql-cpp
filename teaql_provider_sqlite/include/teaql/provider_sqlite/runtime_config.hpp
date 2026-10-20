#pragma once
#include <teaql/runtime/generated_runtime_config.hpp>

namespace teaql {
// Preserve the generated source's path preparation separately from the native
// SqliteConnection filename API. The returned path belongs to the destination.
Result<String, GeneratedRuntimeConfigurationError>
prepare_generated_sqlite_runtime_path(Memory, std::string_view database_url);
}
