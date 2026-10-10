#include <sqlite3.h>
#include <array>
#include <barrier>
#include <cstdio>
#include <thread>

int main() {
    const char* path = "/tmp/teaql-mips-schema-lock.db";
    std::remove(path);
    sqlite3* seed = nullptr;
    sqlite3_open_v2(
        path, &seed, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX, nullptr);
    sqlite3_exec(seed, "CREATE TABLE fixture (id INTEGER)", nullptr, nullptr, nullptr);
    sqlite3_close(seed);
    std::barrier start(8);
    std::array<std::thread, 8> threads;
    std::array<int, 8> errors{};
    for (unsigned i = 0; i < threads.size(); ++i) {
        threads[i] = std::thread([&, i] {
            sqlite3* db = nullptr;
            sqlite3_open_v2(path,
                            &db,
                            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX,
                            nullptr);
            sqlite3_busy_timeout(db, 30000);
            start.arrive_and_wait();
            errors[i] = sqlite3_exec(db, "BEGIN IMMEDIATE", nullptr, nullptr, nullptr);
            const bool begun = errors[i] == SQLITE_OK;
            if (errors[i] == SQLITE_OK) {
                errors[i] =
                    sqlite3_exec(db,
                                 "CREATE TABLE IF NOT EXISTS new_fixture (id INTEGER); COMMIT",
                                 nullptr,
                                 nullptr,
                                 nullptr);
            }
            std::printf(
                "worker %u: begun=%d result=%d %s\n", i, begun, errors[i], sqlite3_errmsg(db));
            sqlite3_close(db);
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }
    std::remove(path);
    for (auto error : errors) {
        if (error != SQLITE_OK) {
            return 1;
        }
    }
}
