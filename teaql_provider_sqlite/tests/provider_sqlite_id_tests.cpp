#include <teaql/provider_sqlite/id.hpp>
#include <teaql/runtime/id_space.hpp>
#include <teaql/runtime/context.hpp>
#include <sqlite3.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <iostream>
#include <thread>
using namespace teaql;

static void check(bool ok, const char* message) {
    if (!ok) {
        throw std::runtime_error(message);
    }
}

static void execute_fixture_sql(SqliteConnection& connection, const char* text) {
    check(sqlite3_exec(connection.native_handle(), text, nullptr, nullptr, nullptr) == SQLITE_OK,
          "native fixture SQL");
}

static void lifetimes_and_transactions() {
    GlobalContext global;
    auto& connection = global.make_resource<SqliteConnection>();
    auto& provider = global.make<SqliteMutationExecutor>(connection);
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    auto& generator = user->make<SqliteIdSpaceGenerator>(provider);
    generator.with_table_name("custom ids");
    auto& retained = global.make<SqliteIdSpaceGenerator>(generator);
    check(generator.next_id(r, "OrderLine").value() == 1 &&
              generator.next_id(r, "order_line").value() == 2,
          "canonical model names share persistent ID space");
    check(bool(generator.ensure_floor(r, "OrderLine", 100)) &&
              generator.next_id(r, "OrderLine").value() == 101,
          "floor reserves fixed bootstrap ID range");
    check(bool(generator.ensure_floor(r, "OrderLine", 5)) &&
              generator.next_id(r, "OrderLine").value() == 102,
          "floor never reduces an allocated level");
    check(canonical_id_space_entity(r, "XMLThing") == "x_m_l_thing" &&
              canonical_id_space_entity(r, "中文ID") == "中文_i_d",
          "normalization retains source acronym and Unicode behavior");
    check(bool(provider.begin_transaction(r)), "synchronous ID-space transaction starts");
    check(generator.next_id(r, "OrderLine").value() == 103,
          "ID generator shares provider transaction");
    check(bool(provider.rollback_transaction(r)), "synchronous ID allocation rolls back");
    check(generator.next_id(r, "OrderLine").value() == 103,
          "rolled-back ID allocation is reusable, source semantics");
    auto& leased = provider.begin_sql(r).wait().value().get();
    check(!generator.next_id(r, "OrderLine"),
          "parent ID generator cannot bypass an asynchronously leased transaction");
    check(bool(leased.rollback_sql(r).wait()), "leased transaction releases its connection");
    auto& error = global.make<MutationExecutorError>(
        generator.ensure_floor(r, "OrderLine", UINT64_MAX).error());
    user.reset();
    check(error.memory_resource() == global.memory_resource() &&
              error.detail().find("exceeds i64") != std::string_view::npos,
          "ID error can be explicitly retained in GlobalContext");
    auto next = global.make_user_context();
    check(retained.next_id(next->memory_resource(), "OrderLine").value() == 104,
          "copied configuration retains global provider and deep-copies its table name");
    auto local = global.make_user_context();
    auto& local_connection = local->make_resource<SqliteConnection>();
    auto& local_provider = local->make<SqliteMutationExecutor>(local_connection);
    bool rejected = false;
    try {
        global.make<SqliteIdSpaceGenerator>(local_provider);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    check(rejected, "global generator cannot retain a user-owned connection");
}

static void strict_storage_and_failures() {
    GlobalContext global;
    auto& connection = global.make_resource<SqliteConnection>();
    auto& provider = global.make<SqliteMutationExecutor>(connection);
    auto& generator = global.make<SqliteIdSpaceGenerator>(provider);
    auto user = global.make_user_context();
    auto r = user->memory_resource();
    generator.ensure_table(r);
    execute_fixture_sql(connection, "INSERT INTO teaql_id_space VALUES('record',-2)");
    check(!generator.next_id(r, "Record") && generator.next_id(r, "Record").value() == 0,
          "negative generated ID fails after persisted update; zero remains valid");
    execute_fixture_sql(connection, "UPDATE teaql_id_space SET current_level=9223372036854775807");
    check(generator.next_id(r, "Record").error().kind() == MutationExecutorError::Kind::Bind,
          "signed SQLite ID overflow is typed");
    execute_fixture_sql(connection, "UPDATE teaql_id_space SET current_level='bad'");
    check(generator.next_id(r, "Record").error().detail() ==
              "Invalid column type Text at index: 0, name: current_level",
          "ID reads do not use permissive text-to-number conversion");
    execute_fixture_sql(
        connection,
        "DELETE FROM teaql_id_space; CREATE TRIGGER ignore_id BEFORE INSERT ON teaql_id_space BEGIN SELECT RAISE(IGNORE); END");
    check(generator.next_id(r, "Record")
                  .error()
                  .detail()
                  .find("changed 0 rows on optimistic-lock attempt 1") != std::string_view::npos,
          "ignored ID insert fails with native affected row count");
    check(generator.ensure_floor(r, "Record", 9)
                  .error()
                  .detail()
                  .find("after 100 optimistic-lock attempts") != std::string_view::npos,
          "floor retries ignored inserts exactly to source limit");
}

static void concurrent_connections() {
    constexpr auto uri = "file:teaql-id-concurrent-contract?mode=memory&cache=shared";
    GlobalContext global;
    auto& anchor = global.make_resource<SqliteConnection>(uri);
    auto& provider = global.make<SqliteMutationExecutor>(anchor);
    auto& generator = global.make<SqliteIdSpaceGenerator>(provider);
    generator.ensure_table(global.memory_resource());
    constexpr std::size_t workers = 4, count = 60;
    std::array<std::array<std::uint64_t, count>, workers> ids{};
    std::array<std::thread, workers> threads;
    std::atomic<bool> failed = false;
    for (std::size_t index = 0; index < workers; ++index) {
        threads[index] = std::thread([&, index] {
            try {
                GlobalContext owned;
                auto& connection = owned.make_resource<SqliteConnection>(uri);
                sqlite3_busy_timeout(connection.native_handle(), 0);
                auto& service = owned.make<SqliteMutationExecutor>(connection);
                auto& sequence = owned.make<SqliteIdSpaceGenerator>(service);
                for (std::size_t n = 0; n < count; ++n) {
                    auto user = owned.make_user_context();
                    auto result = sequence.next_id(user->memory_resource(), "ConcurrentRecord");
                    if (!result) {
                        failed = true;
                        return;
                    }
                    ids[index][n] = result.value();
                }
            } catch (...) {
                failed = true;
            }
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }
    check(!failed, "native busy/locked contention recovers within retry budget");
    std::array<std::uint64_t, workers * count> sorted{};
    for (std::size_t index = 0; index < workers; ++index) {
        std::copy(ids[index].begin(), ids[index].end(), sorted.begin() + index * count);
    }
    std::sort(sorted.begin(), sorted.end());
    for (std::size_t n = 0; n < sorted.size(); ++n) {
        check(sorted[n] == n + 1,
              "CAS IDs across real SQLite connections are unique and contiguous");
    }
}

static void transaction_resource_scope_isolation() {
    GlobalContext global;
    auto& connection = global.make_resource<SqliteConnection>();
    auto& provider = global.make<SqliteMutationExecutor>(connection);
    auto& generator = global.make<SqliteIdSpaceGenerator>(provider);
    auto user_context = global.make_user_context();
    auto other_user_context = global.make_user_context();
    auto r = user_context->memory_resource();
    check(bool(generator.ensure_floor(r, "Scoped", 100)), "reserve scoped identity floor");
    auto& transaction = provider.begin_sql(r).wait().value().get();
    check(!generator.next_id(r, "Scoped"), "unbound generator cannot enter leased transaction");
    bool wrong_domain = false;
    try {
        auto invalid =
            transaction.bind_transaction_resources(other_user_context->memory_resource());
    } catch (const std::invalid_argument&) {
        wrong_domain = true;
    }
    check(wrong_domain, "resource scope rejects a different Context");
    {
        auto scope = transaction.bind_transaction_resources(r);
        check(generator.next_id(r, "Scoped").value() == 101,
              "bound generator enters its transaction");
        check(!generator.next_id(other_user_context->memory_resource(), "Scoped"),
              "another Context cannot borrow the active ID scope");
        CompiledQuery query(std::allocator_arg, Allocator(r));
        query.sql = "SELECT current_level FROM teaql_id_space";
        check(!provider.fetch_all_compact(r, query),
              "ID scope does not authorize ordinary parent SQL");

        struct ScopeExit {};

        try {
            auto nested = transaction.bind_transaction_resources(r);
            auto moved = std::move(nested);
            check(generator.next_id(r, "Scoped").value() == 102,
                  "nested moved scope retains authorization");
            throw ScopeExit{};
        } catch (const ScopeExit&) {
        }
        check(generator.next_id(r, "Scoped").value() == 103,
              "nested exception restores outer authorization");
        auto* streaming = dynamic_cast<StreamingSqlTransport<MutationExecutorError>*>(&transaction);
        check(streaming != nullptr, "native transaction exposes actual streaming transport");
        query.sql = "SELECT 1 AS id UNION ALL SELECT 2 AS id";
        auto& stream = streaming->stream_sql(r, detail::clone_in(r, query), 1);
        auto* first = stream.wait_next();
        check(first && bool(*first), "transaction cursor acquires a real statement lease");
        check(!generator.next_id(r, "Scoped"),
              "ID scope cannot bypass an active transaction cursor");
        stream.cancel();
        check(generator.next_id(r, "Scoped").value() == 104,
              "cursor cancellation releases statement lease");
    }
    check(!generator.next_id(r, "Scoped"),
          "leaving scope revokes authorization before transaction end");
    check(bool(transaction.rollback_sql(r).wait()),
          "scoped allocations roll back with transaction");
    check(generator.next_id(r, "Scoped").value() == 101, "rollback restores persisted floor");
    bool consumed = false;
    try {
        auto invalid = transaction.bind_transaction_resources(r);
    } catch (const std::logic_error&) {
        consumed = true;
    }
    check(consumed, "consumed transaction cannot create a resource scope");
    auto& cancelled = provider.begin_sql(r).wait().value().get();
    {
        auto scope = cancelled.bind_transaction_resources(r);
        check(generator.next_id(r, "Scoped").value() == 102,
              "second transaction allocates inside scope");
        cancelled.cancel_sql();
        check(!generator.next_id(r, "Scoped"),
              "cancelled scope cannot silently allocate outside its transaction");
    }
    check(generator.next_id(r, "Scoped").value() == 102,
          "cancel and scope destruction restore normal access");
    {
        auto short_user_context = global.make_user_context();
        auto sr = short_user_context->memory_resource();
        auto& abandoned = provider.begin_sql(sr).wait().value().get();
        {
            auto scope = abandoned.bind_transaction_resources(sr);
            check(generator.next_id(sr, "Scoped").value() == 103,
                  "short Context allocates inside transaction");
        }
        short_user_context.reset();
    }
    check(generator.next_id(r, "Scoped").value() == 103,
          "Context close rolls back after scope release");
}

int main() {
    try {
        auto* previous = std::pmr::set_default_resource(std::pmr::null_memory_resource());
        lifetimes_and_transactions();
        strict_storage_and_failures();
        transaction_resource_scope_isolation();
        concurrent_connections();
        std::pmr::set_default_resource(previous);
        std::cout << "SQLite ID-space contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
