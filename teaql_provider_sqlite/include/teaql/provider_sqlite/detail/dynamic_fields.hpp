#pragma once
#include <teaql/data_service/dynamic_fields.hpp>
struct sqlite3;

namespace teaql::detail {
Result<std::monostate, DynamicFieldError> sqlite_dynamic_ensure_schema(Memory, sqlite3*);
Result<std::monostate, DynamicFieldError>
    sqlite_dynamic_validate(Memory, std::span<const DynamicFieldWrite>);
Result<std::monostate, DynamicFieldError>
sqlite_dynamic_apply(Memory, sqlite3*, std::span<const DynamicFieldWrite>);
Result<DynamicStorageRows, DynamicFieldError> sqlite_dynamic_load(Memory,
                                                                  sqlite3*,
                                                                  std::string_view,
                                                                  const DynamicFieldDefinitions&,
                                                                  std::span<const std::uint64_t>,
                                                                  const DynamicFieldSelection&);
}
