#pragma once
#include <sqlite3.h>
#include <stdexcept>

namespace sqlite_fixture {
inline void check(bool value, const char* message) {
    if (!value)
        throw std::runtime_error(message);
}

struct Connection {
    sqlite3* handle = nullptr;

    Connection() {
        check(sqlite3_open(":memory:", &handle) == SQLITE_OK, "open SQLite fixture");
    }

    ~Connection() {
        if (sqlite3_close(handle) != SQLITE_OK)
            std::terminate();
    }

    Connection(const Connection&) = delete;

    void execute(const char* sql) {
        check(sqlite3_exec(handle, sql, nullptr, nullptr, nullptr) == SQLITE_OK,
              sqlite3_errmsg(handle));
    }
};

struct Statement {
    sqlite3_stmt* handle = nullptr;

    Statement(Connection& connection, const char* sql) {
        check(sqlite3_prepare_v2(connection.handle, sql, -1, &handle, nullptr) == SQLITE_OK,
              sqlite3_errmsg(connection.handle));
    }

    ~Statement() {
        sqlite3_finalize(handle);
    }

    Statement(const Statement&) = delete;
};
}
