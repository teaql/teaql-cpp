#include <teaql/provider_sqlite/detail/dynamic_fields.hpp>
#include <teaql/core/detail/text.hpp>
#include <sqlite3.h>
#include <bit>
#include <charconv>
#include <cmath>
#include <memory>
#include <algorithm>

namespace teaql::detail {
namespace {
constexpr std::string_view schema =
    R"sql(CREATE TABLE IF NOT EXISTS teaql_dynamic_field_storage_v1 (
namespace TEXT NOT NULL, owner_type TEXT NOT NULL, owner_id TEXT NOT NULL,
code TEXT NOT NULL, definition_revision TEXT NOT NULL, data_type TEXT NOT NULL,
is_null INTEGER NOT NULL CHECK (is_null IN (0,1)), payload TEXT,
PRIMARY KEY (namespace, owner_type, owner_id, code),
CHECK ((is_null=1 AND payload IS NULL) OR (is_null=0 AND payload IS NOT NULL))))sql";
constexpr std::string_view upsert = R"(INSERT INTO teaql_dynamic_field_storage_v1
(namespace,owner_type,owner_id,code,definition_revision,data_type,is_null,payload)
VALUES (?1,?2,?3,?4,?5,?6,?7,?8)
ON CONFLICT(namespace,owner_type,owner_id,code) DO UPDATE SET
definition_revision=excluded.definition_revision,data_type=excluded.data_type,
is_null=excluded.is_null,payload=excluded.payload)";
constexpr std::string_view erase =
    "DELETE FROM teaql_dynamic_field_storage_v1 WHERE namespace=?1 AND owner_type=?2 AND owner_id=?3 AND code=?4";

template <class T>
Result<T, DynamicFieldError> error(Memory r, std::string_view code, std::string_view field) {
    return Result<T, DynamicFieldError>::err(r, DynamicFieldError(r, code, field));
}

DynamicFieldError failed(Memory r) {
    return DynamicFieldError(r, "DYNAMIC_FIELD_STORAGE_FAILED", "storage");
}

void require_sql(Memory r, bool valid) {
    if (!valid) {
        throw failed(r);
    }
}

void check_namespace(Memory r, std::string_view name) {
    detail::trim(name);
    if (name.empty()) {
        throw DynamicFieldError(r, "DYNAMIC_FIELD_NAMESPACE_REQUIRED", "namespace");
    }
}

void bind_text(Memory r, sqlite3_stmt* stmt, int index, std::string_view text) {
    require_sql(r,
                sqlite3_bind_text(stmt, index, text.data(), int(text.size()), SQLITE_TRANSIENT) ==
                    SQLITE_OK);
}

std::string_view type_name(DataType type) {
    constexpr std::string_view names[]{
        "Bool", "I64", "U64", "F64", "Decimal", "Text", "LargeText", "Json", "Date", "Timestamp"};
    auto i = std::size_t(type);
    if (i >= std::size(names)) {
        throw std::invalid_argument("Invalid dynamic DataType");
    }
    return names[i];
}

String id_text(Memory r, std::uint64_t id) {
    char text[32];
    auto end = std::to_chars(text, text + sizeof(text), id).ptr;
    return String(text, end, r);
}

std::string_view column_text(Memory r, sqlite3_stmt* stmt, int column) {
    require_sql(r, sqlite3_column_type(stmt, column) == SQLITE_TEXT);
    return {reinterpret_cast<const char*>(sqlite3_column_text(stmt, column)),
            std::size_t(sqlite3_column_bytes(stmt, column))};
}

