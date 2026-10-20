#pragma once
#include <teaql/data_service/dynamic_fields.hpp>
#include <memory>
struct sqlite3;
struct sqlite3_stmt;

namespace teaql::detail {
struct SqliteDynamicStatementRelease {
    void* owner = nullptr;
    void* slot = nullptr;
    void (*release)(void*, sqlite3_stmt*, void*) noexcept = nullptr;

    void operator()(sqlite3_stmt* statement) const noexcept {
        if (statement && release) {
            release(owner, statement, slot);
        }
    }
};

using SqliteDynamicStatement = std::unique_ptr<sqlite3_stmt, SqliteDynamicStatementRelease>;

// Provider-private borrowing bridge; its owner is the current native connection lease.
class SqliteDynamicStatementCache {
public:
    virtual ~SqliteDynamicStatementCache() = default;
    virtual SqliteDynamicStatement prepare(Memory, std::string_view sql) = 0;
};

Result<std::monostate, DynamicFieldError> sqlite_dynamic_ensure_schema(Memory, sqlite3*);
Result<std::monostate, DynamicFieldError>
    sqlite_dynamic_validate(Memory, std::span<const DynamicFieldWrite>);
Result<std::monostate, DynamicFieldError>
sqlite_dynamic_apply(Memory, SqliteDynamicStatementCache&, std::span<const DynamicFieldWrite>);
Result<DynamicStorageRows, DynamicFieldError> sqlite_dynamic_load(Memory,
                                                                  SqliteDynamicStatementCache&,
                                                                  std::string_view,
                                                                  const DynamicFieldDefinitions&,
                                                                  std::span<const std::uint64_t>,
                                                                  const DynamicFieldSelection&);
}
