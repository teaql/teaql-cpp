#include <teaql/provider_sqlite/schema.hpp>
#include <cstdlib>
#include <iostream>

using namespace teaql;

static void check(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

template <class Function>
static void rejected(Function&& function, const char* message) {
    bool failed = false;
    try {
        function();
    } catch (const std::logic_error&) {
        failed = true;
    }
    check(failed, message);
}

struct Fixture {
    GlobalContext global;
    SqliteConnection& connection = global.make_resource<SqliteConnection>();
    SqliteMutationExecutor& provider = global.make_resource<SqliteMutationExecutor>(connection);
    std::unique_ptr<UserContext> user_context = global.make_user_context();
    Memory memory = user_context->memory_resource();
    SqliteDialect& dialect = user_context->make_resource<SqliteDialect>();
    EntityDescriptor& descriptor = user_context->make<EntityDescriptor>("LeaseProbe");

    Fixture() {
        descriptor.table("lease_probe");
        descriptor.property("id", DataType::I64).id();
        descriptor.property("version", DataType::I64).version();
        descriptor.property("name", DataType::Text);
        descriptor.mask_fields({"name"});
        auto& metadata = user_context->make<InMemoryMetadataStore>();
        metadata.register_entity(detail::clone_in(memory, descriptor));
        user_context->set_metadata(*user_context, metadata);
        use_sqlite_provider(*user_context, global, provider);
        check(bool(user_context->ensure_schema().wait()), "Original modeled schema fixture");
    }

    CompiledQuery insert(std::int64_t id) {
        InsertCommand command(std::allocator_arg, Allocator(memory), "LeaseProbe");
        command.value("id", id).value("version", std::int64_t(1)).value("name", "lease fixture");
        return dialect.compile_insert(memory, descriptor, command);
    }

    CompiledQuery select() {
        SelectQuery query(std::allocator_arg, Allocator(memory), "LeaseProbe");
        query.limit(10).comment("inspect leased transport").purpose("verify transaction isolation");
        return dialect.compile_select(memory, descriptor, query);
    }

    std::size_t count() {
        auto query = select();
        const auto& result = provider.fetch_all_compact_sql(memory, query).wait();
        check(bool(result), "Root row reread");
        return result.value().size();
    }
};

static StreamingSqlTransport<MutationExecutorError>&
streaming(SqlTransportTransaction<MutationExecutorError>& transaction) {
    auto* result = dynamic_cast<StreamingSqlTransport<MutationExecutorError>*>(&transaction);
    check(result != nullptr, "Native transaction implements original StreamingSqlTransport");
    return *result;
}

static void ordinary_root_io_waits_until_transaction_releases_lease() {
    Fixture fixture;
    auto transaction_user_context = fixture.global.make_user_context();
    auto memory = transaction_user_context->memory_resource();
    auto& transaction = fixture.provider.begin_sql(memory).wait().value().get();
    auto compiled = fixture.insert(1);
    auto& root_write = fixture.provider.execute_sql(fixture.memory, compiled);
    check(root_write.poll() == nullptr, "Original root write waits for transaction lease");
    check(bool(transaction.rollback_sql(memory).wait()), "Original rollback releases lease");
    check(root_write.wait().value() == 1 && fixture.count() == 1,
          "Root write executes after the rolled-back transaction");
}

static void cancelled_waiter_and_dropped_transaction_leave_connection_reusable() {
    Fixture fixture;
    auto transaction_user_context = fixture.global.make_user_context();
    auto memory = transaction_user_context->memory_resource();
    auto& transaction = fixture.provider.begin_sql(memory).wait().value().get();
    auto compiled = fixture.insert(1);
    check(transaction.execute_sql(memory, compiled).wait().value() == 1, "Original in-tx write");
    auto& waiting = fixture.provider.begin_sql(fixture.memory);
    check(waiting.poll() == nullptr, "Original second begin waits");
    waiting.cancel();
    transaction_user_context.reset();
    check(fixture.count() == 0, "Actual transaction Context destruction rolls back");
    auto next_user_context = fixture.global.make_user_context();
    auto next_memory = next_user_context->memory_resource();
    auto& next = fixture.provider.begin_sql(next_memory).wait().value().get();
    check(next.execute_sql(next_memory, compiled).wait().value() == 1, "Connection reused");
    check(bool(next.commit_sql(next_memory).wait()) && fixture.count() == 1,
          "Original committed retry persists");
}

static void transaction_stream_and_repeated_probe_do_not_reacquire_lease() {
    Fixture fixture;
    auto transaction_user_context = fixture.global.make_user_context();
    auto memory = transaction_user_context->memory_resource();
    auto& transaction = fixture.provider.begin_sql(memory).wait().value().get();
    for (std::int64_t id = 1; id <= 3; ++id) {
        auto compiled = fixture.insert(id);
        check(transaction.execute_sql(memory, compiled).wait().value() == 1, "Original tx seed");
    }
    SelectQuery query(std::allocator_arg, Allocator(fixture.memory), "LeaseProbe");
    query.filter(Expr::eq(fixture.memory, "id", std::int64_t(1)))
        .limit(1)
        .comment("probe leased rows")
        .purpose("verify prepared statement reuse");
    auto compiled = fixture.dialect.compile_select(fixture.memory, fixture.descriptor, query);
    Vector<Value> values(fixture.memory);
    values.emplace_back(std::int64_t(1));
    values.emplace_back(std::int64_t(3));
    const auto& repeated =
        transaction.fetch_repeated_compact_sql(memory, compiled, 0, values).wait();
    check(bool(repeated) && repeated.value().size() == 2, "Original repeated probe count");
    check(repeated.value()[0].get("id")->try_i64() == 1 &&
              repeated.value()[1].get("id")->try_i64() == 3,
          "Original repeated probe identities and order");
    auto& stream = streaming(transaction).stream_sql(memory, fixture.select(), 2);
    std::size_t count = 0;
    while (const auto* chunk = stream.wait_next()) {
        check(bool(*chunk), "Original transaction stream succeeds");
        count += chunk->value().rows.size();
    }
    check(count == 3, "Original transaction stream sees all three rows");
    check(bool(transaction.commit_sql(memory).wait()), "Exhausted cursor releases tx borrow");
    auto& root_stream = fixture.provider.stream_sql(fixture.memory, fixture.select(), 1);
    const auto* first = root_stream.wait_next();
    check(first && bool(*first) && first->value().rows.size() == 1, "Original root stream chunk");
    auto& waiting = fixture.provider.begin_sql(fixture.memory);
    check(waiting.poll() == nullptr, "Root stream retains connection lease");
    root_stream.cancel();
    check(bool(waiting.wait().value().get().rollback_sql(fixture.memory).wait()),
          "Root early stream close releases waiting transaction");
}

static void open_stream_guards_terminal_domain_and_consumed_access() {
    Fixture fixture;
    auto transaction_user_context = fixture.global.make_user_context();
    auto memory = transaction_user_context->memory_resource();
    auto& transaction = fixture.provider.begin_sql(memory).wait().value().get();
    auto compiled = fixture.insert(1);
    transaction.execute_sql(memory, compiled).wait().value();
    auto& transport = streaming(transaction);
    auto& stream = transport.stream_sql(memory, fixture.select(), 1);
    rejected(
        [&] {
            transaction.commit_sql(memory);
        },
        "Unpolled stream prevents commit");
    check(!transaction.consumed(), "Rejected terminal does not consume transaction");
    rejected(
        [&] {
            transaction.rollback_sql(memory);
        },
        "Unpolled stream prevents rollback");
    rejected(
        [&] {
            transport.stream_sql(fixture.memory, fixture.select(), 1);
        },
        "Foreign stream output domain rejected");
    const auto* first = stream.wait_next();
    check(first && bool(*first), "Partially consume native transaction stream");
    rejected(
        [&] {
            transaction.commit_sql(memory);
        },
        "Partially consumed stream prevents commit");
    check(!transaction.consumed(), "Partial-cursor rejection leaves transaction active");
    stream.cancel();
    check(bool(transaction.commit_sql(memory).wait()), "Cancelled stream permits commit");
    rejected(
        [&] {
            transport.stream_sql(memory, fixture.select(), 1);
        },
        "Consumed transaction rejects new streams");
    check(fixture.count() == 1, "Early close does not discard the committed write");
}

static void transaction_cancellation_stops_stream_before_releasing_waiter() {
    Fixture fixture;
    auto transaction_user_context = fixture.global.make_user_context();
    auto memory = transaction_user_context->memory_resource();
    auto& transaction = fixture.provider.begin_sql(memory).wait().value().get();
    auto compiled = fixture.insert(1);
    transaction.execute_sql(memory, compiled).wait().value();
    auto& stream = streaming(transaction).stream_sql(memory, fixture.select(), 1);
    check(stream.wait_next() != nullptr, "Active native cursor before cancellation");
    auto& waiting = fixture.provider.begin_sql(fixture.memory);
    check(waiting.poll() == nullptr, "Independent begin waits on active cursor");
    transaction.cancel_sql();
    check(stream.status() == StreamStatus::Cancelled, "Transaction cancel stops tracked stream");
    check(bool(waiting.wait().value().get().rollback_sql(fixture.memory).wait()),
          "Cancelled transaction wakes and releases next begin");
    check(fixture.count() == 0, "Cancellation rolls back transaction-visible row");
}

static void actual_context_deletion_closes_cursor_and_rolls_back() {
    Fixture fixture;
    auto transaction_user_context = fixture.global.make_user_context();
    auto memory = transaction_user_context->memory_resource();
    auto& transaction = fixture.provider.begin_sql(memory).wait().value().get();
    auto compiled = fixture.insert(1);
    transaction.execute_sql(memory, compiled).wait().value();
    auto& stream = streaming(transaction).stream_sql(memory, fixture.select(), 1);
    check(stream.wait_next() != nullptr, "Native cursor is open at actual Context deletion");
    auto select = fixture.select();
    auto& waiting = fixture.provider.fetch_all_compact_sql(fixture.memory, select);
    check(waiting.poll() == nullptr, "Independent root read waits on open transaction cursor");
    transaction_user_context.reset();
    check(waiting.wait().value().empty(),
          "Actual Context deletion stops cursor and rolls back before root read resumes");
}

static void stream_error_can_be_closed_before_terminal_reuse() {
    Fixture fixture;
    auto transaction_user_context = fixture.global.make_user_context();
    auto memory = transaction_user_context->memory_resource();
    auto& transaction = fixture.provider.begin_sql(memory).wait().value().get();
    CompiledQuery invalid(std::allocator_arg, Allocator(fixture.memory));
    invalid.sql = "SELECT * FROM missing_transaction_stream_table";
    auto& stream = streaming(transaction).stream_sql(memory, std::move(invalid), 1);
    const auto* result = stream.wait_next();
    check(result && !*result, "Actual native prepare error is returned");
    stream.cancel();
    check(bool(transaction.rollback_sql(memory).wait()), "Error cursor cleanup permits rollback");
    check(fixture.count() == 0, "Connection remains reusable after error cleanup");
}

int main() {
    try {
        std::pmr::set_default_resource(std::pmr::null_memory_resource());
        check(setenv("TEAQL_LOG_ENDPOINT", "off", 1) == 0, "Disable diagnostic destination");
        ordinary_root_io_waits_until_transaction_releases_lease();
        cancelled_waiter_and_dropped_transaction_leave_connection_reusable();
        transaction_stream_and_repeated_probe_do_not_reacquire_lease();
        open_stream_guards_terminal_domain_and_consumed_access();
        transaction_cancellation_stops_stream_before_releasing_waiter();
        actual_context_deletion_closes_cursor_and_rolls_back();
        stream_error_can_be_closed_before_terminal_reuse();
        std::cout
            << "All three original lease bodies and four native transaction stream lifecycle bodies passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