Value decode(Memory r,
             DataType type,
             bool null,
             std::optional<std::string_view> payload,
             std::string_view code) {
    auto invalid = [&] {
        return DynamicFieldError(r, "DYNAMIC_FIELD_INVALID_STORAGE", code);
    };
    if (null) {
        if (payload) {
            throw invalid();
        }
        return Value(r, nullptr);
    }
    if (!payload) {
        throw invalid();
    }
    auto parsed = Json::parse(r, *payload);
    if (!parsed) {
        throw invalid();
    }
    const auto& json = *parsed;
    auto text = std::get_if<String>(&json.data);
    switch (type) {
    case DataType::Bool:
        if (auto v = std::get_if<bool>(&json.data)) {
            return Value(r, *v);
        }
        break;
    case DataType::I64:
        if (auto v = std::get_if<std::int64_t>(&json.data)) {
            return Value(r, *v);
        }
        if (auto v = std::get_if<std::uint64_t>(&json.data); v && *v <= std::uint64_t(INT64_MAX)) {
            return Value(r, std::int64_t(*v));
        }
        break;
    case DataType::U64:
        if (auto v = std::get_if<std::uint64_t>(&json.data)) {
            return Value(r, *v);
        }
        if (auto v = std::get_if<std::int64_t>(&json.data); v && *v >= 0) {
            return Value(r, std::uint64_t(*v));
        }
        break;
    case DataType::F64:
        if (text) {
            std::string_view digits(*text);
            if (digits.starts_with('+')) {
                digits.remove_prefix(1);
            }
            std::uint64_t bits = 0;
            auto [end, ec] =
                std::from_chars(digits.data(), digits.data() + digits.size(), bits, 16);
            if (ec == std::errc{} && end == digits.data() + digits.size()) {
                auto number = std::bit_cast<double>(bits);
                if (std::isfinite(number)) {
                    return Value(r, number);
                }
            }
        }
        break;
    case DataType::Decimal:
        if (text) {
            if (auto v = Decimal::parse(*text)) {
                return Value(r, *v);
            }
        }
        break;
    case DataType::Text:
    case DataType::LargeText:
        if (text) {
            return Value(r, *text);
        }
        break;
    case DataType::Json:
        return Value(r, json);
    case DataType::Date:
        if (text) {
            if (auto v = Date::parse(*text)) {
                return Value(r, *v);
            }
        }
        break;
    case DataType::Timestamp:
        if (auto v = std::get_if<std::int64_t>(&json.data)) {
            return Value(r, Timestamp(*v));
        }
        if (auto v = std::get_if<std::uint64_t>(&json.data); v && *v <= std::uint64_t(INT64_MAX)) {
            return Value(r, Timestamp(std::int64_t(*v)));
        }
        break;
    }
    throw invalid();
}
}

Result<std::monostate, DynamicFieldError> sqlite_dynamic_ensure_schema(Memory r, sqlite3* db) {
    if (sqlite3_exec(db, schema.data(), nullptr, nullptr, nullptr) != SQLITE_OK) {
        return Result<std::monostate, DynamicFieldError>::err(r, failed(r));
    }
    return Result<std::monostate, DynamicFieldError>::ok(r, {});
}

Result<std::monostate, DynamicFieldError>
sqlite_dynamic_validate(Memory r, std::span<const DynamicFieldWrite> writes) {
    using Output = Result<std::monostate, DynamicFieldError>;
    try {
        for (const auto& write : writes) {
            check_namespace(r, write.namespace_name);
            if (!write.owner_id) {
                return error<std::monostate>(
                    r, "DYNAMIC_FIELD_OWNER_ID_REQUIRED", write.definitions.get().owner_type());
            }
            for (const auto& [code, mutation] : write.changes) {
                auto declared = write.definitions.get().data_type(code);
                if (!declared) {
                    return Output::err(r, std::move(declared).error());
                }
                if (auto v = mutation.value()) {
                    if (auto number = std::get_if<double>(&v->data);
                        number && !std::isfinite(*number)) {
                        return error<std::monostate>(r, "DYNAMIC_FIELD_NONFINITE_NUMBER", code);
                    }
                    auto checked = write.definitions.get().validate_value(code, *v);
                    if (!checked) {
                        return Output::err(r, std::move(checked).error());
                    }
                }
            }
        }
        return Output::ok(r, {});
    } catch (DynamicFieldError& e) {
        return Output::err(r, std::move(e));
    }
}

Result<std::monostate, DynamicFieldError> sqlite_dynamic_apply(
    Memory r, SqliteDynamicStatementCache& cache, std::span<const DynamicFieldWrite> writes) {
    using Output = Result<std::monostate, DynamicFieldError>;
    auto checked = sqlite_dynamic_validate(r, writes);
    if (!checked) {
        return checked;
    }
    try {
        auto set = cache.prepare(r, upsert);
        auto remove = cache.prepare(r, erase);
        for (const auto& write : writes) {
            auto id = id_text(r, write.owner_id);
            for (const auto& [code, mutation] : write.changes) {
                auto* stmt = mutation.is_delete() ? remove.get() : set.get();
                sqlite3_reset(stmt);
                sqlite3_clear_bindings(stmt);
                bind_text(r, stmt, 1, write.namespace_name);
                bind_text(r, stmt, 2, write.definitions.get().owner_type());
                bind_text(r, stmt, 3, id);
                bind_text(r, stmt, 4, code);
                if (auto v = mutation.value()) {
                    bind_text(r, stmt, 5, write.definitions.get().revision());
                    bind_text(
                        r, stmt, 6, type_name(write.definitions.get().data_type(code).value()));
                    require_sql(r, sqlite3_bind_int(stmt, 7, v->is_null()) == SQLITE_OK);
                    if (v->is_null()) {
                        require_sql(r, sqlite3_bind_null(stmt, 8) == SQLITE_OK);
                    } else {
                        String payload(r);
                        if (auto number = std::get_if<double>(&v->data)) {
                            char hex[16];
                            auto end = std::to_chars(
                                           hex, hex + 16, std::bit_cast<std::uint64_t>(*number), 16)
                                           .ptr;
                            payload += '"';
                            payload.append(16 - std::size_t(end - hex), '0');
                            payload.append(hex, end);
                            payload += '"';
                        } else {
                            payload = v->to_json_value(r).dump(r);
                        }
                        bind_text(r, stmt, 8, payload);
                    }
                }
                require_sql(r, sqlite3_step(stmt) == SQLITE_DONE);
            }
        }
        return Output::ok(r, {});
    } catch (DynamicFieldError& e) {
        return Output::err(r, std::move(e));
    }
}

