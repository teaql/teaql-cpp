#include <teaql/provider_sqlite/business_id.hpp>
#include <teaql/runtime/context.hpp>
#include <sqlite3.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <iostream>
#include <thread>
using namespace teaql;

static void check(bool ok, const char* message) {
    if (!ok)
        throw std::runtime_error(message);
}

static void sql(SqliteConnection& connection, const char* text) {
    check(sqlite3_exec(connection.native_handle(), text, nullptr, nullptr, nullptr) == SQLITE_OK,
          "native fixture SQL");
}

static BusinessIdPlan plan(Memory r,
                           std::string_view period = "20261003",
                           std::uint64_t initial = 1,
                           std::uint64_t maximum = 1000) {
    BusinessIdScope scope(std::allocator_arg, Allocator(r), "root", "Order", "orders", period);
    return BusinessIdPlan(
        std::allocator_arg, Allocator(r), scope, "ORD", std::nullopt, 6, "-", initial, maximum);
}

static void scopes_and_lifetimes() {
    GlobalContext global;
    auto& connection = global.make_resource<SqliteConnection>();
    auto& provider = global.make<SqliteMutationExecutor>(connection);
    auto& allocator = global.make<SqliteBusinessIdAllocator>(provider);
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    check(allocator.allocate(r, plan(r)).error().code == BusinessIdErrorCode::Allocation,
          "business schema installation is explicit");
    check(sqlite3_get_autocommit(connection.native_handle()),
          "missing schema allocation rolled back its own transaction");
    check(bool(static_cast<const BusinessIdSchemaContributor&>(allocator).ensure_schema(r, *user)),
          "business schema installs explicitly");
    const BusinessIdAllocator& spi = allocator;
    auto& allocation =
        user->make<Result<BusinessIdAllocation, BusinessIdError>>(spi.allocate(r, plan(r)));
    check(allocation && allocation.value().sequence == 1 &&
              allocation.value().scope.memory_resource() == r,
          "SPI allocation deep-copies scope into caller Context");
    auto& retained = global.make<BusinessIdAllocation>(allocation.value());
    check(spi.allocate(r, plan(r)).value().sequence == 2 &&
              spi.allocate(r, plan(r, "20261004")).value().sequence == 1,
          "independent canonical scope sequences");
    auto& invalid = global.make<BusinessIdError>(spi.allocate(r, plan(r, "invalid", 5, 4)).error());
    check(invalid.code == BusinessIdErrorCode::InvalidDefinition,
          "allocation range validated before native transaction");
    check(retained.scope.memory_resource() == global.memory_resource() &&
              retained.scope.period_key == "20261003",
          "global retained scope is an explicit copy");
    auto next = global.make_user_context();
    auto nr = next->memory_resource();
    auto& copy = next->make<SqliteBusinessIdAllocator>(allocator);
    copy.with_table_name("isolated business ids");
    check(bool(copy.ensure_schema(nr)) && copy.allocate(nr, plan(nr)).value().sequence == 1,
          "allocator clone owns independent table configuration");
    auto& error = user->make<BusinessIdError>(spi.allocate(r, plan(r, "20261003", 1, 2)).error());
    check(error.code == BusinessIdErrorCode::Exhausted &&
              spi.allocate(r, plan(r)).value().sequence == 3,
          "exhaustion rolls back sequence and version before retry with larger limit");
    check(sqlite3_get_autocommit(connection.native_handle()),
          "all successful allocations close their short transactions");
    user.reset();
    check(retained.scope.period_key == "20261003" &&
              invalid.code == BusinessIdErrorCode::InvalidDefinition,
          "explicit Global copies survive allocation Context destruction");
}

