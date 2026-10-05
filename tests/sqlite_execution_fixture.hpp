#pragma once
#include <teaql/sql/executor.hpp>
#include <teaql/provider_sqlite/dialect.hpp>
#include <teaql/runtime/context.hpp>
#include <sqlite3.h>
using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

struct Database {
    sqlite3* handle = nullptr;

    Database() {
        if (sqlite3_open(":memory:", &handle) != SQLITE_OK) {
            throw std::runtime_error("open fixture database");
        }
        exec(
            "CREATE TABLE records(id INTEGER PRIMARY KEY, version INTEGER, name TEXT, tenant INTEGER DEFAULT 2); INSERT INTO records VALUES(7,1,'old',2)");
    }

    ~Database() {
        sqlite3_close(handle);
    }

    void exec(const char* sql) {
        if (sqlite3_exec(handle, sql, nullptr, nullptr, nullptr) != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(handle));
        }
    }
};

struct Statement {
    sqlite3_stmt* handle = nullptr;

    Statement(Database& db, const CompiledQuery& query) {
        if (sqlite3_prepare_v2(db.handle, query.sql.c_str(), -1, &handle, nullptr) != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(db.handle));
        }
        try {
            for (std::size_t i = 0; i < query.params.size(); ++i) {
                const auto& v = query.params[i];
                int rc = std::visit(
                    [&](const auto& x) -> int {
                        using T = std::decay_t<decltype(x)>;
                        if constexpr (std::same_as<T, std::uint64_t> ||
                                      std::same_as<T, std::int64_t>) {
                            return sqlite3_bind_int64(handle, i + 1, static_cast<sqlite3_int64>(x));
                        } else if constexpr (std::same_as<T, String>) {
                            return sqlite3_bind_text(handle,
                                                     i + 1,
                                                     x.c_str(),
                                                     static_cast<int>(x.size()),
                                                     SQLITE_TRANSIENT);
                        } else if constexpr (std::same_as<T, std::monostate> ||
                                             std::same_as<T, TypedNull>) {
                            return sqlite3_bind_null(handle, i + 1);
                        } else {
                            throw std::logic_error("fixture binder only needs integer/text/null");
                        }
                    },
                    v.data);
                check(rc == SQLITE_OK, "fixture bind");
            }
        } catch (...) {
            sqlite3_finalize(handle);
            throw;
        }
    }

    ~Statement() {
        sqlite3_finalize(handle);
    }
};

struct Transport : SqlTransport<String> {
    using allocator_type = Allocator;
    Database& db;
    bool pause_read = false, fail_read = false;
    unsigned writes = 0, reads = 0, stopped = 0, cancelled = 0;

    Transport(std::allocator_arg_t, Allocator, Database& db) : db(db) {}

    template <class T>
    struct Call {
        using allocator_type = Allocator;
        Memory resource;
        Transport& transport;
        CompiledQuery query;
        bool entered = false;

        Call(std::allocator_arg_t, Allocator a, Transport& transport, const CompiledQuery& q)
            : resource(a.resource()), transport(transport), query(std::allocator_arg, a, q) {}

        std::optional<Result<T, String>> poll(WakeSignal&) {
            if (!entered) {
                entered = true;
                if constexpr (std::same_as<T, std::uint64_t>) {
                    ++transport.writes;
                } else {
                    ++transport.reads;
                }
            }
            if constexpr (!std::same_as<T, std::uint64_t>) {
                if (transport.pause_read) {
                    return {};
                }
                if (transport.fail_read) {
                    return Result<T, String>::err(resource, String("read failed", resource));
                }
            }
            Statement statement(transport.db, query);
            if constexpr (std::same_as<T, std::uint64_t>) {
                check(sqlite3_step(statement.handle) == SQLITE_DONE, "fixture write");
                return Result<T, String>::ok(resource,
                                             std::uint64_t(sqlite3_changes(transport.db.handle)));
            } else {
                T rows(resource);
                int rc;
                while ((rc = sqlite3_step(statement.handle)) == SQLITE_ROW) {
                    rows.emplace_back();
                    for (int i = 0; i < sqlite3_column_count(statement.handle); ++i) {
                        auto name = sqlite3_column_name(statement.handle, i);
                        if (sqlite3_column_type(statement.handle, i) == SQLITE_INTEGER) {
                            rows.back().insert(
                                name,
                                Value(resource,
                                      std::int64_t(sqlite3_column_int64(statement.handle, i))));
                        } else if (sqlite3_column_type(statement.handle, i) == SQLITE_NULL) {
                            rows.back().insert(name, Value(resource));
                        } else {
                            rows.back().insert(
                                name,
                                Value(resource,
                                      std::string_view(reinterpret_cast<const char*>(
                                          sqlite3_column_text(statement.handle, i)))));
                        }
                    }
                }
                check(rc == SQLITE_DONE, "fixture read");
                return Result<T, String>::ok(resource, std::move(rows));
            }
        }

        void finish_and_wait() noexcept {
            ++transport.stopped;
        }

        void cancel_and_wait() noexcept {
            ++transport.stopped;
            ++transport.cancelled;
        }
    };

    DataServiceOperationResult<std::uint64_t, String>&
    execute_sql(Memory r, const CompiledQuery& q) override {
        return make_operation<Result<std::uint64_t, String>, Call<std::uint64_t>>(r, *this, q);
    }

    DataServiceOperationResult<Vector<CompactRow>, String>&
    fetch_all_compact_sql(Memory r, const CompiledQuery& q) override {
        return make_operation<Result<Vector<CompactRow>, String>, Call<Vector<CompactRow>>>(
            r, *this, q);
    }
};

struct Schema : SchemaProvider {
    const EntityDescriptor& e;

    explicit Schema(const EntityDescriptor& e) : e(e) {}

    const EntityDescriptor* get_entity(std::string_view name) const override {
        return name == e.name ? &e : nullptr;
    }
};

static EntityDescriptor& model(GlobalContext& global) {
    auto& e = global.make<EntityDescriptor>("Record");
    e.table("records");
    e.property("id", DataType::U64).id();
    e.property("version", DataType::I64).version();
    e.property("name", DataType::Text);
    e.property("tenant", DataType::U64);
    e.mask_fields({"name"});
    return e;
}

static MutationRequest update(Memory r, std::string_view name = "new") {
    UpdateCommand c(std::allocator_arg, Allocator(r), "Record", std::uint64_t(7));
    c.value("name", name).audit_as("update reason");
    return MutationRequest(std::allocator_arg, Allocator(r), std::move(c));
}
