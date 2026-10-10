#include <sqlite3.h>
#include <cstdint>
#include <cstdio>
#include <cstring>

// Isolate native SQLite numeric conversion from all TEAQL code and JSON writers.
int main(int argc, char** argv) {
    sqlite3* connection = nullptr;
    if (sqlite3_open(":memory:", &connection) != SQLITE_OK) {
        sqlite3_close(connection);
        return 1;
    }
    if (argc > 2 && sqlite3_exec(connection, argv[1], nullptr, nullptr, nullptr) != SQLITE_OK) {
        std::fprintf(stderr, "%s\n", sqlite3_errmsg(connection));
        sqlite3_close(connection);
        return 1;
    }
    sqlite3_stmt* statement = nullptr;
    const char* sql = argc > 2 ? argv[2]
                             : argc > 1 ? argv[1]
                                        : "SELECT CAST('0.0000000000000000000000000001' AS NUMERIC)";
    if (sqlite3_prepare_v2(connection, sql, -1, &statement, nullptr) != SQLITE_OK) {
        std::fprintf(stderr, "%s\n", sqlite3_errmsg(connection));
        sqlite3_finalize(statement);
        sqlite3_close(connection);
        return 1;
    }
    const bool row = sqlite3_step(statement) == SQLITE_ROW;
    if (row) {
        const double value = sqlite3_column_double(statement, 0);
        std::uint64_t bits = 0;
        static_assert(sizeof(bits) == sizeof(value));
        std::memcpy(&bits, &value, sizeof(bits));
        std::printf("SQLite %s; sizeof(long double)=%zu; value=%.17g; bits=%016llx\n",
                    sqlite3_libversion(),
                    sizeof(long double),
                    value,
                    static_cast<unsigned long long>(bits));
    }
    sqlite3_finalize(statement);
    const int closed = sqlite3_close(connection);
    return row && closed == SQLITE_OK ? 0 : 1;
}