static void native_failures_and_save_isolation() {
    GlobalContext global;
    auto& connection = global.make_resource<SqliteConnection>();
    auto& provider = global.make<SqliteMutationExecutor>(connection);
    auto& allocator = global.make<SqliteBusinessIdAllocator>(provider);
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    allocator.ensure_schema(r);
    auto& save = provider.begin_sql(r).wait().value().get();
    auto failed = allocator.allocate(r, plan(r));
    check(!failed && !sqlite3_get_autocommit(connection.native_handle()),
          "failed nested begin does not roll back aggregate transaction");
    save.rollback_sql(r).wait();
    auto& separate = global.make_resource<SqliteConnection>();
    auto& separate_provider = global.make<SqliteMutationExecutor>(separate);
    auto& separate_allocator = global.make<SqliteBusinessIdAllocator>(separate_provider);
    separate_allocator.ensure_schema(r);
    auto& aggregate = provider.begin_sql(r).wait().value().get();
    check(separate_allocator.allocate(r, plan(r)).value().sequence == 1,
          "separate business allocator commits independently of save");
    aggregate.rollback_sql(r).wait();
    check(separate_allocator.allocate(r, plan(r)).value().sequence == 2,
          "aggregate rollback cannot reclaim committed business sequence");
    // A deferred FK commit failure leaves its native transaction open until the
    // owning Context closes, matching source error timing and C++ cleanup rules.
    auto scope = global.make_user_context();
    auto sr = scope->memory_resource();
    sql(connection,
        "DROP TABLE teaql_business_id_space; PRAGMA foreign_keys=ON; CREATE TABLE parent(k TEXT PRIMARY KEY); CREATE TABLE teaql_business_id_space(scope_key TEXT PRIMARY KEY REFERENCES parent(k) DEFERRABLE INITIALLY DEFERRED,current_level INTEGER NOT NULL,version INTEGER NOT NULL)");
    auto& error = scope->make<BusinessIdError>(allocator.allocate(sr, plan(sr)).error());
    check(error.code == BusinessIdErrorCode::Allocation &&
              !sqlite3_get_autocommit(connection.native_handle()),
          "commit failure preserves native error and active transaction");
    scope.reset();
    check(sqlite3_get_autocommit(connection.native_handle()),
          "failed allocation commit rolls back before Context memory reclamation");
    auto local = global.make_user_context();
    auto& local_connection = local->make_resource<SqliteConnection>();
    auto& local_provider = local->make<SqliteMutationExecutor>(local_connection);
    auto& local_allocator = local->make<SqliteBusinessIdAllocator>(local_provider);
    bool rejected = false;
    try {
        local_allocator.allocate(global.memory_resource(), plan(global.memory_resource()));
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    check(rejected,
          "allocation cannot retain failure cleanup in a longer Context than its connection");
}

static void concurrent_allocations() {
    constexpr auto uri = "file:teaql-business-id-contract?mode=memory&cache=shared";
    GlobalContext global;
    auto& connection = global.make_resource<SqliteConnection>(uri);
    auto& provider = global.make<SqliteMutationExecutor>(connection);
    auto& allocator = global.make<SqliteBusinessIdAllocator>(provider);
    allocator.ensure_schema(global.memory_resource());
    constexpr std::size_t workers = 4, count = 40;
    std::array<std::array<std::uint64_t, count>, workers> ids{};
    std::array<std::thread, workers> threads;
    std::atomic<bool> failed = false;
    for (std::size_t index = 0; index < workers; ++index)
        threads[index] = std::thread([&, index] {
            try {
                GlobalContext owned;
                auto& db = owned.make_resource<SqliteConnection>(uri);
                sqlite3_busy_timeout(db.native_handle(), 0);
                auto& service = owned.make<SqliteMutationExecutor>(db);
                auto& sequence = owned.make<SqliteBusinessIdAllocator>(service);
                for (std::size_t n = 0; n < count; ++n) {
                    auto user = owned.make_user_context();
                    auto result =
                        sequence.allocate(user->memory_resource(), plan(user->memory_resource()));
                    if (!result) {
                        failed = true;
                        return;
                    }
                    ids[index][n] = result.value().sequence;
                }
            } catch (...) {
                failed = true;
            }
        });
    for (auto& thread : threads)
        thread.join();
    check(!failed, "short business transactions retry lock contention");
    std::array<std::uint64_t, workers * count> sorted{};
    for (std::size_t index = 0; index < workers; ++index)
        std::copy(ids[index].begin(), ids[index].end(), sorted.begin() + index * count);
    std::sort(sorted.begin(), sorted.end());
    for (std::size_t n = 0; n < sorted.size(); ++n)
        check(sorted[n] == n + 1, "real concurrent business allocations are unique and contiguous");
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        scopes_and_lifetimes();
        native_failures_and_save_isolation();
        concurrent_allocations();
        std::pmr::set_default_resource(previous);
        std::cout << "SQLite business ID contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