Result<DynamicStorageRows, DynamicFieldError>
sqlite_dynamic_load(Memory r,
                    SqliteDynamicStatementCache& cache,
                    std::string_view namespace_name,
                    const DynamicFieldDefinitions& definitions,
                    std::span<const std::uint64_t> ids,
                    const DynamicFieldSelection& selection) {
    using Output = Result<DynamicStorageRows, DynamicFieldError>;
    try {
        check_namespace(r, namespace_name);
        auto valid = selection.validate(definitions);
        if (!valid) {
            return Output::err(r, std::move(valid).error());
        }
        DynamicStorageRows result(r);
        for (auto id : ids) {
            result.try_emplace(id);
        }
        for (std::size_t start = 0; start < ids.size(); start += 400) {
            auto chunk = ids.subspan(start, std::min<std::size_t>(400, ids.size() - start));
            String sql(
                "SELECT owner_id,code,definition_revision,data_type,is_null,payload FROM teaql_dynamic_field_storage_v1 WHERE namespace=? AND owner_type=? AND owner_id IN (",
                r);
            for (std::size_t i = 0; i < chunk.size(); ++i) {
                if (i) {
                    sql += ',';
                }
                sql += '?';
            }
            sql += ')';
            auto stmt = cache.prepare(r, sql);
            bind_text(r, stmt.get(), 1, namespace_name);
            bind_text(r, stmt.get(), 2, definitions.owner_type());
            for (std::size_t i = 0; i < chunk.size(); ++i) {
                bind_text(r, stmt.get(), int(i + 3), id_text(r, chunk[i]));
            }
            int status;
            while ((status = sqlite3_step(stmt.get())) == SQLITE_ROW) {
                auto owner = column_text(r, stmt.get(), 0);
                auto code = column_text(r, stmt.get(), 1);
                auto revision = column_text(r, stmt.get(), 2);
                auto stored_type = column_text(r, stmt.get(), 3);
                require_sql(r, sqlite3_column_type(stmt.get(), 4) == SQLITE_INTEGER);
                bool null = sqlite3_column_int64(stmt.get(), 4) != 0;
                std::optional<std::string_view> payload;
                if (sqlite3_column_type(stmt.get(), 5) != SQLITE_NULL) {
                    payload = column_text(r, stmt.get(), 5);
                }
                if (!selection.contains(code)) {
                    continue;
                }
                auto type = definitions.data_type(code);
                if (!type) {
                    return Output::err(r, std::move(type).error());
                }
                if (revision != definitions.revision() || stored_type != type_name(type.value())) {
                    return error<DynamicStorageRows>(r, "DYNAMIC_FIELD_DEFINITION_MISMATCH", code);
                }
                std::uint64_t id = 0;
                auto [end, ec] = std::from_chars(owner.data(), owner.data() + owner.size(), id);
                if (ec != std::errc{} || end != owner.data() + owner.size()) {
                    return error<DynamicStorageRows>(
                        r, "DYNAMIC_FIELD_INVALID_STORAGE", "owner_id");
                }
                auto target = result.find(id);
                if (target == result.end()) {
                    return error<DynamicStorageRows>(
                        r, "DYNAMIC_FIELD_UNEXPECTED_OWNER", definitions.owner_type());
                }
                target->second.insert_or_assign(String(code, r),
                                                decode(r, type.value(), null, payload, code));
            }
            require_sql(r, status == SQLITE_DONE);
        }
        return Output::ok(r, std::move(result));
    } catch (DynamicFieldError& e) {
        return Output::err(r, std::move(e));
    }
}
}
